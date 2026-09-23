#!/usr/bin/env bash
# Dump the connected ESP32-S3's entire 16 MiB flash into backups/<utc-stamp>/.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ESPTOOL="${ROOT}/tools/venv/bin/esptool"
PORT="${1:-/dev/cu.usbmodem1101}"
BAUD="${2:-921600}"
STAMP="$(date -u +%Y%m%d-%H%M%S)"
OUT="${ROOT}/backups/${STAMP}"

if [[ ! -x "$ESPTOOL" ]]; then
  echo "esptool missing. Create tools/venv and pip install esptool." >&2
  exit 1
fi

mkdir -p "$OUT"

{
  echo "timestamp_utc: ${STAMP}"
  echo "port: ${PORT}"
  echo "baud: ${BAUD}"
  echo
  "$ESPTOOL" --chip esp32s3 --port "$PORT" --baud 115200 --before default-reset --after no-reset chip-id
  echo
  "$ESPTOOL" --chip esp32s3 --port "$PORT" --baud 115200 --before no-reset --after no-reset flash-id
  echo
  "$ESPTOOL" --chip esp32s3 --port "$PORT" --baud 115200 --before no-reset --after no-reset get-security-info
} | tee "${OUT}/chip-info.txt"

echo "Reading 16 MiB flash -> ${OUT}/full-flash.bin"
"$ESPTOOL" --chip esp32s3 --port "$PORT" --baud "$BAUD" --before no-reset --after hard-reset \
  read-flash 0 0x1000000 "${OUT}/full-flash.bin"

SIZE="$(wc -c < "${OUT}/full-flash.bin" | tr -d ' ')"
SHA="$(shasum -a 256 "${OUT}/full-flash.bin" | awk '{print $1}')"
{
  echo "bytes: ${SIZE}"
  echo "sha256: ${SHA}"
} | tee "${OUT}/checksums.txt"

if [[ "$SIZE" != "16777216" ]]; then
  echo "WARNING: expected 16777216 bytes, got ${SIZE}" >&2
  exit 2
fi

echo "Backup complete: ${OUT}"
