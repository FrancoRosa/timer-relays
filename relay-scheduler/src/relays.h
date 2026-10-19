// Relay outputs and per-channel manual override.
#pragma once

#include <stdint.h>

#include "board_config.h"
#include "evaluator.h"

enum class OverrideMode : uint8_t { AUTO = 0, FORCE_ON = 1, FORCE_OFF = 2 };

class RelayDriver {
 public:
  // Writes the OFF level to every relay pin before enabling output, so the
  // relays cannot glitch on during boot.
  void begin();

  // Core rule for one channel, applied once per tick:
  //   FORCE_ON  -> ON
  //   FORCE_OFF -> OFF
  //   AUTO      -> ON iff clockValid && isOn(schedules, ch, weekSecond)
  // Writes the relay and remembers the resulting ON/OFF state.
  void tick(uint8_t ch, bool clockValid, const Schedule* schedules, size_t count,
            uint32_t weekSecond);

  void setOverride(uint8_t ch, OverrideMode mode);
  OverrideMode getOverride(uint8_t ch) const;
  bool isOn(uint8_t ch) const;

 private:
  OverrideMode overrides_[CHANNEL_COUNT] = {};
  bool state_[CHANNEL_COUNT] = {};
};

// Sets digitalWrite/pinMode for one relay pin according to RELAY_ACTIVE_HIGH.
// This is the only place that touches the relay GPIOs.
void relayWrite(uint8_t ch, bool on);
