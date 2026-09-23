# CLAUDE.md — Tami

## Project Overview

Tami is a **pocket Progress Quest**: one original fantasy adventurer on a Waveshare ESP32-S3-Touch-AMOLED-1.75 (round 466×466 AMOLED) who kills, loots, sells, buys, and quests **by themselves**. You watch a current-task bar fill. You do not tap to attack.

Richer than Progress Quest: they are a living figure on glass, not a Windows form. Stats and gear actually change fight time and drops. Hunger and rest can send them to camp. You may pep, feed, or send them to a different field — optional, never required. It is not World of Warcraft, not a phone launcher, and not a Pokémon clone.

Smallest version worth shipping: hatch a people, put the board down, pick it up later, and see a filled quest bar, a pile of named loot, and a hungry orc.

## Tech Stack

| Layer | Choice | Why |
| --- | --- | --- |
| MCU firmware | ESP-IDF 5.5.x, target `esp32s3` | Board BSP and PrintSphere are validated here; FreeRTOS + light sleep |
| UI | LVGL 9 | Round clip, official Waveshare / PrintSphere path |
| Board glue | `waveshare/esp32_s3_touch_amoled_1_75` ^3.x | Display, touch, codec, TCA9554 |
| Panel / touch | `espressif/esp_lcd_co5300` + `waveshare/esp_lcd_touch_cst9217` | Registry drivers the BSP already wraps |
| PMU / IMU / RTC | XPowersLib (AXP2101), SensorLib (QMI8658, PCF85063) | BSP does not expose these chips |
| Audio | `espressif/esp_codec_dev` → ES8311 | Short stings, not streaming music in v1 |
| Sim + rules | Plain C in `app/` (no ESP headers) | Host-testable idle catch-up; device/ is BSP only |
| Persist | NVS (adventurer blob) | Simulation state never lives on SD (Polymo rule) |
| Host tests | C11 + `make test` | Catch-up math must be deterministic |
| Desk sim | `host/main.c` CLI + `host/gfx.c` 466×466 SDL2 | Same `app/` as the device; gfx is the panel stand-in |
| Flash | 16 MB, DIO 80 MHz | This SKU only — not 1.75C |

Arduino_GFX is the TamaPoke path. We do not take it: this is an OS-shaped idle sim with LVGL, not a single `.ino`.

## Architecture Principles

1. **Time is the engine.** All progress is elapsed RTC time applied as discrete ticks. Waking from an 8-hour nap must not replay 28,800 one-second frames; catch-up is closed-form or coarse chunks with a hard cap.
2. **`app/` does not include ESP headers.** People, needs, combat, loot, and catch-up compile and test on the host. `device/` owns display, touch, PMU, RTC, NVS adapters.
3. **The current task is the game.** Always one act with a progress bar (kill, walk to market, sell one item, buy, walk back, camp). Like Progress Quest. The sprite animates that act; the sim does not wait on frames.
4. **Care is optional throttle, not a chore wheel.** Needs slow the current task and can force camp. A neglected adventurer still *has* a bar — it just crawls. You can ignore them and they still Progress-Quest; you cannot ignore them forever (fade).
5. **One heart, later a party.** v0–v2 is a single living adventurer whose **gear is visible on the figure**. Later they *find* companions in the field and raid; the hatchling remains the Tamagotchi you care for. No alts, no Warcraft-style character select.
6. **This SKU only.** Pins, 6 px CO5300 gap, CST9217 @ 0x5A, 16 MB flash. 1.75C / 1.8 / 2.06 images are out of scope.
7. **Original folklore fantasy.** No Warcraft names, factions, maps, chrome, or model sheets. No Progress Quest joke races (Double Wookiee, Eel Man). Procedural *item and monster* names are in-house.
8. **Offline.** No account, no Wi-Fi requirement, no cloud loot. Clock comes from PCF85063 (user-set) plus optional NTP later.

## Project Structure

```
Tami/
  CLAUDE.md                 this spec
  PLAN.md                   segmented build (when locked)
  app/                      host-testable rules (C)
    sim.h / sim.c           tick + catch-up
    adventurer.h            blob: people, calling, stats, needs, xp, gear
    task.h                  current act + progress bar (kill / market / sell / buy / road / camp)
    combat.h                monster duration from level + stats + gear
    loot.h                  encumbrance, sell-one-by-one, procedural names
    quest.h                 exterminate / deliver / seek / placate
    plot.h                  act checklist
    names.h                 monster + item generators
    needs.h                 hunger / rest / morale / wounds
  device/                   ESP-IDF project
    main/
    boards/                 1.75 pin/BSP wrappers
  host/                     desk simulator (CLI). Same `app/` as the device.
    main.c
  tests/                    host tests for app/
  Makefile                  `make test` and `make sim`
  assets/                   original sprites (not third-party IP)
  backups/                  factory flash dumps
  scripts/                  backup/restore
  docs/prior-art.md
```

## Domain Model / Spec

### The loop (Progress Quest, on glass)

Always exactly one **current task** with a progress bar. The sim advances that bar with RTC time. When it completes, the next task is chosen automatically:

1. **Kill** — “Executing 4 bristled gnolls…” Duration from level + stats + gear + need multipliers. On finish: XP, one loot line, quest +1, plot + a little. Then either another kill or market if encumbered.
2. **Road to market** — short travel bar.
3. **Sell** — one inventory item per bar (this is the PQ charm; do not bulk-sell in one tick on screen. Catch-up may collapse sells).
4. **Buy** — if gold can afford a better slot, “Negotiating a better blade…”
5. **Road to the fields** — back to kill.
6. **Camp** — only if hunger or rest hit the floor, or the player sent them. Bar is rest; they are not earning XP.
7. **Downed** — wounds maxed; bar is “crawling to camp” until bandaged or catch-up applies a slow crawl.

You never tap to attack. Optional: feed, pep, bandage, pick a field. Screen off or powered down to RTC: same state machine, catch-up on wake.

PWR short: screen off, sim continues. PWR long: AXP2101 off; PCF85063 keeps time; catch-up at boot.

### People (v1 roster)

| People | Need bias | Idle bias |
| --- | --- | --- |
| Human | balanced | extra gold |
| Orc | hunger drains faster; wounds recover faster | extra melee damage |
| Elf | rest drains slower; morale hates filth | extra rare-drop chance |
| Undead | no “food” — they drain **ichor** (same hunger slot, different item); daylight zones slightly harsher | extra night-zone XP |

Visual language is original: not Blizzard green-skin shoulder-pad orcs, not glowing-eye Forsaken, not Night Elf face tattoos. Readable at 466×466, round-safe (feet/anchor in the lower third).

### Callings (classes)

Warrior, Ranger, Mage, Rogue. Generic words. Chosen once after hatch (not a WoW talent tree). Calling weights which stats grow on level-up and which weapon bases drop. No respec in v1.

### Stats (they matter)

STR, CON, DEX, INT, WIS, CHA — rolled at hatch, grow on level-up (weighted toward the calling’s two primaries). Unlike Progress Quest, these are not flavor:

| Stat | Effect |
| --- | --- |
| STR | Encumbrance cap (10 + STR items). Higher STR → fewer market trips → faster XP. Same PQ lever, kept. |
| CON | Kill-bar duration down a little; wounds accrue slower |
| DEX | Rare-drop chance |
| INT | Spellbook: new spell on some quest rewards / levels (spells are collected, not cast by the player) |
| WIS | Quest rewards skew toward stats instead of junk |
| CHA | Vendor prices (sell gold up, buy cost down) |

### Needs (0–100)

| Need | While a kill/road/sell task runs | If it hits the floor |
| --- | --- | --- |
| Hunger (or ichor) | slow drain | next task becomes **Camp** |
| Rest | drain; faster in hard fields | Camp |
| Morale | slow; extra if wounded | kill bars take longer (×1.5), they still go |
| Wounds | up on a lost kill | at 100 → **Downed** |

Care is optional. A well-fed adventurer is faster Progress Quest. A starving one camps until the hunger bar recovers (slow) or you feed them. IMU shake and tap-figure = pep (morale). Bottom arc: Feed, Camp, Bag, Fields.

### Kill task

No action bar. A kill is a timer, not a round of HP.

Duration scales with field band vs level, reduced by CON/gear, increased by low morale. Finish: always “win” for v1 loot/XP (PQ style — dying is rare). A small chance of a **bad fight**: extra wounds, no XP, still a pathetic drop. Bad-fight rate rises if the field is far above their level.

On-screen, the figure fights a silhouette until the bar completes. Cosmetic; catch-up does not play animations.

### Catch-up

- Source of truth: `last_tick_unix` vs PCF85063 now.
- Cap: **12 hours**. Extra discarded. Overnight yes; two-week holiday no.
- Apply remaining duration on the current task, then complete whole tasks in a loop until time runs out. Collapse consecutive **Sell** ticks so an 8-hour wake is milliseconds, not 500 UI sells.
- Clock rollback: no progress; clamp `last_tick_unix` forward. Never double-pay.

### Fields (killing grounds)

PQ has “the killing fields.” We have a short ring of named fields. Default: they stay on the current field forever (PQ). Optional check-in: send them to another.

| Field | Band | Note |
| --- | --- | --- |
| Greenroad | 1–5 | hatch default |
| Ironpit | 4–10 | |
| Moonwood | 8–14 | |
| Barrow | 12–18 | night bonus |
| Ashfen | 16–22 | end of v1 |

Too hard: more bad fights, more wounds. Too easy: XP trickle, still loot.

### Encumbrance, market, gear

Inventory is a list, gold on top (PQ). Each non-gold item is 1 cubit. Cap = 10 + STR. At cap → road to market.

Sell one item per task. Items with an “of …” suffix are worth more. After gold-only remains: if they can afford a better **equipment** slot, buy; else road to fields.

Equipment slots: weapon, head, body, hands, feet, trinket. Each item carries a `look` code. The round figure is a paper doll: empty slot = skin; equipped helm/mail/plate/boots/weapon replace that layer. Strip or replace a piece and the silhouette changes the same frame. Buy/quest/level may replace a slot if the new item’s score is higher.

**Procedural names** (PQ algorithm, our word lists): pick a level-matched base (Stick → Shiv → Longsword → …), spend the level-gap on up to two adjectives, remainder becomes `+N`. Example: `+4 pitted iron mail`. Word lists are original; do not copy Progress Quest’s files.

### Quests and plot

Always one quest. Types: **Exterminate**, **Deliver**, **Seek**, **Placate** (MMO verbs, not PQ’s joke log). Exterminate N of a monster family; kills of that family count extra. At 100%: reward is one of stat bump, new spell, equipment, or a valuable named item. Then the next quest is assigned immediately.

**Plot** is a checklist of acts (Act I …). Quest completions fill the plot bar. A finished act is a checkmark and a one-line epitaph on the sheet (“The barrow went quiet”). No cutscenes, no dialogue trees.

**Spellbook:** names collect on the sheet. The player never casts. INT/level/quest rewards add lines. Richer than PQ: spell count shaves a little off kill duration (they are “using” them).

### XP

Exponential, PQ-shaped, pocket-paced: early levels in minutes to hours; the tail is long but not “years to 100” satire. Level-up: stats bump (weighted), maybe a spell, maybe a slot upgrade.

### Life, not permadeath

Downed = downed task until bandaged (or a long slow crawl in catch-up). Hunger+rest at 0 for many hours → **fade**, new whelp, one heirloom kept. Not a graveyard run.

### Party and raids (later, not v0)

The hatchling is the heart. After plot and gear have somewhere to go, they can **find** companions on the road (up to four including the heart) and run **raids** — long tasks on harder fields. That is FEATURES v3.1–v3.2. v0 does not spawn a roster.

### What v1 is not

- Warcraft IP, UI, or look-alikes
- Progress Quest assets, joke races, or a Windows-form clone
- Multiplayer / leaderboards (PQ had them; we do not in v1)
- Tap-to-combat, talent calculator, housing
- Xiaozhi / Brookesia launcher
- Tamagotchi P1 ROM
- 1.75C or other panel firmware
- A day-one party of alts you roll at hatch

## API / Interface Surface

No network API in v1.

**Host / device boundary**

- `tami_sim_catchup(adv, now_unix, rng) -> report` — remaining ms on current task + completed tasks; minutes applied, kills, gold, items, quests, acts, camped/downed.
- `tami_sim_feed / camp / bandage / pep / set_field`
- NVS blob: versioned `adventurer_v1` (fixed struct, migrate by version byte).

**Desk simulator** (`make sim` → `build/tami-sim`) — the agent and the human run this on a computer. No board, no IDF. Commands: `sheet`, `tick <sec>`, `hours <n>`, `feed`, `pep`, `camp`, `field <name>`, `watch`. `--hours 8 --people orc` for headless. The device firmware later calls the same `tami_catchup`.

**Serial debug (USB CDC)** — same verbs as the desk sim. Not a user-facing feature.

## UI / UX Guidelines

The glass is a round 466×466 AMOLED. Content lives in a ~420 px circle; corners do not exist.

**Home (the PQ screen, round):** living figure in the field; **one wide current-task bar** with the act line (“Executing 4 bristled gnolls”); XP ring; encumbrance pip; quest pip. Needs are a faint inner ring, not a second game. No hamburger.

**Sheet (swipe up):** stats, equipment slots, inventory list (newest highlighted), spellbook, quest, plot checkmarks. This is the richer character sheet PQ had as a Windows layout.

**Check-in card (after catch-up):** “8h — 22 kills, quest done, +3 items, sold a haul, hungry.” Dismiss with a tap. Camped/downed takes the card first.

**Gestures**

| Input | Action |
| --- | --- |
| Tap figure | pep |
| Swipe up | sheet |
| Swipe down | clock + brightness |
| Swipe sideways | fields (optional send) |
| Bottom arc | Feed, Camp, Bag, Fields |
| Shake (IMU) | pep |
| PWR short | screen off |
| PWR long | power off (RTC lives) |

AMOLED burn-in: dim after idle on the home view; pixel-shift the figure a few px; never leave a static HUD at full brightness.

First boot: set clock, pick people, hatch animation, Greenroad. No Wi-Fi wizard.

## Visual Style Guide

- Original painterly-pixel or chunky illustrated sprites, not WoW 3D models, not Pokémon mystery-dungeon sheets.
- Four people must be distinguishable in silhouette on a 200 px figure.
- Armor is read at a glance: stick vs spear vs plate vs a glowing trinket. Gear on/off is the dress-up loop.
- Zone palettes: Greenroad warm green, Ironpit rust, Moonwood teal dusk, Barrow cold violet, Ashfen grey-ember.
- UI chrome is metal-and-ink, not the WoW stone-and-gold panel, not LVGL default widgets left raw.
- One typeface for numbers (tabular), one for names. No “Friz Quadrata.”

## Development Phases

**Phase 0 — Desk sim.** `app/` + `make test` + `build/tami-sim` + `build/tami-gfx` (466×466 round SDL2). Current-task state machine, encumbrance → market, catch-up cap. No ESP-IDF. Roadmap: `docs/FEATURES.md`.

**Phase 0b — Board.** Display (CO5300 + 6 px gap), touch, RTC, AXP2101, a non-black screen. Restore image still in `backups/`. Device is a new front-end on the same `app/`.

**Phase 2 — Hatchling.** One people, Greenroad, home view with a real task bar, feed, screen-off catch-up on hardware. Smallest version worth shipping.

**Phase 3 — Progress Quest complete.** Procedural names, six gear slots, quests, plot acts, spellbook, stats that matter, five fields, check-in card, downed/fade.

**Phase 4 — Richer.** IMU pep, ES8311 stings, burn-in dim, biome day/night, sell-one-by-one animation when the screen is on.

Exit of Phase 2 is the first firmware we flash in place of Brookesia.

## Coding Conventions

- C11. `app/` files are `-Werror` clean with no `esp_` includes.
- Catch-up and loot RNG take an explicit `rng` pointer; no `esp_random()` inside `app/`.
- Fixed-point or integer math for needs and damage (no float in the sim).
- NVS struct has a `uint16_t version` and is padded; never raw-write an unversioned blob.
- No Pokémon, Warcraft, or third-party sprite rips in `assets/`.
- Tests for: catch-up cap, clock rollback, encumbrance trips market, sell-until-gold, camp when hunger hits 0, quest 100% assigns the next, downed at wounds 100.

## Key Design Decisions Log

1. **Idle RPG + care, not a WoW clone.** User direction 2026-09-20. Generic fantasy peoples; no Blizzard marks.
9. **Progress Quest loop, richer presentation.** User 2026-09-20. Current-task bar, market trips, procedural loot, quests/plot. Stats actually affect duration/drops. Living sprite + optional care. Not PQ’s joke races or zero-player satire as the whole product.
2. **ESP-IDF + LVGL 9, not Arduino_GFX.** PrintSphere proved the stack on this SKU; TamaPoke’s `.ino` is the wrong shape for an OS.
3. **`app/` vs `device/`.** PixelCat / pocket-pet / TamaPoke `pet.cpp` all split rules from bring-up. Catch-up must be unit-tested.
4. **12-hour catch-up cap.** Overnight yes; skip-a-month no.
5. **Simulation state in NVS only.** Polymo: never put the sim on SD.
6. **One adventurer, four peoples, four callings, five zones in v1.** Roster and world maps wait.
7. **This SKU only.** 1.75C is a different board (no SD, no RTC, different resets, 32 MB flash).
8. **Factory dump stays sacred.** `backups/20260920-144530/full-flash.bin` is the restore image before any Tami flash.
10. **Host simulator first.** User 2026-09-20. `app/` must run on a Mac via `make test` / `make sim` so the loop is built without flashing. Device firmware is a later front-end.
11. **466×466 gfx sim + versioned feature list.** User 2026-09-20. `make gfx` is the panel stand-in. PQ source sits in `vendor/pq-cli` as ideas only. Releases listed in `docs/FEATURES.md`.
12. **Paper-doll gear + later party/raids.** User 2026-09-20. Equipped `look` drives the figure. Found companions and raids are the late-game gate (FEATURES v3.1–v3.2), not v0.
13. **v0.3 desk PQ parity.** Stacks, spell ranks, 11 PQ slots, plot names, passing adventurers, `.tami` save/load. Blob version 2.
14. **v0.4 care on the figure.** Hunger/rest slump, camp fire, downed, fade+heirloom, night tint, check-in card. Blob version 3.
15. **v1.0 hatchling flashed.** 2026-09-20. ESP-IDF 5.5.2 Docker build, merged image at 0x0, tap-to-pep, 1:1 sim time. AXP/RTC/NVS still later. Restore image unchanged in backups/.
16. **v0.5 painted sprites.** 2026-09-20. Imagine 4×4 people×armor, five fields, enemy, campfire. Desk sim blits chroma-keyed PNGs from `assets/sprites`. Device UI still a blob until v1.1.
17. **v0.6 menus.** 2026-09-20. Bottom arc care/bag/map/sheet on desk gfx and device LVGL. Care is optional (feed/pep/bandage/camp). Map sends to a field. Bag and sheet are inspect. Tap figure still peps.
18. **v0.7 pets.** 2026-09-20. Toad/rat/moth/crow strays, max 3, one at heel. Optional care (feed/play/heel/release). Not Pokémon. Blob version 4.
