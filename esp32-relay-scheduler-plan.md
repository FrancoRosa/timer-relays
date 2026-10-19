# ESP32 Relay Scheduler: Implementation Plan

Plan written for Claude Code. Work through the milestones in order and stop at each "Done when" check.

## Goal

An 8-channel relay timer on an ESP32. Each relay follows weekly schedules (days of week, start time, duration as hh:mm:ss). The board runs its own Wi-Fi access point and serves a dark-theme web UI to list, create, edit and delete schedules, set a manual override per channel, and view a weekly chart of when each relay is ON or OFF.

## Decisions already made

- Relay outputs are **active-high**, isolated in one function (`relayWrite`) and one constant (`RELAY_ACTIVE_HIGH`) so the polarity can be flipped in one place if the hardware needs it.
- A schedule runs **at most 24 hours** (duration 1 to 86400 seconds).
- **Manual override** per channel: AUTO, FORCE ON, FORCE OFF.
- **No over-the-air updates** for now.
- UI uses a **dark theme**.
- Phase 1: ESP32-WROOM-32 with a software ("dummy") clock. Phase 2: ESP32-S3 board with a hardware RTC. Only the clock module and pin map change between phases.
- Default time zone: Lima, POSIX string `<-05>5` (no daylight saving). Stored as a setting.

## Hardware

| | Phase 1 | Phase 2 |
| --- | --- | --- |
| Board | ESP32-WROOM-32 dev board + 8-channel relay module | ESP32-S3 relay board (model TBD) |
| FQBN | `esp32:esp32:esp32` | `esp32:esp32:esp32s3` |
| Clock | `SoftClock`, set from the browser | `RtcClock` (I2C RTC chip, e.g. PCF85063, PCF8563 or DS3231; confirm the chip) |
| Relay pins | 16, 17, 18, 19, 21, 22, 23, 25 | From the board documentation |

Avoid GPIO 0, 2, 12, 15 (boot strapping), 6 to 11 (flash) and 34 to 39 (input only) for relays on the WROOM-32.

The ESP32-S3 chip's internal RTC is not a battery-backed calendar, so phase 2 needs the board's external RTC chip.

Pin maps live in `src/board_config.h`, selected with `#if defined(CONFIG_IDF_TARGET_ESP32S3)` or a `BOARD_*` define set per profile.

## Toolchain (arduino-cli)

One-time setup:

```bash
arduino-cli config init
arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32
arduino-cli lib install ArduinoJson
arduino-cli lib install RTClib        # phase 2 only
```

Daily loop (port is `COMx` on Windows, `/dev/ttyUSB0` or `/dev/ttyACM0` on Linux):

```bash
arduino-cli board list
arduino-cli compile --profile wroom32 .
arduino-cli upload  --profile wroom32 -p /dev/ttyUSB0 .
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

`sketch.yaml` pins versions. After the first clean build, run `arduino-cli core list` and `arduino-cli lib list` and replace the placeholders with the real versions.

```yaml
profiles:
  wroom32:
    fqbn: esp32:esp32:esp32
    platforms:
      - platform: esp32:esp32 (<core version>)
        platform_index_url: https://espressif.github.io/arduino-esp32/package_esp32_index.json
    libraries:
      - ArduinoJson (<version>)
  s3:
    fqbn: esp32:esp32:esp32s3
    platforms:
      - platform: esp32:esp32 (<core version>)
        platform_index_url: https://espressif.github.io/arduino-esp32/package_esp32_index.json
    libraries:
      - ArduinoJson (<version>)
      - RTClib (<version>)
```

Dependencies: use the built-in `WiFi`, `WebServer` and `LittleFS` libraries from the ESP32 core. The only external libraries are ArduinoJson and, in phase 2, RTClib.

The web page is embedded in the firmware as a `PROGMEM` string (`src/webui.h`), because arduino-cli has no built-in step to upload a LittleFS data folder. LittleFS stores only `schedules.json` and `settings.json`, formatted automatically on first boot with `LittleFS.begin(true)`.

## Project layout

```
relay-scheduler/
  relay-scheduler.ino      setup() and loop() only
  sketch.yaml
  src/
    board_config.h         pin map, RELAY_ACTIVE_HIGH, channel count
    clock.h / clock.cpp    IClock, SoftClock, RtcClock
    schedule.h / .cpp      Schedule struct, ScheduleStore (LittleFS + ArduinoJson)
    evaluator.h / .cpp     pure logic, no Arduino includes
    relays.h / .cpp        RelayDriver, override modes
    webapi.h / .cpp        access point, routes
    webui.h                PROGMEM page (HTML, CSS, JS)
  tests/
    evaluator_test.cpp     host-side tests, built with g++
    cases.json             shared cases for firmware and browser chart
```

## Firmware design

### Core rule

Relay state is recomputed from scratch once per second from the current time, the schedules and the override mode. There is no stored "timer running" state, so a reboot, a time change or an edited schedule corrects itself on the next tick.

For each channel:

1. If override is FORCE ON, the relay is ON.
2. If override is FORCE OFF, the relay is OFF.
3. If override is AUTO, the relay is ON when any enabled schedule for that channel covers the current moment (windows combine with OR).

Override modes are held in RAM only and reset to AUTO on reboot. This is a safety choice; make it persistent only if asked.

### Time math

Work in seconds since Sunday 00:00 (`weekSecond`, 0 to 604799). A schedule covers a day `d` if bit `d` of `days` is set (bit 0 = Sunday). For each selected day, the window is `[d*86400 + start, d*86400 + start + dur)`, taken modulo 604800 so that windows crossing midnight or the Saturday-to-Sunday boundary wrap correctly. Since `dur` is at most 86400, a window never overlaps itself.

### Relay polarity (active-high, in one place)

```cpp
// board_config.h
constexpr bool RELAY_ACTIVE_HIGH = true;   // set false if the relay module is active-low

// relays.cpp
void relayWrite(uint8_t ch, bool on) {
  const bool level = RELAY_ACTIVE_HIGH ? on : !on;
  digitalWrite(RELAY_PINS[ch], level ? HIGH : LOW);
}
```

At boot, write the OFF level to every pin with `digitalWrite` before calling `pinMode(pin, OUTPUT)`, so the relays do not glitch on during startup. No other code touches the relay GPIOs.

### Modules

| Module | Job |
| --- | --- |
| `IClock` | Interface: `nowEpoch()`, `setEpoch()`, `isValid()`. |
| `SoftClock` (phase 1) | Epoch from `millis()` plus an offset; set from the browser; saved to LittleFS every 10 minutes; after reboot restored from the saved value and flagged unverified. Debug build option: run at 60x speed. |
| `RtcClock` (phase 2) | Reads the I2C RTC into system time at boot; writes it back when the user sets the time. |
| `ScheduleStore` | Load, validate and save `schedules.json`. Write to a temp file then rename, so a power cut cannot corrupt it. |
| `Evaluator` | Pure function `isOn(schedules, channel, weekSecond)`. No Arduino calls, so it can be tested on a PC. |
| `RelayDriver` | Channel to GPIO map, `relayWrite`, override modes, applies the core rule each tick. |
| `WebApi` | Access point, `WebServer` routes, JSON in and out. |

When the clock is not valid (never set), treat AUTO channels as OFF; overrides still work.

`loop()` calls only `server.handleClient()` and the one-second tick. Nothing blocks.

## Schedule data model

Stored as a JSON array in `/schedules.json`. Several schedules may target the same channel.

| Field | Type | Rule |
| --- | --- | --- |
| `id` | integer | Assigned by the board, unique while the record exists. |
| `name` | string | Up to 24 characters, optional. |
| `ch` | integer | 0 to 7. |
| `days` | integer bitmask | Bit 0 = Sunday ... bit 6 = Saturday; at least one bit set. |
| `start` | seconds from midnight | 0 to 86399, entered as hh:mm:ss. |
| `dur` | seconds | 1 to 86400 (maximum 24:00:00), entered as hh:mm:ss. |
| `en` | boolean | Disabled schedules are kept but ignored. |

Maximum 64 schedules. The board rejects an out-of-range save with a reason string, and the browser validates the same rules.

```json
{ "id": 7, "name": "Irrigation", "ch": 2, "days": 42, "start": 21600, "dur": 1800, "en": true }
```

Channel 2 on at 06:00:00 for 00:30:00 on Monday, Wednesday and Friday (`days` = 2 + 8 + 32).

## Web UI and API

Access point: SSID `RelayTimer-XXXX` (last bytes of the MAC), WPA2 password from `settings.json` (default set at first boot and printed on the serial monitor). UI at `http://192.168.4.1`. The page is one HTML file with inline CSS and JavaScript and no external resources, because the phone has no internet while connected to the board.

| Method and path | Purpose |
| --- | --- |
| `GET /api/state` | Current time, clock status (valid or unverified), per channel: ON or OFF and override mode. |
| `POST /api/time` | Set the clock: epoch seconds and time zone string. |
| `GET /api/schedules` | All stored schedules. |
| `POST /api/schedules` | Create; returns the record with its `id`. |
| `PUT /api/schedules/<id>` | Modify. |
| `DELETE /api/schedules/<id>` | Remove. |
| `POST /api/relays/<ch>/mode` | Body `{"mode":"auto"|"on"|"off"}`; sets the override. |

### Screens (tabs on one page)

1. **Status:** clock and a banner when the time is unverified, with a "Set time from this device" button; 8 relay cards, each showing ON or OFF and a three-way AUTO / ON / OFF override control.
2. **Schedules:** list grouped by channel; editor with channel, seven day checkboxes, start time, and duration as separate hours, minutes and seconds fields (hours 0 to 24; total must be 1 s to 24:00:00); enable toggle; delete.
3. **Week:** the weekly chart.

The page polls `/api/state` every 2 seconds.

### Dark theme

- Dark background (about `#0f1115`), slightly lighter cards (about `#181b22`), light text (about `#e6e8ee`), muted secondary text.
- One accent color for ON states, bars and primary buttons (for example a green or teal). Use a distinct color for FORCE ON / FORCE OFF overrides so a forced relay is obvious.
- Define colors as CSS variables on `:root`; add `<meta name="color-scheme" content="dark">`.
- Large touch targets, since it is used from a phone. Keep contrast readable (text at least 4.5:1).

## Weekly chart

Drawn in the browser as inline SVG from the schedules JSON (no chart library, no work for the board).

- 8 rows (channels 1 to 8) by 7 columns (Sunday to Saturday); each column spans 00:00 to 24:00 with an hour grid.
- Filled bars mean ON, empty space means OFF.
- A window crossing midnight is split into two bars, the second at 00:00 of the next day; Saturday night wraps to Sunday.
- Overlapping schedules on one channel merge into one bar.
- A vertical "now" line, updated on each `/api/state` poll.
- Tapping a bar opens that schedule in the editor. Disabled schedules are drawn as hatched outlines.
- The chart shows the planned schedule. Rows for channels currently under an override show a small badge so the chart is not mistaken for the real relay state.

The browser's wrap-around logic must produce the same bars as the firmware `Evaluator`; `tests/cases.json` holds shared cases that both sides are checked against.

## Milestones

Do these in order. Stop and verify each "Done when" before continuing.

1. **Toolchain.** Install arduino-cli, the ESP32 core and ArduinoJson; create `sketch.yaml`; upload a blink sketch. Done when `compile` and `upload` work from the terminal on the WROOM-32.
2. **RelayDriver.** Pin map, `relayWrite` with `RELAY_ACTIVE_HIGH`, safe boot state. Done when each of the 8 relays clicks in turn and all stay OFF during boot and reset.
3. **Evaluator and tests.** Pure logic plus `tests/evaluator_test.cpp` built with `g++`. Done when tests pass for midnight wrap, Saturday-to-Sunday wrap, overlapping schedules, a 24:00:00 duration and a disabled schedule.
4. **SoftClock and `/api/time`.** Done when time set from a phone survives a page reload and a reboot shows "unverified".
5. **ScheduleStore and schedule API.** Done when create, edit and delete persist across a power cycle, and invalid input is rejected with a reason.
6. **Access point, Status tab, override.** Done when a phone connects to the AP and AUTO / ON / OFF overrides drive the relay and show the correct color in the dark UI.
7. **Schedules tab.** Done when a schedule created in the UI switches the relay at the right second.
8. **Weekly chart.** Done when the chart matches `tests/cases.json` and the now line moves.
9. **Soak test.** Run with the 60x debug clock, then in real time. Done when a full week runs with no missed switch events.
10. **Phase 2.** Add `RtcClock`, the S3 pin map and the `s3` profile. Done when the time survives a power-off of several minutes and the same schedules behave identically.

## Risks and notes

- **Clock drift in phase 1:** a crystal-based soft clock drifts a few seconds per day; re-set the time from the phone now and then. A DS3231-class RTC is within a few seconds per year.
- **Mains safety:** if the relays switch mains loads, the wiring needs fuses, an enclosure and isolation by a qualified person. Firmware cannot make that safe.
- **Access point security:** anyone with the Wi-Fi password can change schedules. Use a strong password; add a UI PIN later if needed.
- **Flash wear:** write `schedules.json` only on edits and the soft-clock time at most every 10 minutes.
- **Out of scope for now:** over-the-air updates, internet or NTP sync, persistent overrides.

## Open items for later

- Exact ESP32-S3 board model and RTC chip (decides the pin map and I2C pins).
- Whether overrides should expire after a set time instead of lasting until changed.
