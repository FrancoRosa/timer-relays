#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

PROFILE="${1:-wroom32}"
PORT="${2:-/dev/ttyUSB0}"

arduino-cli compile --profile "$PROFILE" .
arduino-cli upload --profile "$PROFILE" -p "$PORT" .