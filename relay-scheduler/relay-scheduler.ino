// setup() and loop() only; everything else lives under src/.
#include <LittleFS.h>

#include "src/board_config.h"
#include "src/clock.h"
#include "src/relays.h"
#include "src/schedule.h"
#include "src/settings.h"
#include "src/webapi.h"

#if BOARD_HAS_RTC
RtcClock relayClock;
#else
SoftClock relayClock;
#endif

SettingsStore settingsStore;
ScheduleStore scheduleStore;
RelayDriver relayDriver;
WebApi webApi;

uint32_t lastTickMillis = 0;
uint32_t lastSaveMillis = 0;
constexpr uint32_t TICK_INTERVAL_MS = 1000;
constexpr uint32_t SAVE_INTERVAL_MS = 10UL * 60UL * 1000UL;  // 10 minutes

void setup() {
  Serial.begin(115200);

  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
  }

  settingsStore.begin();
  scheduleStore.begin();
  relayDriver.begin();

#if BOARD_HAS_RTC
  relayClock.begin();
#else
  relayClock.begin(settingsStore.get().softClockEpoch);
#endif

  webApi.begin(&relayClock, &scheduleStore, &relayDriver, &settingsStore);

  Serial.println("Relay scheduler ready.");
}

void loop() {
  webApi.handleClient();

  uint32_t now = millis();

  if (now - lastTickMillis >= TICK_INTERVAL_MS) {
    lastTickMillis = now;

    uint32_t epoch = relayClock.nowEpoch();
    uint32_t weekSecond = weekSecondFromEpoch((time_t)epoch, settingsStore.get().tz);
    bool valid = relayClock.isValid();

    for (uint8_t ch = 0; ch < CHANNEL_COUNT; ch++) {
      relayDriver.tick(ch, valid, scheduleStore.data(), scheduleStore.count(), weekSecond);
    }
  }

#if !BOARD_HAS_RTC
  if (now - lastSaveMillis >= SAVE_INTERVAL_MS) {
    lastSaveMillis = now;
    settingsStore.setSoftClockEpoch(relayClock.nowEpoch());
    settingsStore.save();
  }
#endif
}
