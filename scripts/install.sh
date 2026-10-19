#!/usr/bin/env bash
set -euo pipefail

# Assumes arduino-cli is already installed.
# Installs the ESP32 core + libraries this sketch depends on.

arduino-cli config init --overwrite || true
arduino-cli config add board_manager.additional_urls \
  https://espressif.github.io/arduino-esp32/package_esp32_index.json

arduino-cli core update-index
arduino-cli core install esp32:esp32@3.3.12

arduino-cli lib install "ArduinoJson@7.4.3"

# Phase 2 (ESP32-S3 + RTC) only — safe to install even for phase 1 work:
arduino-cli lib install "RTClib@2.1.4"
arduino-cli lib install "Adafruit BusIO@1.17.4"

echo "Done. Verify with: arduino-cli core list && arduino-cli lib list"

