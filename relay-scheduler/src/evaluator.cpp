#include "evaluator.h"

bool windowCovers(uint32_t windowStart, uint32_t dur, uint32_t weekSecond) {
  uint32_t windowEnd = windowStart + dur; // may exceed SECONDS_PER_WEEK
  if (windowEnd <= SECONDS_PER_WEEK) {
    return weekSecond >= windowStart && weekSecond < windowEnd;
  }
  uint32_t wrappedEnd = windowEnd - SECONDS_PER_WEEK;
  return weekSecond >= windowStart || weekSecond < wrappedEnd;
}

bool isOn(const Schedule* schedules, size_t count, uint8_t channel, uint32_t weekSecond) {
  for (size_t i = 0; i < count; i++) {
    const Schedule& s = schedules[i];
    if (!s.en || s.ch != channel) continue;
    for (uint8_t d = 0; d < 7; d++) {
      if (!(s.days & (1 << d))) continue;
      uint32_t windowStart = (d * SECONDS_PER_DAY + s.start) % SECONDS_PER_WEEK;
      if (windowCovers(windowStart, s.dur, weekSecond)) return true;
    }
  }
  return false;
}
