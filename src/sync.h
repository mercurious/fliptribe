// fliptribe: Wi-Fi sync. Speaks the "HI5S" v1 protocol of the sibling grooveboxes (HI5, Jaffedrone, Rap Machine),
// so a mixed group elects one leader and everyone follows it. Each unit broadcasts tempo, position and transport on
// UDP 5150 ten times a second; the unit set to LEAD (else the lowest id on AUTO) leads, and followers lock their
// phase by trimming playback speed. Port of HI5's src/sync.c; keep the two in step.
//
// Packet, 32 bytes little-endian: "HI5S", version 1, playing u8, sender mode u8 (1 AUTO, 2 LEAD, 3 FOLLOW),
// reserved u8, id u32 (random, non-zero), bpm f32, position f64 in 16th notes since play started (unwrapped;
// receivers use it mod 16, a follower starting up takes it whole), sender monotonic time u64 in us (unused).
#pragma once
#include <cstdint>
#include <string>

class Engine;

enum SyncMode : uint8_t { SYNC_OFF, SYNC_AUTO, SYNC_LEAD, SYNC_FOLLOW, NUM_SYNC_MODES };   // OFF neither sends nor listens
static const char* const kSyncModeNames[NUM_SYNC_MODES] = {"OFF", "AUTO", "LEAD", "FOLLOW"};
int syncModeFromName(const std::string& s, int fallback);

struct SyncStatus {
  uint32_t id = 0;
  int peers = 0;              // other units heard in the last 3 s
  bool leading = false;       // this unit is the leader
  bool haveLeader = false;    // following someone
  uint32_t leaderId = 0;
  float offsetMs = 0.0f;      // follower: how far we trail the leader (+ = behind)
};

class WifiSync {
public:
  static constexpr int kPort = 5150, kPacketLen = 32;
  explicit WifiSync(Engine& e) : eng(e) {}
  ~WifiSync() { close(); }
  void open();                            // picks an id and binds the socket
  void poll(uint64_t nowUs);              // main thread, every frame (monoMicros())
  void close();
  bool online() const { return sock >= 0; }
  int mode() const { return syncMode; }
  void setMode(int m) { syncMode = m >= 0 && m < NUM_SYNC_MODES ? m : SYNC_OFF; }
  const SyncStatus& status() const { return st; }

  // protocol core, no sockets
  void coreInit(uint32_t id);
  int makePacket(uint8_t* buf, uint64_t nowUs);
  void handlePacket(const uint8_t* buf, int n, uint64_t nowUs);
  void elect(uint64_t nowUs);

private:
  static constexpr int kPeers = 8, kErrs = 8;
  struct Peer { uint32_t id; uint64_t lastUs; int playing, role; float bpm; double pos; };

  Engine& eng;
  int syncMode = SYNC_OFF;
  SyncStatus st;
  Peer peers[kPeers] = {};
  int sock = -1;
  uint64_t lastSendUs = 0, lastIfUs = 0;
  uint32_t bcast[8] = {};     // interface broadcast addresses, network order
  int nbcast = 0;
  double errs[kErrs] = {};
  int nerrs = 0, erri = 0;
  double sentRate = 1.0;

  Peer* peerSlot(uint32_t id, uint64_t nowUs);
  double medianErr() const;
  void follow(const Peer& p, uint64_t nowUs);
  void setRate(double r);
  void refreshBroadcast();
};
