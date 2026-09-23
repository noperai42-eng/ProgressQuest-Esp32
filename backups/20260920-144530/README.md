# Backup 20260920-144530

Taken 2026-09-20 14:45 UTC from the ESP32-S3 currently on the desk, before any
Tami firmware was written.

## Chip

| Field | Value |
| --- | --- |
| SoC | ESP32-S3 (QFN56) revision v0.2 |
| USB | Espressif USB JTAG/serial debug unit |
| MAC | redacted |
| PSRAM | 8 MB (in-package) |
| Flash | 16 MB, manufacturer `20` device `4018`, quad, 3.3 V |
| Secure boot | disabled |
| Flash encryption | disabled |

## Image

| Field | Value |
| --- | --- |
| File | `full-flash.bin` |
| Size | 16,777,216 bytes |
| SHA-256 | `ce0e34c65f67da210cfad259f2f9e71d8b65cf079eaff39b7dc800eca3c2c3e7` |
| Matches Waveshare FactoryOnly-260805 | **no** (that published image is SHA `2b7e01ff1f37027385a3820e006f945464cfcee2a05954b3239cf75f250080b6`) |

The mismatch is expected: this dump is the **live** flash, including NVS,
assets, and storage. It is also an older build than the 2026-08-05 factory
recovery image on GitHub. Use **this file** to put the board back to the state
it had today, not the published recovery image.

## What was running

`otadata` is erased (all `0xFF`), so the chip boots the **factory** app:

- Project `esp-brookesia`, version `1`
- Compiled `May 28 2026 16:20:50`
- ESP-IDF `v5.5.4-dirty`
- Description string: "Display ESP-Brookesia phone demo"

The unused `ota_0` slot contains **xiaozhi** `2.2.6`, same compile day. It is
not selected.

## Partition table (`0x8000`)

| Name | Type | Offset | Size |
| --- | --- | --- | --- |
| nvsfactory | data | `0x9000` | 200 KiB |
| nvs | data | `0x3b000` | 840 KiB |
| otadata | data | `0x10d000` | 8 KiB |
| phy_init | data | `0x10f000` | 4 KiB |
| factory | app | `0x110000` | 5632 KiB |
| ota_0 | app | `0x690000` | 3072 KiB |
| assets | data | `0x990000` | 3072 KiB |
| storage | data | `0xc90000` | 3520 KiB |

## Restore

```bash
./scripts/restore-flash.sh backups/20260920-144530/full-flash.bin
```
