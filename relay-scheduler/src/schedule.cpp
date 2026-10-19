#include "schedule.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

static const char* SCHEDULES_PATH = "/schedules.json";
static const char* SCHEDULES_TMP_PATH = "/schedules.json.tmp";

static void fromJson(JsonObjectConst obj, Schedule& s) {
  s.id = obj["id"] | -1;
  const char* name = obj["name"] | "";
  strlcpy(s.name, name, sizeof(s.name));
  s.ch = obj["ch"] | 0;
  s.days = obj["days"] | 0;
  s.start = obj["start"] | 0;
  s.dur = obj["dur"] | 0;
  s.en = obj["en"] | true;
}

static void toJson(const Schedule& s, JsonObject obj) {
  obj["id"] = s.id;
  obj["name"] = s.name;
  obj["ch"] = s.ch;
  obj["days"] = s.days;
  obj["start"] = s.start;
  obj["dur"] = s.dur;
  obj["en"] = s.en;
}

bool ScheduleStore::begin() {
  count_ = 0;
  nextId_ = 1;

  if (LittleFS.exists(SCHEDULES_PATH)) {
    File f = LittleFS.open(SCHEDULES_PATH, "r");
    if (f) {
      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, f);
      f.close();
      if (!err && doc.is<JsonArray>()) {
        for (JsonObjectConst obj : doc.as<JsonArray>()) {
          if (count_ >= MAX_SCHEDULES) break;
          fromJson(obj, items_[count_]);
          if (items_[count_].id >= nextId_) nextId_ = items_[count_].id + 1;
          count_++;
        }
        return true;
      }
    }
  }

  // Missing or corrupt file: start empty and write a fresh one.
  return save();
}

int ScheduleStore::findIndex(int32_t id) const {
  for (size_t i = 0; i < count_; i++) {
    if (items_[i].id == id) return (int)i;
  }
  return -1;
}

const Schedule* ScheduleStore::find(int32_t id) const {
  int idx = findIndex(id);
  return idx < 0 ? nullptr : &items_[idx];
}

bool ScheduleStore::validate(const Schedule& in, bool isCreate, String& reason) const {
  if (strlen(in.name) > MAX_NAME_LEN) {
    reason = "name too long (max 24 characters)";
    return false;
  }
  if (in.ch >= CHANNEL_COUNT) {
    reason = "ch out of range (0-" + String(CHANNEL_COUNT - 1) + ")";
    return false;
  }
  if (in.days == 0 || in.days > 0x7F) {
    reason = "days must have at least one of bits 0-6 set";
    return false;
  }
  if (in.start > 86399) {
    reason = "start out of range (0-86399)";
    return false;
  }
  if (in.dur < 1 || in.dur > 86400) {
    reason = "dur out of range (1-86400)";
    return false;
  }
  if (isCreate && count_ >= MAX_SCHEDULES) {
    reason = "schedule limit reached (max 64)";
    return false;
  }
  return true;
}

bool ScheduleStore::create(const Schedule& in, Schedule& out, String& reason) {
  if (!validate(in, true, reason)) return false;

  out = in;
  out.id = nextId_;
  items_[count_] = out;
  count_++;

  if (!save()) {
    count_--;
    reason = "failed to save to flash";
    return false;
  }
  nextId_++;
  return true;
}

bool ScheduleStore::update(int32_t id, const Schedule& in, String& reason) {
  int idx = findIndex(id);
  if (idx < 0) {
    reason = "schedule not found";
    return false;
  }
  if (!validate(in, false, reason)) return false;

  Schedule previous = items_[idx];
  items_[idx] = in;
  items_[idx].id = id;

  if (!save()) {
    items_[idx] = previous;
    reason = "failed to save to flash";
    return false;
  }
  return true;
}

bool ScheduleStore::remove(int32_t id, String& reason) {
  int idx = findIndex(id);
  if (idx < 0) {
    reason = "schedule not found";
    return false;
  }

  Schedule removed = items_[idx];
  for (size_t i = idx; i + 1 < count_; i++) items_[i] = items_[i + 1];
  count_--;

  if (!save()) {
    for (size_t i = count_; i > (size_t)idx; i--) items_[i] = items_[i - 1];
    items_[idx] = removed;
    count_++;
    reason = "failed to save to flash";
    return false;
  }
  return true;
}

bool ScheduleStore::save() {
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (size_t i = 0; i < count_; i++) {
    toJson(items_[i], arr.add<JsonObject>());
  }

  File f = LittleFS.open(SCHEDULES_TMP_PATH, "w");
  if (!f) return false;
  bool ok = serializeJson(doc, f) > 0;
  f.close();
  if (!ok) {
    LittleFS.remove(SCHEDULES_TMP_PATH);
    return false;
  }

  LittleFS.remove(SCHEDULES_PATH);
  return LittleFS.rename(SCHEDULES_TMP_PATH, SCHEDULES_PATH);
}
