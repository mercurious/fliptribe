// fliptribe: pattern persistence. One JSON file per pattern slot plus the shipped demo set.
#pragma once
#include "params.h"
#include "json.h"
#include <string>

namespace store {
std::string dataDir();                       // user-writable: $FLIPTRIBE_DATA or $HOME/.config/fliptribe
std::string assetDir();                      // $FLIPTRIBE_ASSETS or <exe dir>/assets
std::string patternDir();                    // shipped patterns: <assetDir>/../patterns
std::string slotName(int slot);              // 0 -> "A01", 255 -> "D64"
json::Value toJson(const Pattern& p);
bool fromJson(const json::Value& v, Pattern& p);
bool savePattern(int slot, const Pattern& p);
bool loadPattern(int slot, Pattern& p);      // user slot first, then shipped pattern, else false
void seedFromShipped();                      // copy shipped demo patterns into empty user slots on first run
bool saveSettings(const json::Value& v);
bool loadSettings(json::Value& v);
}
