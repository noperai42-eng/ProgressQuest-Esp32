# Tami — feature list by release

Progress Quest (Eric Fredricksen, 2002; source 2011) is the mechanical north star.
A local copy of the faithful `pq-cli` port lives in `vendor/pq-cli` for ideas only.
Tami does **not** ship PQ joke races, PQ spell names, or PQ word lists.

Status: **have** = in `app/` + desk sims now. Everything else is planned.
Device firmware is a front-end on the same `app/`. The hatchling is the heart;
found companions and raids are late-game, not a day-one roster.

---

## PQ original (what we are remixing)

Taken from `vendor/pq-cli` (MIT port of the Delphi/JS game):

| Feature | PQ | Tami stance |
| --- | --- | --- |
| Zero-player after “Sold!” | yes | default: they run themselves |
| Current-task bar | kill / road market / sell / buy / road fields | same, plus camp / downed |
| Encumbrance = 10+STR cubits | yes | yes |
| Gold on top of inventory | yes | gold separate, pack is items |
| Stacking inventory qty | yes | **not yet** (unique lines) |
| Sell one item per bar | yes | yes (catch-up may collapse) |
| Buy a random better slot | yes | yes, best-gain slot |
| 11 equipment slots (weapon, shield, helm, hauberk, brassairts, vambraces, gauntlets, gambeson, cuisses, greaves, sollerets) | yes | **6** in v0 (weapon, head, body, hands, feet, trinket) |
| Procedural loot: base + adjectives + leftover `+N` | yes | yes, original words |
| Monster “Executing N adj type” | yes | yes, original families |
| Quest types Exterminate / Deliver / Seek / Placate | yes | yes |
| Quest log history (last 100) | yes | **current quest only** until v0.4 |
| Plot acts + Prologue, Roman numerals | yes | act number + bar; named acts in v2.2 |
| Spellbook with **ranks** that stack | yes | names collect; ranks in v0.3 |
| Stats STR CON DEX INT WIS CHA + HP Max + MP Max | 8 | 6 primes; HP/MP later |
| Stats mostly flavor except STR | PQ joke | **ours matter** (duration, drops, prices) |
| Level-up time ≈ 20×level minutes | yes | pocket curve (faster early) |
| Win 2 stats + 1 spell on level | yes | stats + maybe spell |
| Traits / impressive titles | yes | v2.2 |
| Save roster, multiple characters | yes | one heart now; **found party** in v3.1 |
| Online leaderboard / auth | yes | **never in v1**; maybe never |
| Death | rare | downed → fade, not a graveyard |
| Living sprite, round AMOLED, care | no | the richer part |

---

## v0.1 — Desk CLI  **have**

- Host-only C sim (`app/`), no ESP-IDF
- `make test` / `make sim` → `build/tami-sim`
- Hatch: people (human/orc/elf/undead) + calling (warrior/ranger/mage/rogue)
- Current-task state machine
- 12-hour catch-up cap, clock-rollback clamp
- Encumbrance → market → sell → buy → fields
- Procedural item/monster names (original lists)
- Quests + plot bar
- Needs (hunger/rest/morale/wounds) throttle and can camp
- Feed / pep / camp / bandage / set field
- Interactive `tick` / `hours`

## v0.2 — Round 466×466 graphical desk  **have**

- SDL2 window, logical **466×466**, circular clip (the panel)
- Same `app/` as the CLI
- Home: figure + enemy silhouette + **one task bar** + XP ring + need pips
- **Paper doll:** weapon / helm / body / hands / feet / trinket `look` codes; empty slot shows skin; upgrades change the silhouette
- Field palettes (Greenroad / Ironpit / Moonwood / Barrow / Ashfen)
- Sheet overlay (stats, gear, pack, spells, quest, plot)
- Realtime sim rate (watch the bar fill) + `--hours` then screenshot
- Keys: feed, pep, camp, bandage, sheet, fields, speed
- `build/tami-gfx --screenshot path.bmp` for headless frames
- No device flash

## v0.3 — PQ parity on the desk  **have**

- Inventory **quantities** (stack “mire helm ×4”)
- Spell **ranks** (Gloom II) instead of duplicate names
- Quest **history** list on the sheet (last 12)
- Named plot acts: Prologue, Act I–V with one-line epitaphs
- 11 equipment slots (PQ set), still original item names
- HP/MP max on the sheet (derived from CON/INT)
- Level-up fanfare on the task bar (“Mog is now level 11”)
- Passing-adventurer rare kill (“Executing a passing ranger”)
- Save/load a desk `.tami` blob (same struct as future NVS)

## v0.4 — Care that feels like a pet  **have**

- Hunger/rest drain visible on the figure (slump, sit, glow)
- Camp animation, downed crawl
- Fade + heirloom (one item survives)
- Night/day tint from sim clock (Barrow night bonus visible)
- Check-in card after `--hours` / after unpausing
- Host tests for stack qty, spell ranks, save round-trip

## v0.5 — Painted figure  **have**

- Imagine 4×4 people × armor set (cloth / leather / mail / plate), edit-chained from four hatchling bases
- Desk sim blits chroma-keyed sprites instead of the ellipse paper-doll
- Five field paintings, bristled field-beast, campfire
- Armor tier follows the best equipped body/head/limb look (`--armor` to preview)
- Contact sheet: `docs/screenshots/armor-set.png`. In-game frames: `docs/screenshots/in-game-*.bmp`
- Stick is still painted into every kit (weapon overlay later). Device UI still a people-colored blob.

## v0.6 — Buttons and menus  **have**

- Bottom arc on the 466 disc: **care / bag / map / sheet**
- **Care:** hunger (ichor if undead), rest, morale, wounds + feed / pep / bandage / camp
- **Bag:** gold, pack, worn gear, newest loot
- **Map:** five fields with level band, here/hard/easy; tap to send
- **Sheet:** stats, plot, quest, spells, worn slots
- Tap figure still peps. Esc closes a menu. Device hatchling has the same four buttons + overlays
- Screens: `docs/screenshots/menu-*.png`

## v0.7 — Pets  **have**

- Folklore strays (toad, rat, moth, crow) find you on the road — not a catch roster
- Max 3. One at heel sits on the home disc and pays a small bonus (forage / crumbs / rest / XP)
- Manage from the **pets** button: feed, play, heel, let go. Hungry too long → they wander
- Blob version 4. Desk `--pet toad` to preview; `--menu pets`

## v0.8 — Task motion + cutscenes  **have**

- Figure pose follows the current-task bar (wind-up/lunge on kill, walk on the road, bow at market, sit at camp)
- Mini cutscenes on kill / loot / sell / camp / level / pet: slash, coins, spark, dust
- Device runs a 40ms anim timer so motion is live, not 1s snaps

## v0.9 — Readable HUD + fat taps  **have**

- HUD meta chip shows **lv N  xx%  field**; gold XP bar + rim arc fill toward the next level
- Sheet is a two-column stat card (HP/MP, XP, plot, quest, spells) — gear stays in bag
- Cream type sits on solid dark chips, not on the painted field
- Menus use an opaque rounded panel (not a translucent ellipse) so bag/sheet lines stay readable
- Bottom nav is two rows of ~100×48 buttons (care/bag/map, then sheet/pets) so a finger can hit them on the 1.75" disc
- Hatch select: people × calling on a portal void; adventure home sits inside an ornate portal ring. One hatchling, not a roster.
- Screens: `docs/screenshots/menu-*.png`

## v0.10 — Gleam  **have**

- Loot rarity: common / uncommon / rare / legendary (score, sell price, a small kill-bar bonus)
- Elite packs on the road (Ashfen and Barrow-night more often); passing adventurers drop better too
- Rare+ sparkles on the figure (gold motes for a relic); bag names tint and mark `*` / `**`
- Work-day recap names a relic if one is worn. Desk: `--gleam`. USB `story` includes `rares=` `legend=` `elites=` `gleam=`
- Found party is still v3.1 — pets are the only company for now
- Eight field-beasts, one per family (gnoll, boar, wight, moth, pike, crow, mire-eel, ash-rat). Home fight pose and device enemy pick the sprite from the current kill. Desk: `--mob wight`. Elites use the same beast, drawn a little larger on the desk.
- Sheet is two pages: **stats** and **worn** (11 full-width slots, colored by rarity). The figure’s loadout changes with weapon / helm / shield / boots, not just cloth–plate. Live as kills land. **Log** is the story so far. **Map** is gone — tap the HUD field name to travel. **Camp** (was care) is optional hurry-up rest. USB: `log`. Pets **gone** asks once more before banishing.
- Menus pause the fight redraw and the 80ms pose timer so scrolling stays snappy while the sim keeps ticking. Side keys poll at 25ms even at 1×.
- 14 field-beasts. Quests also Rescue / Clear a den / Escort / Recover a relic (two phrasings each). Rare field events: **delve a dungeon**, **storm a camp**, **save a traveler** (plus crawl/open, burn a palisade, cut a merchant free).

## v1.0 — Device hatchling  **have** (first flash)

- ESP-IDF 5.5.2 + LVGL 9.6 + Waveshare BSP 3.0.1 + `esp_lcd_co5300` 2.2.0
- Same `app/` on the 466×466 disc: name, people blob, task bar, XP/pack/gold
- 1 sim second per real second while powered; tap figure to pep; care/bag/map/sheet/pets buttons
- Random people/calling at boot
- Not yet: AXP2101 power button, PCF85063 catch-up, NVS persist, paper-doll layers
- Restore: `./scripts/restore-flash.sh backups/20260920-144530/full-flash.bin`

- ESP-IDF 5.5 + LVGL 9 + Waveshare 1.75 BSP
- CO5300 6 px gap, CST9217, 16 MB flash only
- Same `app/` linked into firmware
- First boot: clock, people, hatch, Greenroad, task bar
- PWR short = screen off; long = AXP2101 off, RTC catch-up
- Factory restore image still in `backups/`
- **Not** Brookesia, **not** Xiaozhi as the product

## v1.1 — Device is the desk

- Round home matches the 466 gfx sim (painted figure, not blob)
- Swipe-up still a later gesture; sheet is a bottom-arc button now
- NVS persist (`adventurer_v1`)
- USB Serial JTAG: `rate N` / `auto` / `tick N` / `hours N` / `status` / `story` / `hatch PEOPLE CALLING` / care verbs. Plugged in defaults to 20×; unplug is 1×. Host: `./scripts/device-cli.py`
- Work-day recap: `tami_format_recap` / USB `story`. Desk: `make paths`. Disc: `./scripts/progress-harness.py --device`

## v1.2 — Pocket reliability

- Light sleep while screen off, sim keeps a 1s tick or catch-up on wake
- Catch-up report card on wake
- Burn-in: dim, pixel-shift, never freeze a full-bright HUD
- Battery pip from AXP2101
- Watchdog-safe display lock (PrintSphere lesson)

## v1.3 — Touch language

- Tap figure = pep
- IMU shake = pep
- Long-press PWR already power; short already screen
- Two-point CST9217 ignored (one finger is enough)
- Touch INT GPIO11, reset GPIO40 (not 1.75C pins)

## v2.0 — Juice

- ES8311 stings: kill done, level-up, market, downed
- Day/night biome from PCF85063
- Walk-to-market and sell-one-by-one animations when the screen is on
- Face-down IMU → screen off (Capsule Radar pattern)

## v2.1 — More peoples (still one living heart)

- Dwarf, goblin as extra peoples (folklore, not Blizzard)
- Two more fields
- Calling-specific weapon bases
- No alts; found party is v3.1

## v2.2 — Titles and traits

- PQ-style traits that actually do something (carry +1, kill bar −5%)
- Impressive title on the sheet (“Mog the Notched”)
- Plot epitaphs per act

## v2.3 — Book

- Spellbook page, quest log page, plot page as swipe destinations
- Bestowals / unique named drops on top of rarity (our list)

## v3.0 — Long tail

- Bigger monster/item tables (still original)
- “Too-hard field” visibly wrecks them
- Heirloom new-game+ (one item, one spell)

## v3.1 — Found companions

- Rare field event: a wanderer of a people/calling waits on the road (not a second hatch)
- Optional recruit, party cap **4** including the hatchling
- Companions idle with you: extra pack, faster kill bar, their own paper-doll gear
- You still **care for the heart** (hunger/rest of the hatchling); companions don’t replace the Tamagotchi
- Sheet: party row of small silhouettes
- No character-select screen, no alts parked in town

## v3.2 — Raids

- Unlock when party ≥ 3 and plot act ≥ III
- A raid is a **long task** (real hours, 12h catch-up still caps a sitting)
- Separate raid fields above Ashfen
- Wipe = all downed, not fade; the heart can still fade from neglect
- Raid loot can be unique looks (still original names)
- This is how the late game grows, not a tap-combat instance

## Explicitly out until we say otherwise

- World of Warcraft names, chrome, models, factions
- PQ joke races (Double Wookiee, Eel Man, …) and PQ spell strings
- Xiaozhi / Brookesia launcher
- Multiplayer, leaderboards, accounts, Wi-Fi required
- Tap-to-combat, talent trees, housing, professions grid
- Tamagotchi P1 ROM / tamalib
- 1.75C / 1.8 / 2.06 binaries
- Simulation state on SD
