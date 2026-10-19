// ScheduleStore: load, validate and save /schedules.json on LittleFS.
// The Schedule struct itself (plain data, no Arduino deps) lives in
// evaluator.h so the Evaluator can be built and tested without Arduino.
#pragma once

#include <Arduino.h>

#include "board_config.h"
#include "evaluator.h"

class ScheduleStore {
 public:
  // Loads /schedules.json, creating an empty one if it does not exist.
  bool begin();

  size_t count() const { return count_; }
  const Schedule* data() const { return items_; }
  const Schedule* find(int32_t id) const;

  // Validates `in` against the field rules in the plan (name length, ch
  // range, days bitmask, start/dur range, and the 64-schedule cap for a
  // new record). On failure, fills `reason` with a human-readable cause.
  bool validate(const Schedule& in, bool isCreate, String& reason) const;

  // Assigns an id, appends and saves. On success fills `out`.
  bool create(const Schedule& in, Schedule& out, String& reason);

  // Replaces the record with this id (keeping its id) and saves.
  bool update(int32_t id, const Schedule& in, String& reason);

  // Removes the record with this id and saves. Returns false if not found.
  bool remove(int32_t id, String& reason);

 private:
  Schedule items_[MAX_SCHEDULES];
  size_t count_ = 0;
  int32_t nextId_ = 1;

  int findIndex(int32_t id) const;
  bool save();
};
