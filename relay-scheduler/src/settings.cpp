#include "settings.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

static const char* SETTINGS_PATH = "/settings.json";
static const char* SETTINGS_TMP_PATH = "/settings.json.tmp";

bool SettingsStore::begin() {
  if (LittleFS.exists(SETTINGS_PATH)) {
    File f = LittleFS.open(SETTINGS_PATH, "r");
    if (f) {
      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, f);
      f.close();
      if (!err) {
        strlcpy(settings_.tz, doc["tz"] | "<-05>5", sizeof(settings_.tz));
        settings_.softClockEpoch = doc["softClockEpoch"] | 0;
        JsonArrayConst names = doc["chNames"].as<JsonArrayConst>();
        for (uint8_t ch = 0; ch < CHANNEL_COUNT; ch++) {
          strlcpy(settings_.channelNames[ch], names[ch] | "", sizeof(settings_.channelNames[ch]));
        }
        return true;
      }
    }
  }

  // First boot, or a corrupt/missing file: write the defaults.
  return save();
}

void SettingsStore::setTz(const char* tz) { strlcpy(settings_.tz, tz, sizeof(settings_.tz)); }

void SettingsStore::setSoftClockEpoch(uint32_t epoch) { settings_.softClockEpoch = epoch; }

void SettingsStore::setChannelName(uint8_t ch, const char* name) {
  if (ch >= CHANNEL_COUNT) return;
  strlcpy(settings_.channelNames[ch], name, sizeof(settings_.channelNames[ch]));
}

bool SettingsStore::save() {
  JsonDocument doc;
  doc["tz"] = settings_.tz;
  doc["softClockEpoch"] = settings_.softClockEpoch;
  JsonArray names = doc["chNames"].to<JsonArray>();
  for (uint8_t ch = 0; ch < CHANNEL_COUNT; ch++) {
    names.add(settings_.channelNames[ch]);
  }

  File f = LittleFS.open(SETTINGS_TMP_PATH, "w");
  if (!f) return false;
  bool ok = serializeJson(doc, f) > 0;
  f.close();
  if (!ok) {
    LittleFS.remove(SETTINGS_TMP_PATH);
    return false;
  }

  LittleFS.remove(SETTINGS_PATH);
  return LittleFS.rename(SETTINGS_TMP_PATH, SETTINGS_PATH);
}
