#include "relays.h"

#include <Arduino.h>

void relayWrite(uint8_t ch, bool on) {
  const bool level = RELAY_ACTIVE_HIGH ? on : !on;
  digitalWrite(RELAY_PINS[ch], level ? HIGH : LOW);
}

void RelayDriver::begin() {
  for (uint8_t ch = 0; ch < CHANNEL_COUNT; ch++) {
    const uint8_t pin = RELAY_PINS[ch];
    const bool offLevel = RELAY_ACTIVE_HIGH ? LOW : HIGH;
    digitalWrite(pin, offLevel);  // set the level before the pin is an output
    pinMode(pin, OUTPUT);
    digitalWrite(pin, offLevel);
    overrides_[ch] = OverrideMode::AUTO;
    state_[ch] = false;
  }
}

void RelayDriver::tick(uint8_t ch, bool clockValid, const Schedule* schedules, size_t count,
                        uint32_t weekSecond) {
  bool on;
  switch (overrides_[ch]) {
    case OverrideMode::FORCE_ON:
      on = true;
      break;
    case OverrideMode::FORCE_OFF:
      on = false;
      break;
    default:  // AUTO
      on = clockValid && ::isOn(schedules, count, ch, weekSecond);
      break;
  }
  state_[ch] = on;
  relayWrite(ch, on);
}

void RelayDriver::setOverride(uint8_t ch, OverrideMode mode) { overrides_[ch] = mode; }

OverrideMode RelayDriver::getOverride(uint8_t ch) const { return overrides_[ch]; }

bool RelayDriver::isOn(uint8_t ch) const { return state_[ch]; }
