# ProgressQuest Esp32

A **pocket Progress Quest** for the Waveshare **ESP32-S3-Touch-AMOLED-1.75**
(round 466×466 capacitive AMOLED). One original fantasy adventurer (orc, elf,
human, undead) kills, loots, sells, and quests by themselves. You watch the
bar. Richer than the 2002 original: a living figure, stats that matter,
optional camp. Not Warcraft-branded. The firmware and desk sim are named Tami.
Spec: [`CLAUDE.md`](CLAUDE.md).

## This board

| Piece | Chip / bus |
| --- | --- |
| MCU | ESP32-S3R8, 240 MHz, dual-core + LP core |
| Display | CO5300 QSPI AMOLED, 466×466 |
| Touch | CST9217 (I2C) |
| Power | AXP2101 + 3.7 V battery connector |
| Motion | QMI8658 6-axis IMU |
| Clock | PCF85063 RTC |
| Audio | ES8311 speaker + ES7210 dual mics |
| Storage | microSD (1-bit SDMMC) |

## Desk simulator (no board)

The Progress Quest loop builds and runs on this Mac. Same `app/` the firmware will call.

```bash
make test                          # host unit tests
make sim                           # CLI desk
make gfx                           # 466×466 round window (SDL2)
./build/tami-sim --hours 8 --people orc --calling warrior --seed 1
./build/tami-gfx --seed 1          # click camp/bag/log/sheet/pets; space pep
./build/tami-gfx --pet toad --menu pets
./build/tami-gfx --people elf --armor plate --field moonwood
./build/tami-gfx --mob wight --screenshot /tmp/wight.bmp
./build/tami-gfx --menu hatch --screenshot docs/screenshots/menu-hatch.bmp
./build/tami-gfx --menu camp --screenshot docs/screenshots/menu-care.bmp
./build/tami-gfx --hours 8 --menu log --no-card --screenshot docs/screenshots/menu-log.bmp
./build/tami-gfx --hours 8 --menu sheet --no-card --screenshot docs/screenshots/menu-sheet.bmp
./build/tami-gfx --hours 8 --seed 1 --screenshot /tmp/tami.bmp
```

Painted people × armor references: [`assets/art/README.md`](assets/art/README.md). Contact sheet: [`docs/screenshots/armor-set.png`](docs/screenshots/armor-set.png).

Feature roadmap (many releases): [`docs/FEATURES.md`](docs/FEATURES.md).
Progress Quest reference (not linked in): `vendor/pq-cli`.

Device hatchling (ESP-IDF, same `app/`):

```bash
make device
./scripts/flash-device.sh /dev/cu.usbmodem1101
make device-cli                    # USB REPL (rate / tick / hours / status)
./scripts/device-cli.py watch 60   # 60 sim-seconds per real second while plugged in
make paths                         # 16 hatchlings × 8h work day + late game
./scripts/progress-harness.py --device   # same paths on the hooked-up disc
make harness-device                # alias
```

The product is a **side companion**: hatch one heart, leave it on the desk through a work day, pick it up at lunch and read **log** (the story so far). **Sheet** is stats plus worn gear and updates as kills land; scroll with **PWR** / **BOOT**. Tap the field name on home to travel. **Camp** is optional — they rest on their own. Pets **gone** warns once before it actually banishes them. Every people × calling finishes an 8h day at about level 10, Act I, with a toad at heel; a long weekend on the desk reaches Act V in Ashfen. Rare and legendary drops sparkle on the figure (`--gleam` to preview). Found companions are still later — pets are the company for now.

Plugged in over USB, the disc runs **20×** by default so kills and menus can be watched. Unplug and it drops back to 1 sim second per real second. `rate 1` forces pocket speed while still on the cable; `auto` restores the USB/unplug behaviour. The device harness sets `rate 1` while it gulps `hours 8` so the UART is not racing the story dump.

Factory restore: `./scripts/restore-flash.sh backups/20260920-144530/full-flash.bin`.

## Factory backup (do this before flashing)

The original flash image from 2026-09-20 is in
`backups/20260920-144530/full-flash.bin`. Restore it with:

```bash
./scripts/restore-flash.sh backups/20260920-144530/full-flash.bin
```

Take a new dump with `./scripts/backup-flash.sh`.

## Prior art

The idle loop follows [Progress Quest](https://progressquest.com/) by Eric Fredricksen (MIT, [license.txt](http://progressquest.com/license.txt); Delphi source [bitbucket.org/grumdrig/pq](https://bitbucket.org/grumdrig/pq)) and the later CLI port [pq-cli](https://github.com/rr-/pq-cli) by Marcin Kurczewski (MIT, 2018). This tree does not copy their joke races or word lists. Citations: [`NOTICE`](NOTICE), [`docs/prior-art.md`](docs/prior-art.md).

This exact board already has a Tamagotchi-class firmware ([TamaPoke](https://github.com/socquique/TamaPoke)), a production LVGL OS ([PrintSphere](https://github.com/cptkirki/PrintSphere)), and first-party Xiaozhi/Brookesia.

## License

[MIT](LICENSE). Copyright (c) 2026 noperai42-eng.
