# Tami Imagine art

Canonical 4×4 people × armor set the desk sim models after.
Style: stylized 2D handheld game illustration, chunky painterly cel shading, metal-and-ink.

Bases were generated once, then every armor kit was **edit-chained** from that people's cloth hatchling so face, pose, and staff hand stay locked.

| People | Cloth | Leather | Mail | Plate |
| --- | --- | --- | --- | --- |
| human | linen wrap, headband, sandals | cap, jerkin, bracers, boots | chain coif + hauberk, chain boots | visored helm, breastplate, sabatons |
| orc | hide wrap, topknot, tusks | leather cap + cuirass | chain coif (topknot through), ring shirt | rusted great helm + plate |
| elf | leaf wrap, teal hair, barefoot | leather cap + laced jerkin | chain coif (ears through), chain boots | kettle helm (ears through), leaf-etched plate |
| undead | tattered wrap, hollow red eyes | stitched cap, bone-hole jerkin | full ring-mail, face open | cracked visor, red eyes in the slit |

Engine-ready chroma-keyed PNGs: `assets/sprites/{people}_{tier}.png`.
Field paintings: `assets/fields/{greenroad,ironpit,moonwood,barrow,ashfen}.png`.
Contact sheet: `docs/screenshots/armor-set.png`.
JPEG refs: `assets/art/refs/`.

Rebuild sprites from Imagine JPEGs:

```bash
TAMI_IMG_DIR=/path/to/session/images python3 scripts/process-art.py
```

Pets (same style, chroma-keyed): `pet_toad`, `pet_rat`, `pet_moth`, `pet_crow`.
Field-beasts (one per kill family): `mob_gnoll`, `mob_boar`, `mob_wight`, `mob_moth`, `mob_pike`, `mob_crow`, `mob_mire_eel`, `mob_ash_rat`. Contact: `docs/screenshots/mobs.png`.

Known limits: the hatchling stick is painted into every kit (weapon overlays later). Imagine's studio backdrop is dusty rose, not #FF00FF — the processor samples each corner. Campfire is keyed on lime green because the flames were magenta.
