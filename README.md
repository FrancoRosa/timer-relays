# ESP32 Relay Scheduler

An 8-channel relay timer on an ESP32. Each relay follows weekly schedules
(days of week, start time, duration), runs its own Wi-Fi access point, and
serves a dark-theme web UI to manage schedules, manual overrides, and a
weekly chart of ON/OFF windows. See `esp32-relay-scheduler-plan.md` for the
full design and milestones.

## Editing the GPIO pins

Pin assignments live in one place: **`relay-scheduler/src/board_config.h`**.

```cpp
// Phase 1: ESP32-WROOM-32 + 8-channel relay module
constexpr uint8_t RELAY_PINS[CHANNEL_COUNT] = {16, 17, 18, 19, 21, 22, 23, 25};
```

`RELAY_PINS[0]` drives channel 1 (as shown in the UI), `RELAY_PINS[1]` drives
channel 2, and so on. Edit the numbers in that array to rewire which GPIO
controls which channel.

The file has a separate block for the phase 2 ESP32-S3 board
(`#if defined(CONFIG_IDF_TARGET_ESP32S3)`), with its own `RELAY_PINS` array
and the RTC's I2C pins (`RTC_SDA_PIN`, `RTC_SCL_PIN`) — edit that block
instead when building the `s3` profile. Those S3 values are still
placeholders (see the `TODO` comments) until the actual board/RTC chip is
confirmed.

On the WROOM-32, avoid GPIO 0, 2, 12, 15 (boot strapping pins), 6–11 (used
by the internal flash) and 34–39 (input-only, can't drive a relay).

If your relay module switches on a LOW signal instead of HIGH, flip
`RELAY_ACTIVE_HIGH` in the same file — nothing else needs to change.

## Project layout

```
relay-scheduler/
  relay-scheduler.ino   setup() and loop() only
  sketch.yaml           arduino-cli profiles (wroom32, s3)
  src/
    board_config.h       pin map, RELAY_ACTIVE_HIGH, channel count  <- edit pins here
    clock.h / clock.cpp   SoftClock (phase 1) / RtcClock (phase 2)
    schedule.h / .cpp     Schedule struct, ScheduleStore (LittleFS + ArduinoJson)
    evaluator.h / .cpp    pure scheduling logic, no Arduino includes
    relays.h / .cpp       RelayDriver, override modes
    settings.h / .cpp     time zone, channel names, LittleFS settings.json
    webapi.h / .cpp       access point, HTTP routes
    webui.h               the whole web UI (HTML/CSS/JS), served from PROGMEM
  tests/
    evaluator_test.cpp    host-side tests, built with g++
    cases.json            shared test cases
scripts/
  install.sh      one-time arduino-cli + ESP32 core + library setup
  compile.sh       arduino-cli compile --profile <wroom32|s3>
  upload.sh         compile then upload to a board
  onlyupload.sh     upload only (skip compile)
  simulate.py       local UI simulator, see below
```

## Building and flashing

```bash
./scripts/install.sh                      # one-time toolchain setup
./scripts/compile.sh wroom32               # or: s3
./scripts/upload.sh wroom32 /dev/ttyUSB0   # compiles, then uploads
```

Connect to the access point printed on serial boot (`RelayTimer-XXXX`, from
the board's MAC) and open `http://192.168.4.1`. The Wi-Fi password is
deterministic: `pass` followed by the same 4 hex digits as the SSID suffix
(e.g. SSID `RelayTimer-8A44` → password `pass8A44`).

## Running the host-side tests

```bash
cd relay-scheduler/tests
g++ -std=c++17 -I ../src -I <path-to-ArduinoJson>/src evaluator_test.cpp ../src/evaluator.cpp -o evaluator_test
./evaluator_test cases.json
```

## UI simulator (no hardware needed)

```bash
python3 scripts/simulate.py
```

Serves the exact page from `relay-scheduler/src/webui.h` at
`http://127.0.0.1:8000/` and fakes the device's HTTP API in memory
(schedules, overrides, channel names, a simulated clock). Edit `webui.h`
and reload the browser to see changes immediately — no compile/flash cycle.

It's a separate, simplified reimplementation of the firmware's scheduling
logic in Python (good enough for UI work), so re-check against the real
device before relying on it for anything behavior-sensitive. Its state is
in-memory only and resets when the script restarts.
