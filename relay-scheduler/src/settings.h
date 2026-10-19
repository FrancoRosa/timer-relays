// Persisted settings: time zone and the soft clock's last known epoch
// (phase 1 only). Stored as /settings.json on LittleFS.
//
// The AP password is not stored here: it is derived deterministically
// from the chip's MAC in webapi.cpp (same bytes used for the SSID), so it
// never needs generating or persisting.
#pragma once

#include <stddef.h>
#include <stdint.h>

constexpr size_t TZ_MAX_LEN = 31;

struct Settings {
  char tz[TZ_MAX_LEN + 1] = "<-05>5";  // POSIX TZ string; default Lima, no DST
  uint32_t softClockEpoch = 0;         // last epoch SoftClock saved before power-off
};

class SettingsStore {
 public:
  // Loads /settings.json, or writes defaults if it does not exist.
  bool begin();

  const Settings& get() const { return settings_; }

  void setTz(const char* tz);
  void setSoftClockEpoch(uint32_t epoch);

  bool save();

 private:
  Settings settings_;
};
