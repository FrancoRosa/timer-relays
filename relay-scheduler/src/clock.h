// IClock interface plus the phase 1 (SoftClock) and phase 2 (RtcClock)
// implementations. Only this file's RtcClock and the board_config.h pin
// map change between phases.
#pragma once

#include <stdint.h>
#include <time.h>

#include "board_config.h"

// Applies the POSIX TZ string and fills `out` with the local broken-down
// time for `epoch`.
void localTimeFromEpoch(time_t epoch, const char* tz, struct tm* out);

// Converts an epoch + POSIX TZ string into seconds-since-Sunday-00:00
// (bit/day convention: 0 = Sunday), for the Evaluator.
uint32_t weekSecondFromEpoch(time_t epoch, const char* tz);

class IClock {
 public:
  virtual ~IClock() = default;
  virtual uint32_t nowEpoch() const = 0;
  virtual void setEpoch(uint32_t epoch) = 0;
  // False until the clock has been set (this boot) or confirmed by hardware.
  virtual bool isValid() const = 0;
};

// Phase 1: epoch derived from millis() plus an offset. Set from the
// browser. The caller is responsible for periodically calling
// epochForSaving() (every ~10 minutes) and persisting it, and for passing
// the last saved value to begin() after a reboot.
class SoftClock : public IClock {
 public:
  void begin(uint32_t savedEpoch);
  uint32_t nowEpoch() const override;
  void setEpoch(uint32_t epoch) override;
  bool isValid() const override { return valid_; }

 private:
  uint32_t baseEpoch_ = 0;
  uint32_t baseMillis_ = 0;
  bool valid_ = false;
};

#if BOARD_HAS_RTC

// Phase 2: backed by an I2C RTC chip (read at boot, written on setEpoch).
class RtcClock : public IClock {
 public:
  bool begin();
  uint32_t nowEpoch() const override;
  void setEpoch(uint32_t epoch) override;
  bool isValid() const override { return valid_; }

 private:
  bool valid_ = false;
};

#endif
