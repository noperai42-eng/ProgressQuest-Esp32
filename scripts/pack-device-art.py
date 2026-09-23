#!/usr/bin/env python3
"""Pack PNG sprites/fields into RGB565(A8) blobs + LVGL descriptors for the device."""

from __future__ import annotations

from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SPR = ROOT / "assets" / "sprites"
FLD = ROOT / "assets" / "fields"
OUT = ROOT / "device" / "main" / "embed"
HDR = ROOT / "device" / "main" / "art.h"
SRC = ROOT / "device" / "main" / "art.c"

FIG_H = 160
FIELD_S = 233
PORTAL_S = 466
ENEMY_H = 100
PET_H = 56
FIRE_H = 48

PEOPLE = ("human", "orc", "elf", "undead")
TIERS = ("cloth", "leather", "mail", "plate")
FIELDS = ("greenroad", "ironpit", "moonwood", "barrow", "ashfen")
PETS = ("toad", "rat", "moth", "crow")
MOBS = ("gnoll", "boar", "wight", "moth", "pike", "crow", "mire_eel", "ash_rat",
        "wolf", "spider", "ogre", "brigand", "wyrm", "hag")
GEAR = (
    ("wep_stick", 110),
    ("wep_hatchet", 72),
    ("wep_spear", 120),
    ("wep_blade", 90),
    ("helm_cap", 40),
    ("helm_coif", 58),
    ("helm_iron", 52),
    ("shield_round", 48),
    ("shield_tower", 64),
)


def rgb565(r: int, g: int, b: int) -> int:
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def scale_h(im: Image.Image, height: int) -> Image.Image:
    if im.height == height:
        return im
    w = max(1, round(im.width * height / im.height))
    return im.resize((w, height), Image.Resampling.LANCZOS)


def pack_565a8(im: Image.Image) -> tuple[bytes, int, int]:
    im = im.convert("RGBA")
    w, h = im.size
    src = im.tobytes()
    color = bytearray(w * h * 2)
    alpha = bytearray(w * h)
    for i in range(w * h):
        r, g, b, a = src[i * 4 : i * 4 + 4]
        c = rgb565(r, g, b)
        color[i * 2] = c & 0xFF
        color[i * 2 + 1] = c >> 8
        alpha[i] = a
    return bytes(color) + bytes(alpha), w, h


def pack_565(im: Image.Image) -> tuple[bytes, int, int]:
    im = im.convert("RGB")
    w, h = im.size
    src = im.tobytes()
    color = bytearray(w * h * 2)
    for i in range(w * h):
        r, g, b = src[i * 3 : i * 3 + 3]
        c = rgb565(r, g, b)
        color[i * 2] = c & 0xFF
        color[i * 2 + 1] = c >> 8
    return bytes(color), w, h


def write_img(name: str, data: bytes) -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    path = OUT / f"{name}.img"
    path.write_bytes(data)
    print(f"  {path.relative_to(ROOT)}  {len(data)} bytes")


def main() -> int:
    recs: list[tuple[str, str, int, int, str]] = []
    # name, cf_token, w, h, kind
    print("figures")
    for p in PEOPLE:
        for t in TIERS:
            name = f"{p}_{t}"
            im = scale_h(Image.open(SPR / f"{name}.png"), FIG_H)
            data, w, h = pack_565a8(im)
            write_img(name, data)
            recs.append((name, "LV_COLOR_FORMAT_RGB565A8", w, h, "fig"))
    print("gear")
    for name, height in GEAR:
        src = SPR / f"{name}.png"
        if not src.exists():
            print(f"  skip {name}")
            continue
        im = scale_h(Image.open(src), height)
        data, w, h = pack_565a8(im)
        write_img(name, data)
        recs.append((name, "LV_COLOR_FORMAT_RGB565A8", w, h, "gear"))
    print("fields")
    for f in FIELDS:
        im = Image.open(FLD / f"{f}.png").convert("RGB").resize((FIELD_S, FIELD_S), Image.Resampling.LANCZOS)
        data, w, h = pack_565(im)
        write_img(f"field_{f}", data)
        recs.append((f"field_{f}", "LV_COLOR_FORMAT_RGB565", w, h, "field"))
    print("props")
    for name, height, key in (
        ("enemy", ENEMY_H, "enemy"),
        ("campfire", FIRE_H, "fire"),
    ):
        im = scale_h(Image.open(SPR / f"{name}.png"), height)
        data, w, h = pack_565a8(im)
        write_img(name, data)
        recs.append((name, "LV_COLOR_FORMAT_RGB565A8", w, h, key))
    print("mobs")
    for mob in MOBS:
        name = f"mob_{mob}"
        src = SPR / f"{name}.png"
        if not src.exists():
            src = SPR / "enemy.png"
        im = scale_h(Image.open(src), ENEMY_H)
        data, w, h = pack_565a8(im)
        write_img(name, data)
        recs.append((name, "LV_COLOR_FORMAT_RGB565A8", w, h, "mob"))
    for pet in PETS:
        name = f"pet_{pet}"
        im = scale_h(Image.open(SPR / f"{name}.png"), PET_H)
        data, w, h = pack_565a8(im)
        write_img(name, data)
        recs.append((name, "LV_COLOR_FORMAT_RGB565A8", w, h, "pet"))
    print("fx")
    for fx, height in (("fx_slash", 90), ("fx_coins", 80), ("fx_spark", 90), ("fx_dust", 70)):
        im = scale_h(Image.open(SPR / f"{fx}.png"), height)
        data, w, h = pack_565a8(im)
        write_img(fx, data)
        recs.append((fx, "LV_COLOR_FORMAT_RGB565A8", w, h, "fx"))
    print("portal")
    im = Image.open(FLD / "portal_void.png").convert("RGB").resize((PORTAL_S, PORTAL_S), Image.Resampling.LANCZOS)
    data, w, h = pack_565(im)
    write_img("portal_void", data)
    recs.append(("portal_void", "LV_COLOR_FORMAT_RGB565", w, h, "void"))
    ring = Image.open(SPR / "portal_ring.png").convert("RGBA").resize((PORTAL_S, PORTAL_S), Image.Resampling.LANCZOS)
    data, w, h = pack_565a8(ring)
    write_img("portal_ring", data)
    recs.append(("portal_ring", "LV_COLOR_FORMAT_RGB565A8", w, h, "ring"))

    HDR.write_text(
        """#pragma once

#include "tami/sim.h"
#include "lvgl.h"

const lv_image_dsc_t *tami_art_figure(TamiPeople p, int tier);
const lv_image_dsc_t *tami_art_field(TamiField f);
const lv_image_dsc_t *tami_art_pet(TamiPetKind k);
const lv_image_dsc_t *tami_art_enemy(void);
const lv_image_dsc_t *tami_art_mob(uint8_t family);
const lv_image_dsc_t *tami_art_campfire(void);
const lv_image_dsc_t *tami_art_fx_slash(void);
const lv_image_dsc_t *tami_art_fx_coins(void);
const lv_image_dsc_t *tami_art_fx_spark(void);
const lv_image_dsc_t *tami_art_fx_dust(void);
const lv_image_dsc_t *tami_art_portal_void(void);
const lv_image_dsc_t *tami_art_portal_ring(void);
const lv_image_dsc_t *tami_art_weapon(uint8_t look);
const lv_image_dsc_t *tami_art_helm(uint8_t look);
const lv_image_dsc_t *tami_art_shield(uint8_t look);
"""
    )

    lines = [
        '#include "art.h"',
        "",
        "/* Generated by scripts/pack-device-art.py — do not edit. */",
        "",
    ]
    for name, cf, w, h, _kind in recs:
        sym = name
        lines.append(f"extern const uint8_t {sym}_img_start[] asm(\"_binary_{sym}_img_start\");")
        lines.append(f"extern const uint8_t {sym}_img_end[] asm(\"_binary_{sym}_img_end\");")
        stride = w * 2
        dsize = w * h * 2 if cf == "LV_COLOR_FORMAT_RGB565" else w * h * 3
        lines.append(f"static const lv_image_dsc_t dsc_{sym} = {{")
        lines.append("    .header = {")
        lines.append("        .magic = LV_IMAGE_HEADER_MAGIC,")
        lines.append(f"        .cf = {cf},")
        lines.append(f"        .w = {w},")
        lines.append(f"        .h = {h},")
        lines.append(f"        .stride = {stride},")
        lines.append("    },")
        lines.append(f"    .data_size = {dsize},")
        lines.append(f"    .data = {sym}_img_start,")
        lines.append("};")
        lines.append("")

    lines += [
        "static const lv_image_dsc_t *const figures[TAMI_PEOPLE_COUNT][4] = {",
    ]
    for p in PEOPLE:
        names = ", ".join(f"&dsc_{p}_{t}" for t in TIERS)
        lines.append(f"    {{ {names} }},")
    lines += [
        "};",
        "",
        "static const lv_image_dsc_t *const fields[TAMI_FIELD_COUNT] = {",
        "    &dsc_field_greenroad, &dsc_field_ironpit, &dsc_field_moonwood,",
        "    &dsc_field_barrow, &dsc_field_ashfen,",
        "};",
        "",
        "static const lv_image_dsc_t *const pets[TAMI_PET_KIND_COUNT] = {",
        "    &dsc_pet_toad, &dsc_pet_rat, &dsc_pet_moth, &dsc_pet_crow,",
        "};",
        "",
        "const lv_image_dsc_t *tami_art_figure(TamiPeople p, int tier) {",
        "    if (p >= TAMI_PEOPLE_COUNT || tier < 0 || tier > 3) {",
        "        return NULL;",
        "    }",
        "    return figures[p][tier];",
        "}",
        "",
        "const lv_image_dsc_t *tami_art_field(TamiField f) {",
        "    if (f >= TAMI_FIELD_COUNT) {",
        "        return NULL;",
        "    }",
        "    return fields[f];",
        "}",
        "",
        "const lv_image_dsc_t *tami_art_pet(TamiPetKind k) {",
        "    if (k >= TAMI_PET_KIND_COUNT) {",
        "        return NULL;",
        "    }",
        "    return pets[k];",
        "}",
        "",
        "static const lv_image_dsc_t *const mobs[TAMI_FAMILY_COUNT] = {",
        "    &dsc_mob_gnoll, &dsc_mob_boar, &dsc_mob_wight, &dsc_mob_moth,",
        "    &dsc_mob_pike, &dsc_mob_crow, &dsc_mob_mire_eel, &dsc_mob_ash_rat,",
        "    &dsc_mob_wolf, &dsc_mob_spider, &dsc_mob_ogre, &dsc_mob_brigand,",
        "    &dsc_mob_wyrm, &dsc_mob_hag,",
        "};",
        "",
        "const lv_image_dsc_t *tami_art_enemy(void) { return &dsc_enemy; }",
        "const lv_image_dsc_t *tami_art_mob(uint8_t family) {",
        "    if (family >= TAMI_FAMILY_COUNT) {",
        "        return &dsc_enemy;",
        "    }",
        "    return mobs[family];",
        "}",
        "const lv_image_dsc_t *tami_art_campfire(void) { return &dsc_campfire; }",
        "const lv_image_dsc_t *tami_art_fx_slash(void) { return &dsc_fx_slash; }",
        "const lv_image_dsc_t *tami_art_fx_coins(void) { return &dsc_fx_coins; }",
        "const lv_image_dsc_t *tami_art_fx_spark(void) { return &dsc_fx_spark; }",
        "const lv_image_dsc_t *tami_art_fx_dust(void) { return &dsc_fx_dust; }",
        "const lv_image_dsc_t *tami_art_portal_void(void) { return &dsc_portal_void; }",
        "const lv_image_dsc_t *tami_art_portal_ring(void) { return &dsc_portal_ring; }",
        "",
        "const lv_image_dsc_t *tami_art_weapon(uint8_t look) {",
        "    switch (look) {",
        "    case TAMI_LOOK_HATCHET:",
        "    case TAMI_LOOK_PICK:",
        "        return &dsc_wep_hatchet;",
        "    case TAMI_LOOK_SPEAR:",
        "    case TAMI_LOOK_POLE:",
        "        return &dsc_wep_spear;",
        "    case TAMI_LOOK_BLADE:",
        "    case TAMI_LOOK_SHIV:",
        "        return &dsc_wep_blade;",
        "    default:",
        "        return &dsc_wep_stick;",
        "    }",
        "}",
        "const lv_image_dsc_t *tami_art_helm(uint8_t look) {",
        "    if (look == TAMI_LOOK_CAP) {",
        "        return &dsc_helm_cap;",
        "    }",
        "    if (look == TAMI_LOOK_COIF) {",
        "        return &dsc_helm_coif;",
        "    }",
        "    return &dsc_helm_iron;",
        "}",
        "const lv_image_dsc_t *tami_art_shield(uint8_t look) {",
        "    if (look == TAMI_LOOK_SHIELD_TOWER) {",
        "        return &dsc_shield_tower;",
        "    }",
        "    return &dsc_shield_round;",
        "}",
        "",
    ]
    SRC.write_text("\n".join(lines))
    print(f"wrote {HDR.relative_to(ROOT)} {SRC.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
