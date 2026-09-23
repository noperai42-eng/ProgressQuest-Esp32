#!/usr/bin/env bash
# Restore a 16 MiB full-flash image onto the connected ESP32-S3.
# Usage: restore-flash.sh <image.bin> [port] [baud]
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ESPTOOL="${ROOT}/tools/venv/bin/esptool"
IMAGE="${1:-}"
PORT="${2:-/dev/cu.usbmodem1101}"
BAUD="${3:-460800}"

if [[ -z "$IMAGE" || ! -f "$IMAGE" ]]; then
  echo "Usage: $0 <16MiB-image.bin> [port] [baud]" >&2
  exit 1
fi

SIZE="$(wc -c < "$IMAGE" | tr -d ' ')"
if [[ "$SIZE" != "16777216" ]]; then
  echo "Refusing to flash: expected 16777216 bytes, got ${SIZE}" >&2
  exit 2
fi

echo "Flashing ${IMAGE} (${SIZE} bytes) to ${PORT}"
"$ESPTOOL" --chip esp32s3 --port "$PORT" --baud "$BAUD" \
  --before default-reset --after hard-reset \
  write-flash --flash-mode dio --flash-size 16MB --flash-freq 80m \
  0x0 "$IMAGE"

echo "Restore complete."
