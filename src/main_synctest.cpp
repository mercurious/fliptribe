// fliptribe: host-side Wi-Fi sync interop test. Runs the real engine headless in real time, with the sync module on
// UDP 5150; run it next to another one, or next to HI5's / Jaffedrone's synctest, on the same machine or LAN.
//   fliptribe_synctest LEAD|FOLLOW|AUTO BPM SECONDS [16TH|32ND|TRIPLET [SWING%]]
// The optional scale and swing exercise the mapping between pattern steps and the 16th notes on the wire.
// Prints how the link stood each second on stderr and a summary line in HI5's format at the end.
#include "engine.h"
#include "sync.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <vector>

int main(int argc, char** argv) {
  const char* name = argc > 1 ? argv[1] : "AUTO";
  int mode = !strcmp(name, "LEAD") ? SYNC_LEAD : !strcmp(name, "FOLLOW") ? SYNC_FOLLOW : SYNC_AUTO;
  float bpm = argc > 2 ? (float)atof(argv[2]) : 120.0f;
  int secs = argc > 3 ? atoi(argv[3]) : 5;
  int beat = argc > 4 ? (!strcmp(argv[4], "32ND") ? 1 : !strcmp(argv[4], "TRIPLET") ? 2 : 0) : 0;
  int swing = argc > 5 ? atoi(argv[5]) - 50 : 0;
  float rate = 39062.5f;

  Engine engine;
  engine.init(rate);
  Command t; t.type = Command::SetTempo10; t.a = (int)(bpm * 10.0f + 0.5f); engine.send(t);
  Command g; g.type = Command::SetGlobal; g.a = G_BEAT; g.b = beat; engine.send(g);
  g.a = G_SWING; g.b = swing; engine.send(g);
  if (mode == SYNC_LEAD) { Command p; p.type = Command::Play; engine.send(p); }
  WifiSync sync(engine);
  sync.setMode(mode);
  sync.open();
  if (!sync.online()) { fprintf(stderr, "fliptribe synctest: no socket\n"); return 1; }

  // render exactly as many frames as wall time asks for, like an audio callback would
  std::vector<float> buf(256 * 2);
  int64_t t0 = monoMicros(), done = 0;
  int lastSec = 0;
  while (monoMicros() - t0 < (int64_t)secs * 1000000) {
    int64_t now = monoMicros();
    int64_t want = (int64_t)((now - t0) * 1e-6 * rate);
    while (done < want) { int n = (int)std::min<int64_t>(256, want - done); engine.render(buf.data(), n); done += n; }
    sync.poll((uint64_t)monoMicros());
    int sec = (int)((now - t0) / 1000000);
    if (sec != lastSec) {
      lastSec = sec;
      const SyncStatus& s = sync.status();
      SyncClock k = engine.syncClock(monoMicros());
      fprintf(stderr, "  %2ds  bpm %.1f playing %d peers %d leading %d following %d offset %+.1f ms pos %.2f\n", sec, k.tempo10 / 10.0, k.playing, s.peers, s.leading, s.haveLeader, s.offsetMs, k.pos);
    }
    usleep(5000);
  }
  const SyncStatus& s = sync.status();
  SyncClock k = engine.syncClock(monoMicros());
  printf("fliptribe %s: bpm %.1f playing %d peers %d leading %d following %d offset %+.1f ms pos %.2f\n", name, k.tempo10 / 10.0, k.playing, s.peers, s.leading, s.haveLeader, s.offsetMs, k.pos);
  sync.close();
  return 0;
}
