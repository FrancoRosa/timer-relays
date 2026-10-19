// Access point + HTTP API. Serves the PROGMEM page from webui.h and the
// JSON routes described in the plan's "Web UI and API" section.
#pragma once

#include <ArduinoJson.h>
#include <WebServer.h>
#include <WiFi.h>

#include "clock.h"
#include "relays.h"
#include "schedule.h"
#include "settings.h"

class WebApi {
 public:
  void begin(IClock* clock, ScheduleStore* schedules, RelayDriver* relays,
             SettingsStore* settings);
  void handleClient();  // call every loop(), never blocks

 private:
  WebServer server_{80};
  IClock* clock_ = nullptr;
  ScheduleStore* schedules_ = nullptr;
  RelayDriver* relays_ = nullptr;
  SettingsStore* settings_ = nullptr;

  void startAccessPoint();
  void setupRoutes();

  void handleRoot();
  void handleGetState();
  void handlePostTime();
  void handleGetSchedules();
  void handlePostSchedules();
  void handleNotFound();  // dispatches PUT/DELETE /api/schedules/<id> and
                           // POST /api/relays/<ch>/mode, since WebServer has
                           // no path-parameter routing of its own

  void sendJson(int code, const JsonDocument& doc);
  void sendError(int code, const String& reason);
};
