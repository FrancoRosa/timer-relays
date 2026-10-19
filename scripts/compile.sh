#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

PROFILE="${1:-wroom32}"   # wroom32 (phase 1) or s3 (phase 2)

arduino-cli compile --profile "$PROFILE" .



