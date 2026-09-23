# Device backups

Full 16 MiB SPI-flash dumps of the Waveshare ESP32-S3-Touch-AMOLED-1.75
connected over USB-C (`/dev/cu.usbmodem1101` on this Mac).

The board is **not** encrypted and **not** secure-boot locked, so a dump is a
complete restore image. Write it back at offset `0x0`.

## Restore this board

```bash
./scripts/restore-flash.sh backups/<stamp>/full-flash.bin /dev/cu.usbmodem1101
```

If automatic reset fails: hold **BOOT**, tap **PWR**, start the command, then
release BOOT once writing begins.

## New dump

```bash
./scripts/backup-flash.sh /dev/cu.usbmodem1101
```

Needs `tools/venv` with `esptool` installed.
