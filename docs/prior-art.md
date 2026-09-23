# Prior art — Waveshare ESP32-S3-Touch-AMOLED-1.75

Survey date: 2026-09-20. Scope is this exact board (SKU 31261, plus enclosure 31262 and GNSS 31264) and firmware that names it. Near-miss boards are listed so they are not treated as drop-in ports.

## 1. The device

Waveshare’s own docs and hardware reference describe a round 1.75-inch 466×466 capacitive AMOLED on an ESP32-S3R8 with 8 MB PSRAM and 16 MB flash. The panel is a CO5300 over QSPI; touch is a CST9217 on I2C; power is AXP2101; motion is QMI8658; time is PCF85063; audio is ES8311 + ES7210; storage is 1-bit SDMMC. SKUs:

| SKU | Name | Difference |
| --- | --- | --- |
| 31261 | ESP32-S3-Touch-AMOLED-1.75 | Bare board (this desk unit) |
| 31262 | …-B | Same PCB in Waveshare’s protective case |
| 31264 | …-G | Same PCB + LC76G GNSS |

Sources: [Waveshare docs](https://docs.waveshare.com/ESP32-S3-Touch-AMOLED-1.75), [official GitHub README](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75), [HARDWARE_REFERENCE.md](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75/blob/main/HARDWARE_REFERENCE.md).

**1.75C is a different product.** Same 466×466 CO5300 + CST9217 marketing line, different PCB, different BSP (`waveshare/esp32_s3_touch_amoled_1_75c`), different reset pins (LCD reset GPIO1 / touch reset GPIO2 vs GPIO39 / GPIO40 on 1.75), and an aluminum watch-style case. TamaPoke and Capsule Radar both warn not to flash 1.75 images onto 1.75C. Source: [1.75C repo](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C), [Capsule Radar README](https://github.com/socquique/capsule-radar).

## 2. Official firmware (already on this unit)

The vendor repo ships three firmware forms:

1. **CI example packages** (`*-combined.zip` at flash offset `0x0`) for ESP-IDF v5.5.5 / v6.0.2 and Arduino-ESP32 3.3.10. Latest tagged release is v1.0.1 (19 packages).
2. **Brookesia source** under `firmware/brookesia/`, ESP-IDF 5.5.4, a round-screen phone launcher ported from ESP32-P4 Brookesia. Apps: SquareLine, Calculator, DrawPanel, SpecAnalyzer, MusicPlayer, Gallery, VideoPlayer, Recorder, Settings, AIChats (Xiaozhi transports + WakeNet/VAD/Opus), Gravitysphere (QMI8658), Crosshair, Button Test.
3. **Factory recovery image** `ESP32-S3-Touch-AMOLED-1.75-FactoryOnly-260805.bin` (16,777,216 bytes, SHA-256 `2b7e01ff…80b6`, app `v1.0.1-9-gec7380d`).

This desk unit’s live dump (see `backups/20260920-144530/`) boots Brookesia v1 compiled 28 May 2026 (`esp-brookesia`, IDF `v5.5.4-dirty`) and has an unused `ota_0` slot containing **xiaozhi 2.2.6** from the same day. That matches Waveshare shipping Xiaozhi both as Brookesia’s AIChats app and as a standalone image.

Board support package: [`waveshare/esp32_s3_touch_amoled_1_75` ^3.0.1](https://components.espressif.com/components/waveshare/esp32_s3_touch_amoled_1_75) on the ESP Component Registry (~3.5k downloads). Source tree: [Waveshare-ESP32-components](https://github.com/waveshareteam/Waveshare-ESP32-components).

## 3. Direct hit: a Tamagotchi already exists on this board

**[TamaPoke](https://github.com/socquique/TamaPoke)** (socquique, MIT firmware, ~129★) is a Gen-1 Pokémon Tamagotchi **written for this exact SKU**: round 466×466, CO5300 QSPI, CST9217, AXP2101, PCF85063, ES8311, microSD. Status in the README: running on hardware, firmware v1.17, browser installer at https://socquique.github.io/TamaPoke/web/.

What it already implements (values quoted from the repo, not inferred):

- Four needs (FOOD / JOY / ENE / HYG) that drain on a real-time clock; **1 real minute = 1 in-game minute**; **+1 level per real hour**; **ages while powered off** via PCF85063, catch-up capped at 2 weeks.
- Egg → hatch → evolution (player-witnessed, not automatic) → farewell / release / runaway.
- Touch: tap to pet, swipe for Pokédex / stats / clock; PWR short = screen off, long = power off with RTC alive.
- AXP2101 battery + anti-burn-in dimming + ES8311 tones.
- Arduino-ESP32, `Arduino_GFX` (moononournation), SensorLib + XPowersLib (Lewis He), 16 MB / OPI PSRAM / `app3M_fat9M_16MB`. Pins copied from Waveshare’s `pin_config.h`; MCLK corrected to GPIO42 (the 16 in some docs is wrong).
- Explicitly: Standard or -G, **not -B case** (doesn’t fit their Pokéball print), **not 1.75C**.

Sprites are PMD SpriteCollab (CC BY-NC) + Pokémon IP. Firmware MIT. Two substantial forks: [ShadowEnemyx battles/dex](https://github.com/ShadowEnemyx/TamaPoke/tree/tamapoke-expanded-update) and [DylanPDao gym + LAN battles through Gen 3](https://github.com/DylanPDao/TamaPoke).

**Implication for Tami:** the “can we even do a Tamagotchi on this glass” question is already answered yes. Copying TamaPoke’s Pokémon content is legally off-limits for a commercial or original-creature OS. The *loop* (RTC aging, four needs, PWR-sleep, AXP2101, CST9217 gestures) is the reusable engineering prior art.

Same author also shipped **[Capsule Radar](https://github.com/socquique/capsule-radar)** on this board: PlatformIO + LVGL v8 + Arduino_GFX, CST9217, QMI8658 face-down sleep, AXP2101, PCF85063, ES8311, web flasher, native SDL 466×466 simulator. Useful as a second, cleaner bring-up of the same pin map.

## 4. Other complete firmware on this exact SKU

| Project | Stack | What it proves |
| --- | --- | --- |
| [PrintSphere](https://github.com/cptkirki/PrintSphere) v1.6.2 (~293★) | ESP-IDF 5.5.4, LVGL 9.5, official Waveshare BSP | Production round-UI, OTA, battery/USB detect, TE-sync, web installer. First-class `amoled_1_75` variant. Best ESP-IDF reference. |
| [78/xiaozhi-esp32](https://github.com/78/xiaozhi-esp32) | ESP-IDF, board dir `esp32-s3-touch-amoled-1.75` | Voice AI OS with a dedicated board port. Touch INT was fixed to GPIO11 (`#1784`). 1.75C is a *separate* `CONFIG_BOARD_TYPE_…_1_75C`. This is the firmware sitting in our unused OTA slot. |
| [open-bike-computer](https://github.com/seichris/open-bike-computer) | Arduino_GFX | Field notes: CO5300 constructor gap `(6,0,0,0)`; CST9217 often needs `x=465-x`, `y=465-y`; do not treat GPIO21 as the only touch IRQ; TCA9554 P0 vs GPIO40 reset confusion. |
| ESPHome `mipi_spi` CO5300 | ESP-IDF | Works, but without `esp_lcd_panel_set_gap(panel, 6, 0)` it draws a 1-px green wrap line. Confirmed against Waveshare BSP and PrintSphere. [Issue #15765](https://github.com/esphome/esphome/issues/15765). |
| [Smartwatch-V5](https://github.com/mathcampbell/Smartwatch-V5) | Arduino / PlatformIO / LVGL | Watch UI on this 1.75″ board ([Hackaday, 18 Feb 2026](https://hackaday.com/2026/02/18/diamond-age-inspired-pocket-watch-has-esp32-inside/)). Not a pet. |
| [The Badge](https://github.com/curisama/The-Badge) | custom | **1.75C only** (no microSD, no RTC). Clock, games, HID, recorder, liquid sim. |

## 5. Hardware quirks that keep showing up

These are independently reported by Waveshare BSP, PrintSphere, TamaPoke, Capsule Radar, open-bike-computer, and ESPHome. Treat them as board facts, not folklore:

1. **CO5300 column gap is 6 pixels.** `esp_lcd_panel_set_gap(handle, 6, 0)`. Skip it and column 0 wraps as a green line.
2. **Touch reset is GPIO40 on 1.75**, not GPIO20 (USB D+). 1.75C uses GPIO2. Some Arduino sketches routed reset through TCA9554 P0 — verify against the schematic before copying.
3. **Touch INT is GPIO11** on 1.75 (xiaozhi #1784). GPIO21 is QMI8658 INT2, not the CST9217 line.
4. **I2S MCLK is GPIO42**, not GPIO16 (TamaPoke pin_config comment; Waveshare hardware reference). GPIO16 is the expansion header.
5. **466×466×16-bit framebuffer ≈ 434 KB** — it lives in OPI PSRAM. Builds without PSRAM black-screen.
6. **Flash is 16 MB, DIO, 80 MHz.** Mixing 32 MB image headers with this chip fails at boot (vendor troubleshooting doc).
7. **PWR is not a GPIO.** Conditioned `SYS_OUT` is TCA9554 P4/EXIO4; power path is AXP2101. BOOT is GPIO0.
8. **1.75 firmware is not binary-compatible with 1.43, 1.8, 1.75C, or 2.06.** Same family, different pin maps and often different touch ICs.

## 6. Near-miss boards (do not treat as this SKU)

| Board | Why it shows up in searches | Actual difference |
| --- | --- | --- |
| ESP32-S3-Touch-AMOLED-**1.75C** | Same number, community showcase (The Badge, WaveStopwatch, sand sim, Claude Desktop Buddy, Codex Island) | Different PCB/BSP/resets; aluminum case |
| ESP32-S3-Touch-AMOLED-**1.43** | Same 466×466 CO5300 | FT3168 touch @ 0x38, different QSPI pins, no AXP2101/ES8311 on the variant Capsule Radar supports |
| ESP32-S3-Touch-AMOLED-**1.8** | “Waveshare AMOLED Tamagotchi” press | Rectangular 368×448 **SH8601**, FT3168, 8 MB flash. [Polymo](https://www.xda-developers.com/this-esp32-s3-tamagotchi-will-die-if-you-doomscroll-on-your-phone-too-much/) lives here, not on 1.75 |
| ESP32-S3-Touch-AMOLED-**2.06** | CO5300 + AXP2101 family | 410×502, FT3168, 32 MB flash |
| LilyGO T-Display / T-Circle | ESP32-S3 round/rectangular AMOLED | Different pinout and panel |

1.75C community (useful for round-UI ideas, not for flashing): [The Badge / Hackaday 2026-09-16](https://hackaday.com/2026/09/16/round-amoled-badge-does-all-kinds-of-cool-stuff/) (air mouse, clock, games, WAV recorder, liquid sim, ~20 h standby); Waveshare’s own 1.75C showcase lists Claude Desktop Buddy, Volos WaveStopwatch, Codex Island, Nurdism sand sim.

## 7. Virtual-pet prior art *not* on this board

| Project | Hardware | Takeaway |
| --- | --- | --- |
| [TamaFi](https://github.com/cifertech/TamaFi) (~417★) | Custom ESP32-S3 + ST7789 240×240, 6 buttons | Wi-Fi-eating pet, autonomous AI, MIT. Mechanics/UI, not a port. |
| [brenpoly/polymo](https://github.com/brenpoly/polymo) | Waveshare **1.8** 368×448 (SH8601 or CO5300 at boot) | Hunger/happiness/life-stage on-device; NVS + PCF85063 + AXP2101; “never put simulation state on SD.” BLE doomscroll companion. |
| [toddsherman/pixelcat](https://github.com/toddsherman/pixelcat) | 1.8 AMOLED | Host-testable C care/decay (`stats.c` offline catch-up). Wrong panel (FT3168 / CST820). |
| [frolic/pocket-pet](https://github.com/frolic/pocket-pet) | 2.06 kit | LVGL pet FSM split from `device/` BSP; QMI8658 steps. Portable `app/` is the interesting part. |
| [mediacutlet/pocket-tank](https://github.com/mediacutlet/pocket-tank) | 1.8 + SDL sim | Care/LLM split in `common/`; already retargeted to P4 by a fork. Pattern to copy, not a 1.75 binary. |
| [jcrona/tamalib](https://github.com/jcrona/tamalib) | original P1/P2 ROM | Hardware-agnostic first-gen Tamagotchi *emulator*. Needs a copyrighted ROM those repos do not ship. Do not go this way. |
| [claude-desktop-buddy-esp32](https://github.com/vthinkxie/claude-desktop-buddy-esp32) | 1.8 / **1.75C** / 2.16 | Desktop face; 1.75C port, not 1.75. |
| Coglet / Stackchan / generic xiaozhi faces | Various, including a vendored 1.75 board dir | Voice companion, not a care-pet OS. |

Classic Tamagotchi need-decay + RTC-offline aging is public-domain game design (Bandai 1996). TamaPoke’s numbers are one calibration of that loop, not a lock on the genre.

## 8. What is reusable vs green field

**Reuse (bring-up, not product identity):**

- Waveshare BSP + ESP-IDF 5.5.x + LVGL 9 (PrintSphere / official Brookesia path).
- Registry chips the BSP actually wraps: `espressif/esp_lcd_co5300` v2.2.0, `waveshare/esp_lcd_touch_cst9217` v2.0.0, `espressif/esp_codec_dev` (ES8311/ES7210), `espressif/esp_io_expander_tca9554`.
- The 1.75 BSP does **not** expose AXP2101, QMI8658, or PCF85063. Those come from XPowersLib / SensorLib / `espp/pcf85063` (same as Waveshare’s own Arduino examples).
- Or Arduino_GFX + SensorLib + XPowersLib (TamaPoke / Capsule Radar path).
- Host-testable care engine pattern from PixelCat / pocket-pet (`app/` vs `device/`). TamaPoke’s `pet.cpp` is already board-agnostic; only the 466×466 circle UI is not.
- Pin map, 6 px CO5300 gap, CST9217 address 0x5A, AXP2101 @ 0x34, QMI8658 @ 0x6B, RTC @ 0x51.
- PWR-button-via-AXP2101 + RTC-alive deep sleep pattern.
- Web-serial installer pattern (ESP Web Tools) — both TamaPoke and PrintSphere ship one.

**Do not copy:**

- Pokémon sprites, names, or TamaPoke’s PMD pipeline (CC BY-NC + Nintendo).
- Brookesia/Xiaozhi as the product (already on the device; replacing it is the point).
- 1.75C or 1.8 binaries.

**Green field for Tami:**

- An original creature (not a Pokémon clone, not Xiaozhi’s emoji face).
- An *OS* whose home screen is the pet, rather than a single-game firmware.
- Combining TamaPoke’s care loop with PrintSphere’s ESP-IDF/LVGL production stack (nobody has published that combination on this SKU).
- IMU shake as a first-class care gesture (Gravitysphere proves the sensor; TamaPoke barely uses it).
- Dual-mic / speaker character voice that is *not* Xiaozhi cloud chat.

## 9. Sources inspected

Primary (opened, not search-snippet only):

- https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75
- https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75/blob/main/HARDWARE_REFERENCE.md
- https://docs.waveshare.com/ESP32-S3-Touch-AMOLED-1.75
- https://components.espressif.com/components/waveshare/esp32_s3_touch_amoled_1_75
- https://github.com/socquique/TamaPoke (README, CREDITS, pin_config.h)
- https://github.com/socquique/capsule-radar
- https://github.com/cptkirki/PrintSphere
- https://github.com/78/xiaozhi-esp32/tree/main/main/boards/waveshare/esp32-s3-touch-amoled-1.75
- https://github.com/esphome/esphome/issues/15765
- https://github.com/seichris/open-bike-computer/tree/main/hardware
- https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C
- Live dump: `backups/20260920-144530/` (Brookesia factory + xiaozhi ota_0)

Secondary:

- https://hackaday.com/2026/09/16/round-amoled-badge-does-all-kinds-of-cool-stuff/ (1.75C)
- https://www.xda-developers.com/this-esp32-s3-tamagotchi-will-die-if-you-doomscroll-on-your-phone-too-much/ (1.8 Polymo)
- https://github.com/cifertech/TamaFi
