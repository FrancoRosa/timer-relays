// Pure scheduling logic. No Arduino includes here: this header and its
// .cpp are built standalone by tests/evaluator_test.cpp with g++.
#pragma once

#include <stddef.h>
#include <stdint.h>

constexpr uint8_t MAX_SCHEDULES = 64;
constexpr uint8_t MAX_NAME_LEN = 24;

constexpr uint32_t SECONDS_PER_DAY = 86400UL;
constexpr uint32_t SECONDS_PER_WEEK = 604800UL;

struct Schedule {
  int32_t id = -1;
  char name[MAX_NAME_LEN + 1] = {0};
  uint8_t ch = 0;
  uint8_t days = 0;     // bit 0 = Sunday ... bit 6 = Saturday
  uint32_t start = 0;   // seconds from midnight, 0..86399
  uint32_t dur = 0;     // seconds, 1..86400 (24:00:00 max)
  bool en = true;
};

// True if a window of length `dur` starting at `windowStart` (both in
// seconds-since-Sunday-00:00, windowStart in [0, SECONDS_PER_WEEK)) covers
// `weekSecond`. Handles windows that wrap past the end of the week.
bool windowCovers(uint32_t windowStart, uint32_t dur, uint32_t weekSecond);

// True when any enabled schedule for `channel` covers `weekSecond`.
// Windows combine with OR. `weekSecond` must be in [0, SECONDS_PER_WEEK).
bool isOn(const Schedule* schedules, size_t count, uint8_t channel, uint32_t weekSecond);
