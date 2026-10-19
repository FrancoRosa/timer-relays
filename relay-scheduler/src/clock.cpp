#include "clock.h"

#include <Arduino.h>

#include "evaluator.h"

#if BOARD_HAS_RTC
#include <RTClib.h>
#include <Wire.h>
#endif

// Debug build option: run the soft clock at 60x real time, to soak-test a
// full week of schedules in minutes. Set via a build flag, not by default.
#ifdef DEBUG_CLOCK_60X
constexpr uint32_t CLOCK_SPEEDUP = 60;
#else
constexpr uint32_t CLOCK_SPEEDUP = 1;
#endif

void localTimeFromEpoch(time_t epoch, const char* tz, struct tm* out) {
  setenv("TZ", tz, 1);
  tzset();
  localtime_r(&epoch, out);
}

uint32_t weekSecondFromEpoch(time_t epoch, const char* tz) {
  struct tm local;
  localTimeFromEpoch(epoch, tz, &local);
  return (uint32_t)local.tm_wday * SECONDS_PER_DAY + (uint32_t)local.tm_hour * 3600UL +
         (uint32_t)local.tm_min * 60UL + (uint32_t)local.tm_sec;
}

void SoftClock::begin(uint32_t savedEpoch) {
  baseEpoch_ = savedEpoch;
  baseMillis_ = millis();
  valid_ = false;  // unverified until the browser sets the time this boot
}

uint32_t SoftClock::nowEpoch() const {
  uint32_t elapsedMs = millis() - baseMillis_;  // wraps correctly on overflow
  uint32_t elapsedS = (elapsedMs * CLOCK_SPEEDUP) / 1000UL;
  return baseEpoch_ + elapsedS;
}

void SoftClock::setEpoch(uint32_t epoch) {
  baseEpoch_ = epoch;
  baseMillis_ = millis();
  valid_ = true;
}

#if BOARD_HAS_RTC

// TODO: confirm the RTC chip for the chosen S3 board (plan: "Open items
// for later"). RTC_DS3231 is a placeholder; swap for RTC_PCF85063 /
// RTC_PCF8563 if that is what the board actually carries.
static RTC_DS3231 rtc;

bool RtcClock::begin() {
  Wire.begin(RTC_SDA_PIN, RTC_SCL_PIN);
  if (!rtc.begin()) {
    valid_ = false;
    return false;
  }
  valid_ = !rtc.lostPower();
  return true;
}

uint32_t RtcClock::nowEpoch() const { return rtc.now().unixtime(); }

void RtcClock::setEpoch(uint32_t epoch) {
  rtc.adjust(DateTime((uint32_t)epoch));
  valid_ = true;
}

#endif
