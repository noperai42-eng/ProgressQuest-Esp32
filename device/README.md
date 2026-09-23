# Tami device (v1.0 hatchling)

ESP-IDF firmware for the Waveshare ESP32-S3-Touch-AMOLED-1.75. Same `app/` sim
as the desk tools. First boot hatches a random people/calling, draws the round
home (name, blob, task bar), ticks one sim second per real second. Tap the disc
to pep.

Factory backup is still `backups/20260920-144530/full-flash.bin`. Restore with
`./scripts/restore-flash.sh` if this image misbehaves.

## Build

ESP-IDF v5.5.x, target `esp32s3`:

```bash
cd device
. $IDF_PATH/export.sh
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/cu.usbmodem1101 flash monitor
```

Or from the repo root (Docker IDF image, if Colima is up):

```bash
make device
./scripts/flash-device.sh /dev/cu.usbmodem1101
```
