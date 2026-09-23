#!/usr/bin/env bash
# Flash the Tami hatchling (device/build) onto the connected ESP32-S3.
# Usage: flash-device.sh [port]
#        flash-device.sh --full [port]   # merged image at 0x0 (wipes NVS / save)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FULL=0
PORT="/dev/cu.usbmodem1101"
if [[ "${1:-}" == "--full" ]]; then
  FULL=1
  PORT="${2:-/dev/cu.usbmodem1101}"
elif [[ -n "${1:-}" ]]; then
  PORT="$1"
fi
BUILD="${ROOT}/device/build"
ESPTOOL="${ROOT}/tools/venv/bin/esptool"

if [[ ! -x "$ESPTOOL" ]]; then
  echo "esptool missing (tools/venv)." >&2
  exit 1
fi

if [[ ! -f "$BUILD/flasher_args.json" && ! -f "$BUILD/tami.bin" ]]; then
  echo "No firmware in device/build. Run: make device" >&2
  exit 1
fi

BOOT="$BUILD/bootloader/bootloader.bin"
PART="$BUILD/partition_table/partition-table.bin"
APP="$BUILD/tami.bin"
MERGED="$BUILD/merged-binary.bin"

if [[ "$FULL" == 1 ]]; then
  if [[ ! -f "$MERGED" ]]; then
    echo "No merged-binary.bin. Run: make device" >&2
    exit 1
  fi
  echo "Flashing merged image ${MERGED} to ${PORT} (erases NVS save)"
  "$ESPTOOL" --chip esp32s3 --port "$PORT" --baud 460800 \
    --before default-reset --after hard-reset \
    write-flash --flash-mode dio --flash-size 16MB --flash-freq 80m \
    0x0 "$MERGED"
  exit 0
fi

# App-only (+ bootloader/table) so the NVS adventurer blob at 0x9000 survives.
if [[ -f "$BOOT" && -f "$PART" && -f "$APP" ]]; then
  echo "Flashing bootloader + table + app to ${PORT} (keeps NVS save)"
  "$ESPTOOL" --chip esp32s3 --port "$PORT" --baud 460800 \
    --before default-reset --after hard-reset \
    write-flash --flash-mode dio --flash-size 16MB --flash-freq 80m \
    0x0 "$BOOT" \
    0x8000 "$PART" \
    0x10000 "$APP"
  exit 0
fi

if [[ -f "$MERGED" ]]; then
  echo "Piecewise bins missing; flashing merged image ${MERGED} to ${PORT}"
  "$ESPTOOL" --chip esp32s3 --port "$PORT" --baud 460800 \
    --before default-reset --after hard-reset \
    write-flash --flash-mode dio --flash-size 16MB --flash-freq 80m \
    0x0 "$MERGED"
  exit 0
fi

echo "Using esptool write-flash from flasher_args.json"
# shellcheck disable=SC2046
"$ESPTOOL" --chip esp32s3 --port "$PORT" --baud 460800 \
  --before default-reset --after hard-reset \
  write-flash --flash-mode dio --flash-size 16MB --flash-freq 80m \
  $(python3 -c "
import json
p='$BUILD/flasher_args.json'
d=json.load(open(p))
flash=d.get('flash_files') or d.get('extra_esptool_args',{})
# IDF 5 flasher_args: { 'write_flash_args': [...], 'flash_files': {'0x0': 'path'} }
files=d.get('flash_files', {})
for off, path in sorted(files.items(), key=lambda kv: int(kv[0], 16)):
    print(off, '$BUILD/' + path if not path.startswith('/') else path)
")
