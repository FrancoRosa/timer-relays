#include "webapi.h"

#include <esp_mac.h>

#include "webui.h"

static const char* modeToString(OverrideMode m) {
  switch (m) {
    case OverrideMode::FORCE_ON:
      return "on";
    case OverrideMode::FORCE_OFF:
      return "off";
    default:
      return "auto";
  }
}

static bool stringToMode(const String& s, OverrideMode& out) {
  if (s == "auto") {
    out = OverrideMode::AUTO;
    return true;
  }
  if (s == "on") {
    out = OverrideMode::FORCE_ON;
    return true;
  }
  if (s == "off") {
    out = OverrideMode::FORCE_OFF;
    return true;
  }
  return false;
}

static void scheduleFromJson(JsonObjectConst obj, Schedule& s) {
  s.id = obj["id"] | -1;
  strlcpy(s.name, obj["name"] | "", sizeof(s.name));
  s.ch = obj["ch"] | 0;
  s.days = obj["days"] | 0;
  s.start = obj["start"] | 0;
  s.dur = obj["dur"] | 0;
  s.en = obj["en"] | true;
}

static void scheduleToJson(const Schedule& s, JsonObject obj) {
  obj["id"] = s.id;
  obj["name"] = s.name;
  obj["ch"] = s.ch;
  obj["days"] = s.days;
  obj["start"] = s.start;
  obj["dur"] = s.dur;
  obj["en"] = s.en;
}

void WebApi::begin(IClock* clock, ScheduleStore* schedules, RelayDriver* relays,
                    SettingsStore* settings) {
  clock_ = clock;
  schedules_ = schedules;
  relays_ = relays;
  settings_ = settings;

  startAccessPoint();
  setupRoutes();
  server_.begin();
}

void WebApi::handleClient() { server_.handleClient(); }

void WebApi::startAccessPoint() {
  uint8_t mac[6];
  // Read the factory eFuse MAC directly: WiFi.macAddress() /
  // softAPmacAddress() can return all zeros if read before the WiFi
  // driver has finished initializing.
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char ssid[32];
  snprintf(ssid, sizeof(ssid), "RelayTimer-%02X%02X", mac[4], mac[5]);

  // Deterministic password (same MAC bytes as the SSID), so it never
  // needs generating, persisting, or reading off the serial monitor: it
  // is always "pASS" followed by the SSID's last 4 hex digits.
  char password[16];
  snprintf(password, sizeof(password), "pASS%02X%02X", mac[4], mac[5]);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  Serial.print("AP SSID: ");
  Serial.println(ssid);
  Serial.print("AP password: ");
  Serial.println(password);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
}

void WebApi::setupRoutes() {
  server_.on("/", HTTP_GET, [this]() { handleRoot(); });
  server_.on("/api/state", HTTP_GET, [this]() { handleGetState(); });
  server_.on("/api/time", HTTP_POST, [this]() { handlePostTime(); });
  server_.on("/api/schedules", HTTP_GET, [this]() { handleGetSchedules(); });
  server_.on("/api/schedules", HTTP_POST, [this]() { handlePostSchedules(); });
  server_.onNotFound([this]() { handleNotFound(); });
}

void WebApi::handleRoot() { server_.send(200, "text/html", INDEX_HTML); }

void WebApi::handleGetState() {
  uint32_t epoch = clock_->nowEpoch();
  struct tm local;
  localTimeFromEpoch((time_t)epoch, settings_->get().tz, &local);
  char localBuf[32];
  strftime(localBuf, sizeof(localBuf), "%Y-%m-%d %H:%M:%S", &local);
  uint32_t weekSecond = (uint32_t)local.tm_wday * SECONDS_PER_DAY +
                         (uint32_t)local.tm_hour * 3600UL + (uint32_t)local.tm_min * 60UL +
                         (uint32_t)local.tm_sec;

  JsonDocument doc;
  doc["epoch"] = epoch;
  doc["valid"] = clock_->isValid();
  doc["tz"] = settings_->get().tz;
  doc["local"] = localBuf;
  doc["weekSecond"] = weekSecond;
  JsonArray channels = doc["channels"].to<JsonArray>();
  for (uint8_t ch = 0; ch < CHANNEL_COUNT; ch++) {
    JsonObject c = channels.add<JsonObject>();
    c["on"] = relays_->isOn(ch);
    c["mode"] = modeToString(relays_->getOverride(ch));
  }
  sendJson(200, doc);
}

void WebApi::handlePostTime() {
  if (!server_.hasArg("plain")) {
    sendError(400, "missing body");
    return;
  }
  JsonDocument doc;
  if (deserializeJson(doc, server_.arg("plain"))) {
    sendError(400, "invalid JSON");
    return;
  }
  if (doc["epoch"].isNull()) {
    sendError(400, "missing epoch");
    return;
  }
  clock_->setEpoch(doc["epoch"].as<uint32_t>());
  if (!doc["tz"].isNull()) {
    settings_->setTz(doc["tz"].as<const char*>());
    settings_->save();
  }
  handleGetState();
}

void WebApi::handleGetSchedules() {
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (size_t i = 0; i < schedules_->count(); i++) {
    scheduleToJson(schedules_->data()[i], arr.add<JsonObject>());
  }
  sendJson(200, doc);
}

void WebApi::handlePostSchedules() {
  if (!server_.hasArg("plain")) {
    sendError(400, "missing body");
    return;
  }
  JsonDocument doc;
  if (deserializeJson(doc, server_.arg("plain"))) {
    sendError(400, "invalid JSON");
    return;
  }
  Schedule in;
  scheduleFromJson(doc.as<JsonObjectConst>(), in);

  Schedule out;
  String reason;
  if (!schedules_->create(in, out, reason)) {
    sendError(400, reason);
    return;
  }
  JsonDocument res;
  scheduleToJson(out, res.to<JsonObject>());
  sendJson(201, res);
}

void WebApi::handleNotFound() {
  String uri = server_.uri();
  HTTPMethod method = server_.method();

  if (uri.startsWith("/api/schedules/")) {
    int32_t id = uri.substring(strlen("/api/schedules/")).toInt();

    if (method == HTTP_PUT) {
      if (!server_.hasArg("plain")) {
        sendError(400, "missing body");
        return;
      }
      JsonDocument doc;
      if (deserializeJson(doc, server_.arg("plain"))) {
        sendError(400, "invalid JSON");
        return;
      }
      Schedule in;
      scheduleFromJson(doc.as<JsonObjectConst>(), in);
      String reason;
      if (!schedules_->update(id, in, reason)) {
        sendError(400, reason);
        return;
      }
      JsonDocument res;
      scheduleToJson(*schedules_->find(id), res.to<JsonObject>());
      sendJson(200, res);
      return;
    }

    if (method == HTTP_DELETE) {
      String reason;
      if (!schedules_->remove(id, reason)) {
        sendError(404, reason);
        return;
      }
      server_.send(204, "text/plain", "");
      return;
    }
  }

  if (uri.startsWith("/api/relays/") && uri.endsWith("/mode") && method == HTTP_POST) {
    String middle = uri.substring(strlen("/api/relays/"), uri.length() - strlen("/mode"));
    int ch = middle.toInt();
    if (ch < 0 || ch >= CHANNEL_COUNT) {
      sendError(404, "channel out of range");
      return;
    }
    if (!server_.hasArg("plain")) {
      sendError(400, "missing body");
      return;
    }
    JsonDocument doc;
    if (deserializeJson(doc, server_.arg("plain"))) {
      sendError(400, "invalid JSON");
      return;
    }
    OverrideMode mode;
    if (!stringToMode(doc["mode"].as<String>(), mode)) {
      sendError(400, "mode must be auto, on or off");
      return;
    }
    relays_->setOverride((uint8_t)ch, mode);
    handleGetState();
    return;
  }

  server_.send(404, "text/plain", "not found");
}

void WebApi::sendJson(int code, const JsonDocument& doc) {
  String out;
  serializeJson(doc, out);
  server_.send(code, "application/json", out);
}

void WebApi::sendError(int code, const String& reason) {
  JsonDocument doc;
  doc["error"] = reason;
  sendJson(code, doc);
}
