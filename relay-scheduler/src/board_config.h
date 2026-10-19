// Pin map and board-specific constants.
//
// Only this file (and the clock module selection in relay-scheduler.ino)
// changes between phase 1 (ESP32-WROOM-32) and phase 2 (ESP32-S3).
#pragma once

#include <Arduino.h>
#include <stdint.h>

// Set false if the relay module is active-low.
constexpr bool RELAY_ACTIVE_HIGH = true;

#if defined(CONFIG_IDF_TARGET_ESP32S3)

// ---- Phase 2: ESP32-S3 relay board -------------------------------------
// TODO: confirm against the actual board documentation once the model is
// known (see "Open items for later" in the plan).
constexpr uint8_t CHANNEL_COUNT = 8;
constexpr uint8_t RELAY_PINS[CHANNEL_COUNT] = {1, 2, 3, 4, 5, 6, 7, 8};

#define BOARD_HAS_RTC 1
// TODO: confirm RTC chip and I2C pins (SDA/SCL) for the chosen S3 board.
constexpr uint8_t RTC_SDA_PIN = 8;
constexpr uint8_t RTC_SCL_PIN = 9;

#else

// ---- Phase 1: ESP32-WROOM-32 + 8-channel relay module ------------------
// Avoids GPIO 0, 2, 12, 15 (boot strapping), 6-11 (flash) and 34-39
// (input only).
constexpr uint8_t CHANNEL_COUNT = 8;
constexpr uint8_t RELAY_PINS[CHANNEL_COUNT] = {16, 17, 18, 19, 21, 22, 23, 25};

#define BOARD_HAS_RTC 0

#endif
