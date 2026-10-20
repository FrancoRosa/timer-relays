// Persisted settings: time zone, per-channel friendly names, and the
// soft clock's last known epoch (phase 1 only). Stored as /settings.json
// on LittleFS.
//
// The AP password is not stored here: it is derived deterministically
// from the chip's MAC in webapi.cpp (same bytes used for the SSID), so it
// never needs generating or persisting.
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "board_config.h"

constexpr size_t TZ_MAX_LEN = 31;
constexpr size_t CHANNEL_NAME_MAX_LEN = 24;

struct Settings {
  char tz[TZ_MAX_LEN + 1] = "<-05>5";  // POSIX TZ string; default Lima, no DST
  char channelNames[CHANNEL_COUNT][CHANNEL_NAME_MAX_LEN + 1] = {};  // optional, empty by default
  uint32_t softClockEpoch = 0;                                     // last epoch SoftClock saved before power-off
};

class SettingsStore {
 public:
  // Loads /settings.json, or writes defaults if it does not exist.
  bool begin();

  const Settings& get() const { return settings_; }

  void setTz(const char* tz);
  void setSoftClockEpoch(uint32_t epoch);

  // Sets the friendly name for channel `ch`. Truncates to
  // CHANNEL_NAME_MAX_LEN; an empty string clears it back to "Channel N".
  void setChannelName(uint8_t ch, const char* name);

  bool save();

 private:
  Settings settings_;
};
