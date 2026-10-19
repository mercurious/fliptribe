#include "sync.h"
#include "engine.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <strings.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

static constexpr uint64_t kPeerTtlUs = 3000000, kSendUs = 100000, kIfUs = 5000000;

int syncModeFromName(const std::string& s, int fallback) {
  for (int i = 0; i < NUM_SYNC_MODES; i++) if (strcasecmp(s.c_str(), kSyncModeNames[i]) == 0) return i;
  return fallback;
}

// ---------- protocol ----------
void WifiSync::coreInit(uint32_t id) {
  st = SyncStatus{};
  for (Peer& p : peers) p = Peer{};
  st.id = id;
  nerrs = erri = 0;
}

int WifiSync::makePacket(uint8_t* b, uint64_t nowUs) {
  SyncClock k = eng.syncClock((int64_t)nowUs);
  memcpy(b, "HI5S", 4);
  b[4] = 1;
  b[5] = k.playing ? 1 : 0;
  b[6] = (uint8_t)syncMode;
  b[7] = 0;
  memcpy(b + 8, &st.id, 4);
  float bpm = k.tempo10 / 10.0f;
  memcpy(b + 12, &bpm, 4);
  memcpy(b + 16, &k.pos, 8);
  memcpy(b + 24, &nowUs, 8);
  return kPacketLen;
}

WifiSync::Peer* WifiSync::peerSlot(uint32_t id, uint64_t nowUs) {
  Peer* free = nullptr;
  for (Peer& p : peers) {
    if (p.id == id && p.lastUs) return &p;
    if (!free && (!p.lastUs || nowUs - p.lastUs > kPeerTtlUs)) free = &p;
  }
  if (free) *free = Peer{};
  return free;
}

static double wrap16(double x) {
  x = std::fmod(x, 16.0);
  if (x > 8.0) x -= 16.0;
  if (x <= -8.0) x += 16.0;
  return x;
}

double WifiSync::medianErr() const {
  double a[kErrs];
  int n = nerrs < kErrs ? nerrs : kErrs;
  memcpy(a, errs, n * sizeof a[0]);
  for (int i = 1; i < n; i++)
    for (int j = i; j > 0 && a[j - 1] > a[j]; j--) std::swap(a[j], a[j - 1]);
  return n ? a[n / 2] : 0.0;
}

void WifiSync::setRate(double r) {
  if (r == sentRate) return;
  Command c; c.type = Command::SetRate; c.x = r;
  if (eng.send(c)) sentRate = r;
}

// a packet from the unit we follow: match tempo and transport, then phase
void WifiSync::follow(const Peer& p, uint64_t nowUs) {
  SyncClock k = eng.syncClock((int64_t)nowUs);
  // tempo is kept in tenths of a BPM here; the speed trim takes up whatever is left
  int t10 = (int)std::lround(p.bpm * 10.0f);
  if (std::fabs(p.bpm - k.tempo10 / 10.0f) > 0.01f && p.bpm >= 40.0f && p.bpm <= 250.0f && t10 != k.tempo10) {
    Command c; c.type = Command::SetTempo10; c.a = t10; eng.send(c);
    k.tempo10 = t10;
  }
  bool playing = k.playing, started = false;
  if (p.playing != (int)playing) {
    Command c;
    if (p.playing) { c.type = Command::PlayFrom; c.x = p.pos; } else c.type = Command::Stop;
    eng.send(c);
    playing = p.playing; started = playing;
    nerrs = erri = 0;
  }
  double rate = 1.0;
  if (playing) {
    double err = started ? 0.0 : wrap16(p.pos - k.pos);   // just started at the leader's position
    errs[erri] = err;
    erri = (erri + 1) % kErrs;
    if (nerrs < kErrs) nerrs++;
    double med = medianErr();
    if (nerrs >= 3 && std::fabs(med) > 0.5) {
      Command c; c.type = Command::Jump; c.x = med; eng.send(c);
      nerrs = erri = 0;
      med = 0.0;
    } else if (nerrs >= 3) {
      double kp = med * 0.15;
      rate = 1.0 + (kp > 0.03 ? 0.03 : kp < -0.03 ? -0.03 : kp);
    }
    st.offsetMs = (float)(med * 15000.0 / (k.tempo10 / 10.0));
  }
  setRate(rate);
}

void WifiSync::handlePacket(const uint8_t* b, int n, uint64_t nowUs) {
  if (n < kPacketLen || memcmp(b, "HI5S", 4) || b[4] != 1) return;
  uint32_t id; memcpy(&id, b + 8, 4);
  if (id == st.id || syncMode == SYNC_OFF) return;
  Peer* p = peerSlot(id, nowUs);
  if (!p) return;
  p->id = id;
  p->lastUs = nowUs;
  p->playing = b[5];
  p->role = b[6];
  memcpy(&p->bpm, b + 12, 4);
  memcpy(&p->pos, b + 16, 8);
  elect(nowUs);
  if (st.haveLeader && st.leaderId == id) follow(*p, nowUs);
}

// leader: a unit set to LEAD beats one on AUTO, then the lowest id wins; units set to FOLLOW never lead
void WifiSync::elect(uint64_t nowUs) {
  int alive = 0, bestRank = 3;
  uint32_t bestId = 0;
  int selfRank = syncMode == SYNC_LEAD ? 0 : syncMode == SYNC_AUTO ? 1 : 3;
  if (selfRank < 3) { bestRank = selfRank; bestId = st.id; }
  for (const Peer& p : peers) {
    if (!p.lastUs || nowUs - p.lastUs > kPeerTtlUs) continue;
    alive++;
    int rank = p.role == SYNC_LEAD ? 0 : p.role == SYNC_AUTO ? 1 : 3;
    if (rank < bestRank || (rank == bestRank && rank < 3 && p.id < bestId)) { bestRank = rank; bestId = p.id; }
  }
  st.peers = alive;
  bool leading = syncMode != SYNC_OFF && bestRank < 3 && bestId == st.id;
  bool haveLeader = syncMode != SYNC_OFF && bestRank < 3 && bestId != st.id;
  if (!haveLeader && st.haveLeader) {
    setRate(1.0);
    nerrs = erri = 0;
    st.offsetMs = 0.0f;
  }
  if (haveLeader && (!st.haveLeader || st.leaderId != bestId)) nerrs = erri = 0;
  st.leading = leading;
  st.haveLeader = haveLeader;
  st.leaderId = haveLeader ? bestId : 0;
}

// ---------- sockets ----------
void WifiSync::refreshBroadcast() {
  struct ifaddrs* ifa;
  nbcast = 0;
  if (getifaddrs(&ifa) != 0) return;
  for (struct ifaddrs* p = ifa; p && nbcast < 8; p = p->ifa_next) {
    if (!p->ifa_addr || p->ifa_addr->sa_family != AF_INET || !(p->ifa_flags & IFF_BROADCAST) || (p->ifa_flags & IFF_LOOPBACK) || !p->ifa_broadaddr) continue;
    bcast[nbcast++] = ((struct sockaddr_in*)p->ifa_broadaddr)->sin_addr.s_addr;
  }
  freeifaddrs(ifa);
}

void WifiSync::open() {
  uint32_t id = (uint32_t)time(nullptr) ^ (uint32_t)getpid() * 2654435761u;
  if (FILE* f = fopen("/dev/urandom", "rb")) {
    uint32_t r = 0;
    if (fread(&r, 1, 4, f) == 4) id ^= r;
    fclose(f);
  }
  if (!id) id = 1;
  coreInit(id);
  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) return;
  int one = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
  setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &one, sizeof one);
  struct sockaddr_in a{};
  a.sin_family = AF_INET;
  a.sin_addr.s_addr = htonl(INADDR_ANY);
  a.sin_port = htons(kPort);
  if (bind(sock, (struct sockaddr*)&a, sizeof a) != 0) {
    fprintf(stderr, "sync: bind failed: %s\n", strerror(errno));
    ::close(sock);
    sock = -1;
    return;
  }
  fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);
  refreshBroadcast();
}

void WifiSync::poll(uint64_t nowUs) {
  if (sock < 0) return;
  uint8_t b[64];
  for (;;) {
    ssize_t n = recv(sock, b, sizeof b, 0);
    if (n <= 0) break;
    handlePacket(b, (int)n, nowUs);
  }
  elect(nowUs);
  if (syncMode == SYNC_OFF) return;
  if (nowUs - lastIfUs > kIfUs) { lastIfUs = nowUs; refreshBroadcast(); }
  if (nowUs - lastSendUs < kSendUs) return;
  lastSendUs = nowUs;
  int n = makePacket(b, nowUs);
  struct sockaddr_in a{};
  a.sin_family = AF_INET;
  a.sin_port = htons(kPort);
  for (int i = 0; i < nbcast; i++) { a.sin_addr.s_addr = bcast[i]; sendto(sock, b, n, 0, (struct sockaddr*)&a, sizeof a); }
  if (!nbcast) { a.sin_addr.s_addr = htonl(INADDR_BROADCAST); sendto(sock, b, n, 0, (struct sockaddr*)&a, sizeof a); }
}

void WifiSync::close() {
  if (sock >= 0) ::close(sock);
  sock = -1;
}
