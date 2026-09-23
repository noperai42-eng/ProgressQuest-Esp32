#!/usr/bin/env python3
"""Chroma-key Imagine JPEGs into engine-ready PNGs + a labeled contact sheet."""

from __future__ import annotations

import os
import shutil
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = Path(__file__).resolve().parents[1]
IMG = Path(
    os.environ.get(
        "TAMI_IMG_DIR",
        "/Users/nope/.grok/sessions/%2FUsers%2Fnope%2FCode%2FTami/"
        "01a0bf45-2c8d-7521-8e72-78bf8fa64688/images",
    )
)
SPR = ROOT / "assets" / "sprites"
FLD = ROOT / "assets" / "fields"
REF = ROOT / "assets" / "art" / "refs"
SHOT = ROOT / "docs" / "screenshots"

# Canon Imagine map. Discarded hybrids: 6, 8, 10.
# 52+ unarmed paper-doll bodies (no staff, bare head) + gear overlays.
FIGS = {
    "orc_cloth": 52,
    "undead_cloth": 53,
    "elf_cloth": 54,
    "human_cloth": 57,
    "orc_plate": 59,
    "human_leather": 60,
    "orc_mail": 61,
    "human_mail": 62,
    "human_plate": 63,
    "orc_leather": 64,
    "elf_plate": 65,
    "undead_leather": 66,
    "elf_mail": 67,
    "elf_leather": 68,
    "undead_plate": 69,
    "undead_mail": 70,
}
# Front-facing paper-doll overlays on pure magenta (80–90).
GEAR = {
    "wep_stick": (83, 220),
    "wep_hatchet": (80, 180),
    "wep_spear": (87, 240),
    "wep_blade": (81, 200),
    "helm_cap": (89, 90),
    "helm_coif": (85, 120),
    "helm_iron": (82, 110),
    "shield_round": (88, 110),
    "shield_tower": (86, 140),
}
FIELDS = {
    "greenroad": 25,
    "ironpit": 24,
    "moonwood": 21,
    "barrow": 23,
    "ashfen": 26,
}
ENEMY = 22
MOBS = {
    "mob_gnoll": 42,
    "mob_boar": 22,
    "mob_wight": 44,
    "mob_moth": 39,
    "mob_pike": 41,
    "mob_crow": 40,
    "mob_mire_eel": 45,
    "mob_ash_rat": 43,
    "mob_wolf": 50,
    "mob_spider": 51,
    "mob_ogre": 49,
    "mob_brigand": 48,
    "mob_wyrm": 47,
    "mob_hag": 46,
}
FIRE = 27  # green-screen retry of 20
PETS = {
    "pet_toad": 28,
    "pet_rat": 29,
    "pet_crow": 30,
    "pet_moth": 31,
}
FX = {
    "fx_coins": 32,
    "fx_slash": 33,
    "fx_dust": 34,
    "fx_spark": 35,
}
PORTAL_VOID = 36
PORTAL_RING = 37

PEOPLE = ("human", "orc", "elf", "undead")
TIERS = ("cloth", "leather", "mail", "plate")

PANEL = 466
FIG_H = 360
PAD = 12


def dist2(r: int, g: int, b: int, cr: int, cg: int, cb: int) -> int:
    dr, dg, db = r - cr, g - cg, b - cb
    return dr * dr + dg * dg + db * db


def sample_bg(im: Image.Image) -> tuple[int, int, int]:
    """Median RGB of four 16×16 corners — Imagine's 'magenta' is actually dusty rose."""
    rgb = im.convert("RGB")
    w, h = rgb.size
    px = rgb.load()
    vals: list[tuple[int, int, int]] = []
    for ox, oy in ((0, 0), (w - 16, 0), (0, h - 16), (w - 16, h - 16)):
        for y in range(oy, oy + 16):
            for x in range(ox, ox + 16):
                vals.append(px[x, y])
    vals.sort()
    return vals[len(vals) // 2]


def chroma_flood(im: Image.Image, hard: int = 42) -> Image.Image:
    """Drop studio background by flooding from the frame edge. Interior skin stays."""
    from collections import deque

    rgb = im.convert("RGB")
    w, h = rgb.size
    kr, kg, kb = sample_bg(im)
    src = rgb.tobytes()
    hard2 = hard * hard
    vis = bytearray(w * h)
    q: deque[int] = deque()

    def try_push(i: int, x: int, y: int) -> None:
        if vis[i]:
            return
        r, g, b = src[i * 3], src[i * 3 + 1], src[i * 3 + 2]
        if dist2(r, g, b, kr, kg, kb) > hard2:
            return
        vis[i] = 1
        q.append(i)

    for x in range(w):
        try_push(x, x, 0)
        try_push((h - 1) * w + x, x, h - 1)
    for y in range(h):
        try_push(y * w, 0, y)
        try_push(y * w + (w - 1), w - 1, y)
    while q:
        i = q.popleft()
        x, y = i % w, i // w
        if x > 0:
            try_push(i - 1, x - 1, y)
        if x + 1 < w:
            try_push(i + 1, x + 1, y)
        if y > 0:
            try_push(i - w, x, y - 1)
        if y + 1 < h:
            try_push(i + w, x, y + 1)
    out = bytearray(w * h * 4)
    for i in range(w * h):
        r, g, b = src[i * 3], src[i * 3 + 1], src[i * 3 + 2]
        o = i * 4
        if vis[i]:
            out[o : o + 4] = b"\x00\x00\x00\x00"
        else:
            out[o] = r
            out[o + 1] = g
            out[o + 2] = b
            out[o + 3] = 255
    return Image.frombytes("RGBA", (w, h), bytes(out))


def chroma(im: Image.Image, keys: list[tuple[int, int, int]] | None = None, hard: int = 36, soft: int = 58) -> Image.Image:
    """RGB JPEG → RGBA. Keys default to this image's sampled corner color."""
    im = im.convert("RGB")
    w, h = im.size
    src = im.tobytes()
    if not keys:
        keys = [sample_bg(im)]
    kr, kg, kb = keys[0]
    green_screen = kg > 150 and kr < 80
    out = bytearray(w * h * 4)
    hard2 = hard * hard
    soft2 = soft * soft
    span = float(soft2 - hard2) if soft2 > hard2 else 1.0
    for i in range(w * h):
        r, g, b = src[i * 3], src[i * 3 + 1], src[i * 3 + 2]
        d = min(dist2(r, g, b, k0, k1, k2) for k0, k1, k2 in keys)
        keyed = d <= soft2
        if green_screen:
            keyed = keyed and g > 140 and r < 90
        else:
            # dusty-rose studio is pink (r high, b high, g mid). Olive skin is
            # yellow-green (b low). Grey steel is unsaturated.
            keyed = keyed and r > 150 and b > 118 and r > g + 4
        if not keyed:
            a = 255
        elif d <= hard2:
            a = 0
        else:
            a = int(255.0 * (d - hard2) / span)
        o = i * 4
        if a == 0:
            out[o : o + 4] = b"\x00\x00\x00\x00"
        else:
            out[o] = r
            out[o + 1] = g
            out[o + 2] = b
            out[o + 3] = a
    rgba = Image.frombytes("RGBA", (w, h), bytes(out))
    a = rgba.split()[3]
    a_bin = a.point(lambda v: 255 if v > 40 else 0)
    eroded = a_bin.filter(ImageFilter.MinFilter(3))
    return Image.merge(
        "RGBA",
        (
            rgba.getchannel("R"),
            rgba.getchannel("G"),
            rgba.getchannel("B"),
            Image.composite(a, eroded, a_bin),
        ),
    )


def bbox_opaque(im: Image.Image, thresh: int = 16) -> tuple[int, int, int, int]:
    a = im.split()[3]
    box = a.point(lambda v: 255 if v > thresh else 0).getbbox()
    if not box:
        return (0, 0, im.width, im.height)
    return box


def crop_pad(im: Image.Image, pad: int = PAD) -> Image.Image:
    x0, y0, x1, y1 = bbox_opaque(im)
    x0 = max(0, x0 - pad)
    y0 = max(0, y0 - pad)
    x1 = min(im.width, x1 + pad)
    y1 = min(im.height, y1 + pad)
    return im.crop((x0, y0, x1, y1))


def scale_h(im: Image.Image, height: int) -> Image.Image:
    if im.height == height:
        return im
    w = max(1, int(round(im.width * height / im.height)))
    return im.resize((w, height), Image.Resampling.LANCZOS)


def save(im: Image.Image, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    im.save(path, "PNG")
    print(f"  {path.relative_to(ROOT)}  {im.size[0]}x{im.size[1]}")


def process_fig(name: str, n: int) -> Image.Image:
    src = IMG / f"{n}.jpg"
    if not src.exists():
        raise FileNotFoundError(src)
    REF.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, REF / f"{name}.jpg")
    keyed = chroma_flood(Image.open(src))
    sprite = eat_rose_fringe(scale_h(crop_pad(keyed), FIG_H))
    save(sprite, SPR / f"{name}.png")
    return sprite


def drop_magenta(im: Image.Image) -> Image.Image:
    """Punch leftover / enclosed magenta (visor holes) that flood-fill misses."""
    im = im.convert("RGBA")
    px = im.load()
    w, h = im.size
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if a < 8:
                continue
            if r > 180 and b > 160 and g < 100:
                px[x, y] = (0, 0, 0, 0)
    return im


def punch_dark_hollow(im: Image.Image) -> Image.Image:
    """Leather-cap visor cavity is dark brown, not magenta — flood it out."""
    from collections import deque

    im = im.convert("RGBA")
    w, h = im.size
    px = im.load()
    seed = None
    for y in range(int(h * 0.55), int(h * 0.88)):
        x = w // 2
        r, g, b, a = px[x, y]
        if a > 80 and (r + g + b) < 180:
            seed = (x, y)
            break
    if seed is None:
        return im
    q: deque[tuple[int, int]] = deque([seed])
    seen = {seed}
    while q:
        x, y = q.popleft()
        r, g, b, a = px[x, y]
        if a < 8 or (r + g + b) > 170:
            continue
        px[x, y] = (0, 0, 0, 0)
        for dx, dy in ((-1, 0), (1, 0), (0, -1), (0, 1)):
            nx, ny = x + dx, y + dy
            if 0 <= nx < w and 0 <= ny < h and (nx, ny) not in seen:
                seen.add((nx, ny))
                q.append((nx, ny))
    return im


def is_studio_rose(r: int, g: int, b: int) -> bool:
    """Dusty-rose studio bleed. Cream shirts are high-g; peach skin has b << g."""
    return r > 150 and b > 100 and 95 < g < 155 and r > g + 28 and abs(g - b) < 30


def eat_rose_fringe(im: Image.Image, rounds: int = 5) -> Image.Image:
    """Drop rose pixels that touch transparency (armpit / hip pockets)."""
    im = im.convert("RGBA")
    w, h = im.size
    px = im.load()
    for _ in range(rounds):
        victims: list[tuple[int, int]] = []
        for y in range(h):
            for x in range(w):
                r, g, b, a = px[x, y]
                if a < 8 or not is_studio_rose(r, g, b):
                    continue
                edge = False
                for dx, dy in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                    nx, ny = x + dx, y + dy
                    if nx < 0 or ny < 0 or nx >= w or ny >= h or px[nx, ny][3] < 16:
                        edge = True
                        break
                if edge:
                    victims.append((x, y))
        if not victims:
            break
        for x, y in victims:
            px[x, y] = (0, 0, 0, 0)
    return drop_rose_pockets(eat_rose_from_edge(im))


def drop_rose_pockets(im: Image.Image, max_n: int = 2500) -> Image.Image:
    """Studio-rose blobs sealed inside a black outline (armpit / hip)."""
    from collections import deque

    im = im.convert("RGBA")
    w, h = im.size
    px = im.load()
    seen = bytearray(w * h)
    for y in range(h):
        for x in range(w):
            i = y * w + x
            if seen[i]:
                continue
            r, g, b, a = px[x, y]
            if a < 8 or not is_studio_rose(r, g, b):
                continue
            q: deque[tuple[int, int]] = deque([(x, y)])
            seen[i] = 1
            comp = [(x, y)]
            while q:
                cx, cy = q.popleft()
                for dx, dy in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                    nx, ny = cx + dx, cy + dy
                    if nx < 0 or ny < 0 or nx >= w or ny >= h:
                        continue
                    k = ny * w + nx
                    if seen[k]:
                        continue
                    nr, ng, nb, na = px[nx, ny]
                    if na < 8 or not is_studio_rose(nr, ng, nb):
                        continue
                    seen[k] = 1
                    q.append((nx, ny))
                    comp.append((nx, ny))
            if len(comp) <= max_n:
                for cx, cy in comp:
                    px[cx, cy] = (0, 0, 0, 0)
    return im


def eat_rose_from_edge(im: Image.Image) -> Image.Image:
    """Walk in from the frame through transparency and studio-rose only."""
    from collections import deque

    im = im.convert("RGBA")
    w, h = im.size
    px = im.load()
    vis = bytearray(w * h)
    q: deque[int] = deque()

    def can_key(x: int, y: int) -> bool:
        r, g, b, a = px[x, y]
        if a < 16:
            return True
        return is_studio_rose(r, g, b)

    def push(x: int, y: int) -> None:
        if x < 0 or y < 0 or x >= w or y >= h:
            return
        i = y * w + x
        if vis[i] or not can_key(x, y):
            return
        vis[i] = 1
        q.append(i)

    for x in range(w):
        push(x, 0)
        push(x, h - 1)
    for y in range(h):
        push(0, y)
        push(w - 1, y)
    while q:
        i = q.popleft()
        x, y = i % w, i // w
        r, g, b, a = px[x, y]
        if a >= 16:
            px[x, y] = (0, 0, 0, 0)
        for dx, dy in ((-1, 0), (1, 0), (0, -1), (0, 1)):
            push(x + dx, y + dy)
    return im


def process_keyed(
    name: str,
    n: int,
    height: int,
    hard: int = 36,
    soft: int = 58,
    flood: bool = True,
) -> None:
    src = IMG / f"{n}.jpg"
    shutil.copy2(src, REF / f"{name}.jpg")
    if flood:
        keyed = chroma_flood(Image.open(src), hard=max(hard, 64))
    else:
        keyed = drop_magenta(chroma(Image.open(src), hard=hard, soft=soft))
    if name == "helm_cap":
        keyed = punch_dark_hollow(keyed)
    sprite = scale_h(crop_pad(keyed, pad=8), height)
    save(sprite, SPR / f"{name}.png")


def process_field(name: str, n: int) -> None:
    src = IMG / f"{n}.jpg"
    shutil.copy2(src, REF / f"{name}.jpg")
    im = Image.open(src).convert("RGB")
    # Greenroad came with a rounded white frame — crop it off.
    if name == "greenroad":
        w, h = im.size
        m = int(w * 0.045)
        im = im.crop((m, m, w - m, h - m))
    im = im.resize((PANEL, PANEL), Image.Resampling.LANCZOS)
    save(im, FLD / f"{name}.png")


def _circle_mask(r: int) -> Image.Image:
    im = Image.new("L", (PANEL, PANEL), 0)
    ImageDraw.Draw(im).ellipse((233 - r, 233 - r, 233 + r, 233 + r), fill=255)
    return im


def _drop_studio_rose(im: Image.Image) -> Image.Image:
    px = im.load()
    w, h = im.size
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if a < 8:
                continue
            if r > 145 and g > 105 and b > 105 and abs(r - g) < 60 and abs(g - b) < 50:
                px[x, y] = (0, 0, 0, 0)
    return im


def process_portal_void() -> None:
    src = IMG / f"{PORTAL_VOID}.jpg"
    shutil.copy2(src, REF / "portal_void.jpg")
    im = Image.open(src).convert("RGB")
    w, h = im.size
    m = int(min(w, h) * 0.12)
    im = im.crop((m, m, w - m, h - m)).resize((PANEL, PANEL), Image.Resampling.LANCZOS)
    circ = _circle_mask(232)
    black = Image.new("RGB", (PANEL, PANEL), (0, 0, 0))
    rgb = Image.composite(im, black, circ)
    save(rgb, FLD / "portal_void.png")


def process_portal_ring() -> None:
    src = IMG / f"{PORTAL_RING}.jpg"
    shutil.copy2(src, REF / "portal_ring.jpg")
    keyed = _drop_studio_rose(chroma(Image.open(src), hard=44, soft=74))
    box = bbox_opaque(keyed)
    cropped = keyed.crop(box)
    size = max(cropped.size)
    scale = (PANEL - 6) / float(size)
    nw = max(1, int(round(cropped.width * scale)))
    nh = max(1, int(round(cropped.height * scale)))
    fitted = cropped.resize((nw, nh), Image.Resampling.LANCZOS)
    canvas = Image.new("RGBA", (PANEL, PANEL), (0, 0, 0, 0))
    canvas.paste(fitted, ((PANEL - nw) // 2, (PANEL - nh) // 2), fitted)
    px = canvas.load()
    cx, cy = 233.0, 233.0
    inner, outer = 174.0, 232.0
    inner2, outer2 = inner * inner, outer * outer
    for y in range(PANEL):
        dy = y + 0.5 - cy
        for x in range(PANEL):
            dx = x + 0.5 - cx
            d2 = dx * dx + dy * dy
            if d2 < inner2 or d2 > outer2:
                px[x, y] = (0, 0, 0, 0)
    save(canvas, SPR / "portal_ring.png")


def disc_mask() -> None:
    im = Image.new("RGBA", (PANEL, PANEL), (0, 0, 0, 255))
    d = ImageDraw.Draw(im)
    # Match host/gfx.c CX=233 CY=233 CR=232.
    d.ellipse((1, 1, 465, 465), fill=(0, 0, 0, 0))
    save(im, SPR / "disc_mask.png")


def font(size: int) -> ImageFont.ImageFont:
    for p in (
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/Supplemental/Arial Unicode.ttf",
        "/Library/Fonts/Arial.ttf",
    ):
        if os.path.exists(p):
            return ImageFont.truetype(p, size)
    return ImageFont.load_default()


def contact_sheet(sprites: dict[str, Image.Image]) -> None:
    cell = 280
    label_h = 28
    gap = 14
    margin = 28
    cols, rows = 4, 4
    W = margin * 2 + cols * cell + (cols - 1) * gap
    H = margin * 2 + 48 + rows * (cell + label_h) + (rows - 1) * gap
    sheet = Image.new("RGB", (W, H), (18, 14, 16))
    draw = ImageDraw.Draw(sheet)
    title_f = font(22)
    lab_f = font(15)
    draw.text((margin, 16), "Tami — people × armor", font=title_f, fill=(230, 214, 180))
    for r, people in enumerate(PEOPLE):
        for c, tier in enumerate(TIERS):
            key = f"{people}_{tier}"
            im = sprites[key].convert("RGBA")
            # Fit into cell, keep aspect, bottom-align (feet).
            scale = min(cell / im.width, cell / im.height)
            nw, nh = max(1, int(im.width * scale)), max(1, int(im.height * scale))
            fitted = im.resize((nw, nh), Image.Resampling.LANCZOS)
            x = margin + c * (cell + gap)
            y = margin + 48 + r * (cell + label_h + gap)
            # dark well
            draw.rounded_rectangle((x, y, x + cell, y + cell), radius=12, fill=(28, 22, 26))
            px = x + (cell - nw) // 2
            py = y + cell - nh - 6
            sheet.paste(fitted, (px, py), fitted)
            draw.text(
                (x + 8, y + cell + 4),
                f"{people}  {tier}",
                font=lab_f,
                fill=(190, 176, 150),
            )
    out = SHOT / "armor-set.png"
    save(sheet.convert("RGB"), out)
    save(sheet.convert("RGB"), ROOT / "assets" / "art" / "contact-sheet.png")


def fringe_existing_figures() -> None:
    for people in PEOPLE:
        for tier in TIERS:
            path = SPR / f"{people}_{tier}.png"
            if not path.exists():
                continue
            cleaned = eat_rose_fringe(Image.open(path))
            save(cleaned, path)


def main() -> int:
    mode = sys.argv[1] if len(sys.argv) > 1 else "all"
    if not IMG.is_dir() and mode != "--fringe":
        print("missing Imagine dir:", IMG, file=sys.stderr)
        return 1
    SPR.mkdir(parents=True, exist_ok=True)
    FLD.mkdir(parents=True, exist_ok=True)
    REF.mkdir(parents=True, exist_ok=True)
    SHOT.mkdir(parents=True, exist_ok=True)

    if mode == "--fringe":
        print("fringe")
        fringe_existing_figures()
        print("ok")
        return 0

    if mode in ("all", "--gear"):
        print("gear")
        for name, (n, h) in GEAR.items():
            process_keyed(name, n, height=h, hard=28, soft=52, flood=False)
        if mode == "--gear":
            print("ok")
            return 0

    if mode == "--figures":
        print("figures")
        sprites = {}
        for name, n in FIGS.items():
            sprites[name] = process_fig(name, n)
        contact_sheet(sprites)
        print("ok")
        return 0

    print("figures")
    sprites = {}
    for name, n in FIGS.items():
        sprites[name] = process_fig(name, n)

    print("props")
    process_keyed("enemy", ENEMY, height=280, hard=40, soft=70)
    print("mobs")
    mob_sprites: dict[str, Image.Image] = {}
    for name, n in MOBS.items():
        process_keyed(name, n, height=280, hard=40, soft=70)
        mob_sprites[name] = Image.open(SPR / f"{name}.png")
    # 2×4 contact of the field-beasts
    cell, gap, lab_h = 140, 10, 20
    cols, rows = 4, 4
    sheet = Image.new("RGB", (gap + cols * (cell + gap), gap + 36 + rows * (cell + lab_h + gap)), (18, 14, 16))
    draw = ImageDraw.Draw(sheet)
    try:
        font = ImageFont.truetype("/System/Library/Fonts/Supplemental/Arial.ttf", 14)
    except OSError:
        font = ImageFont.load_default()
    draw.text((gap, 10), "Tami — field beasts", font=font, fill=(230, 214, 180))
    order = list(MOBS.keys())
    for i, name in enumerate(order):
        r, c = divmod(i, cols)
        im = mob_sprites[name].convert("RGBA")
        scale = min(cell / im.width, cell / im.height)
        nw, nh = max(1, int(im.width * scale)), max(1, int(im.height * scale))
        fitted = im.resize((nw, nh), Image.Resampling.LANCZOS)
        x = gap + c * (cell + gap)
        y = gap + 36 + r * (cell + lab_h + gap)
        draw.rounded_rectangle((x, y, x + cell, y + cell), radius=10, fill=(28, 22, 26))
        sheet.paste(fitted, (x + (cell - nw) // 2, y + cell - nh - 4), fitted)
        draw.text((x + 6, y + cell + 2), name.replace("mob_", "").replace("_", "-"), font=font, fill=(190, 176, 150))
    save(sheet, SHOT / "mobs.png")
    save(sheet, ROOT / "assets" / "art" / "mobs-sheet.png")
    process_keyed("campfire", FIRE, height=180, hard=40, soft=70)
    print("pets")
    for name, n in PETS.items():
        process_keyed(name, n, height=180)
    print("fx")
    for name, n in FX.items():
        process_keyed(name, n, height=180)

    print("fields")
    for name, n in FIELDS.items():
        process_field(name, n)

    print("portal")
    process_portal_void()
    process_portal_ring()

    print("mask + sheet")
    disc_mask()
    contact_sheet(sprites)
    print("ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
