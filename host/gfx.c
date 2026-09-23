#define SDL_MAIN_HANDLED
#include "tami/sim.h"
#include "tami/anim.h"
#include "tami/roster.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <SDL.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-compare"
#include "../vendor/stb_easy_font.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "../vendor/stb_image.h"
#pragma GCC diagnostic pop

#define PANEL 466
#define CX 233
#define CY 233
#define CR 232
#define TIER_COUNT 4

static const char *k_people_file[TAMI_PEOPLE_COUNT] = {"human", "orc", "elf", "undead"};
static const char *k_tier_file[TIER_COUNT] = {"cloth", "leather", "mail", "plate"};
static const char *k_field_file[TAMI_FIELD_COUNT] = {"greenroad", "ironpit", "moonwood", "barrow",
                                                    "ashfen"};
static const char *k_mob_file[TAMI_FAMILY_COUNT] = {
    "gnoll", "boar", "wight", "moth", "pike", "crow", "mire_eel", "ash_rat",
    "wolf",  "spider", "ogre", "brigand", "wyrm", "hag",
};

static char g_asset_root[512] = "assets";
static SDL_Texture *g_fig[TAMI_PEOPLE_COUNT][TIER_COUNT];
static SDL_Texture *g_field_tex[TAMI_FIELD_COUNT];
static SDL_Texture *g_enemy;
static SDL_Texture *g_mob[TAMI_FAMILY_COUNT];
static SDL_Texture *g_fire;
static SDL_Texture *g_disc_mask;
static SDL_Texture *g_fx_slash, *g_fx_coins, *g_fx_spark, *g_fx_dust;
static SDL_Texture *g_wep[4];
static SDL_Texture *g_helm[3];
static SDL_Texture *g_shield[2];
static int g_force_tier = -1;
static int g_sheet_scroll;
static int g_sheet_page;
static int g_gone_arm;
static TamiCut g_cut;
static float g_cut_t;

enum {
    VIEW_HOME = 0,
    VIEW_CARE,
    VIEW_BAG,
    VIEW_LOG,
    VIEW_SHEET,
    VIEW_PETS,
    VIEW_HATCH,
    VIEW_FIELDS,
    VIEW_ROSTER
};
static int g_view = VIEW_HOME;
static int g_hatched = 1;
static int g_hud;
static int g_pick_people = TAMI_PEOPLE_ORC;
static int g_pick_call = TAMI_CALL_WARRIOR;
static int g_create_slot;
static TamiRoster g_roster;
static TamiRng *g_rng;
static TamiAdventurer *g_adv;
static SDL_Texture *g_portal_ring;
static SDL_Texture *g_portal_void;
static char g_toast[64];
static float g_toast_left;
static int g_pet_sel;
static SDL_Texture *g_pet_tex[TAMI_PET_KIND_COUNT];

static const char *k_nav_lab[5] = {"camp", "bag", "log", "sheet", "pets"};
static const char *k_people_lab[4] = {"human", "orc", "elf", "undead"};
static const char *k_call_lab[4] = {"warrior", "ranger", "mage", "rogue"};
static const char *k_care_lab[4] = {"feed", "pep", "bandage", "camp"};
static const char *k_pet_act[4] = {"feed", "play", "heel", "gone"};
static const char *k_pet_file[TAMI_PET_KIND_COUNT] = {"pet_toad", "pet_rat", "pet_moth", "pet_crow"};

static int file_ok(const char *p) {
    return p && access(p, R_OK) == 0;
}

static void find_asset_root(const char *argv0) {
    char cand[8][512];
    int n = 0;
    snprintf(cand[n++], 512, "assets");
    snprintf(cand[n++], 512, "../assets");
    if (argv0) {
        const char *slash = strrchr(argv0, '/');
        if (slash) {
            int dirlen = (int)(slash - argv0);
            snprintf(cand[n++], 512, "%.*s/../assets", dirlen, argv0);
            snprintf(cand[n++], 512, "%.*s/../../assets", dirlen, argv0);
        }
    }
    for (int i = 0; i < n; i++) {
        char probe[640];
        snprintf(probe, sizeof(probe), "%s/sprites/orc_cloth.png", cand[i]);
        if (file_ok(probe)) {
            snprintf(g_asset_root, sizeof(g_asset_root), "%s", cand[i]);
            return;
        }
    }
}

static SDL_Texture *load_png(SDL_Renderer *ren, const char *path) {
    int w = 0, h = 0, comp = 0;
    unsigned char *pix = stbi_load(path, &w, &h, &comp, 4);
    if (!pix) {
        return NULL;
    }
    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    if (!surf) {
        stbi_image_free(pix);
        return NULL;
    }
    memcpy(surf->pixels, pix, (size_t)w * (size_t)h * 4u);
    stbi_image_free(pix);
    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
    SDL_FreeSurface(surf);
    if (tex) {
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    }
    return tex;
}

static int load_art(SDL_Renderer *ren) {
    char path[640];
    int n = 0;
    for (int p = 0; p < TAMI_PEOPLE_COUNT; p++) {
        for (int t = 0; t < TIER_COUNT; t++) {
            snprintf(path, sizeof(path), "%s/sprites/%s_%s.png", g_asset_root, k_people_file[p],
                     k_tier_file[t]);
            g_fig[p][t] = load_png(ren, path);
            if (g_fig[p][t]) {
                n++;
            }
        }
    }
    for (int f = 0; f < TAMI_FIELD_COUNT; f++) {
        snprintf(path, sizeof(path), "%s/fields/%s.png", g_asset_root, k_field_file[f]);
        g_field_tex[f] = load_png(ren, path);
    }
    snprintf(path, sizeof(path), "%s/sprites/enemy.png", g_asset_root);
    g_enemy = load_png(ren, path);
    for (int m = 0; m < TAMI_FAMILY_COUNT; m++) {
        snprintf(path, sizeof(path), "%s/sprites/mob_%s.png", g_asset_root, k_mob_file[m]);
        g_mob[m] = load_png(ren, path);
        if (!g_mob[m]) {
            g_mob[m] = g_enemy;
        }
    }
    snprintf(path, sizeof(path), "%s/sprites/campfire.png", g_asset_root);
    g_fire = load_png(ren, path);
    snprintf(path, sizeof(path), "%s/sprites/disc_mask.png", g_asset_root);
    g_disc_mask = load_png(ren, path);
    snprintf(path, sizeof(path), "%s/sprites/portal_ring.png", g_asset_root);
    g_portal_ring = load_png(ren, path);
    snprintf(path, sizeof(path), "%s/fields/portal_void.png", g_asset_root);
    g_portal_void = load_png(ren, path);
    snprintf(path, sizeof(path), "%s/sprites/fx_slash.png", g_asset_root);
    g_fx_slash = load_png(ren, path);
    snprintf(path, sizeof(path), "%s/sprites/fx_coins.png", g_asset_root);
    g_fx_coins = load_png(ren, path);
    snprintf(path, sizeof(path), "%s/sprites/fx_spark.png", g_asset_root);
    g_fx_spark = load_png(ren, path);
    snprintf(path, sizeof(path), "%s/sprites/fx_dust.png", g_asset_root);
    g_fx_dust = load_png(ren, path);
    for (int k = 0; k < TAMI_PET_KIND_COUNT; k++) {
        snprintf(path, sizeof(path), "%s/sprites/%s.png", g_asset_root, k_pet_file[k]);
        g_pet_tex[k] = load_png(ren, path);
    }
    {
        static const char *wep_n[4] = {"wep_stick", "wep_hatchet", "wep_spear", "wep_blade"};
        static const char *helm_n[3] = {"helm_cap", "helm_coif", "helm_iron"};
        static const char *sh_n[2] = {"shield_round", "shield_tower"};
        for (int i = 0; i < 4; i++) {
            snprintf(path, sizeof(path), "%s/sprites/%s.png", g_asset_root, wep_n[i]);
            g_wep[i] = load_png(ren, path);
        }
        for (int i = 0; i < 3; i++) {
            snprintf(path, sizeof(path), "%s/sprites/%s.png", g_asset_root, helm_n[i]);
            g_helm[i] = load_png(ren, path);
        }
        for (int i = 0; i < 2; i++) {
            snprintf(path, sizeof(path), "%s/sprites/%s.png", g_asset_root, sh_n[i]);
            g_shield[i] = load_png(ren, path);
        }
    }
    fprintf(stderr, "art: %d/16 figures from %s\n", n, g_asset_root);
    return n;
}

static int look_tier(uint8_t look) {
    switch (look) {
    case TAMI_LOOK_PLATE:
    case TAMI_LOOK_HELM:
    case TAMI_LOOK_GAUNTLETS:
    case TAMI_LOOK_GREAVES:
    case TAMI_LOOK_SOLLERET:
    case TAMI_LOOK_BRASSAIRT:
        return 3;
    case TAMI_LOOK_MAIL:
    case TAMI_LOOK_HAUBERK:
    case TAMI_LOOK_COIF:
    case TAMI_LOOK_VAMBRACE:
    case TAMI_LOOK_CUISSE:
        return 2;
    case TAMI_LOOK_LEATHER:
    case TAMI_LOOK_BOOTS:
    case TAMI_LOOK_GLOVES:
    case TAMI_LOOK_CAP:
        return 1;
    default:
        return 0;
    }
}

static int armor_tier(const TamiAdventurer *a) {
    if (g_force_tier >= 0) {
        return g_force_tier;
    }
    int t = 0;
    for (int s = 0; s < TAMI_SLOT_COUNT; s++) {
        if (s == TAMI_SLOT_WEAPON || s == TAMI_SLOT_SHIELD) {
            continue;
        }
        if (tami_item_on(&a->equip[s])) {
            int lt = look_tier(a->equip[s].look);
            if (lt > t) {
                t = lt;
            }
        }
    }
    return t;
}

static void rarity_rgb(uint8_t r, int *R, int *G, int *B) {
    uint8_t rr, rg, rb;
    tami_rarity_rgb(r, &rr, &rg, &rb);
    *R = (int)rr;
    *G = (int)rg;
    *B = (int)rb;
}

static void tint_rarity(uint8_t rar, int *r, int *g, int *b) {
    if (rar <= TAMI_RARITY_COMMON) {
        return;
    }
    int rr, rg, rb;
    rarity_rgb(rar, &rr, &rg, &rb);
    *r = (*r * 2 + rr) / 3;
    *g = (*g * 2 + rg) / 3;
    *b = (*b * 2 + rb) / 3;
}

static void blit_feet(SDL_Renderer *ren, SDL_Texture *tex, int fx, int fy, int dest_h, int alpha,
                      double angle, SDL_RendererFlip flip) {
    if (!tex || dest_h < 1) {
        return;
    }
    int tw = 0, th = 0;
    SDL_QueryTexture(tex, NULL, NULL, &tw, &th);
    if (th < 1) {
        return;
    }
    int dw = tw * dest_h / th;
    SDL_Rect d = {fx - dw / 2, fy - dest_h, dw, dest_h};
    SDL_SetTextureAlphaMod(tex, (Uint8)alpha);
    SDL_RenderCopyEx(ren, tex, NULL, &d, angle, NULL, flip);
}

static void draw_gleam(SDL_Renderer *ren, const TamiAdventurer *a, float t, int feet_x, int feet_y) {
    uint8_t rar = tami_worn_rarity(a);
    if (rar < TAMI_RARITY_RARE || !g_fx_spark) {
        return;
    }
    int n = rar >= TAMI_RARITY_LEGENDARY ? 4 : 2;
    int h = rar >= TAMI_RARITY_LEGENDARY ? 28 : 18;
    int cr, cg, cb;
    rarity_rgb(rar, &cr, &cg, &cb);
    SDL_SetTextureColorMod(g_fx_spark, (Uint8)cr, (Uint8)cg, (Uint8)cb);
    for (int i = 0; i < n; i++) {
        float ph = t * 2.4f + (float)i * 1.7f;
        int mx = feet_x + (int)(cosf(ph) * 38.0f);
        int my = feet_y - 78 + (int)(sinf(ph * 1.35f) * 28.0f);
        int al = 110 + (int)(sinf(ph * 1.8f) * 70.0f);
        if (al < 40) {
            al = 40;
        }
        blit_feet(ren, g_fx_spark, mx, my, h, al, 0, SDL_FLIP_NONE);
    }
    SDL_SetTextureColorMod(g_fx_spark, 255, 255, 255);
}

static char g_vbuf[120000];

static void put_pixel(SDL_Renderer *ren, int x, int y) {
    SDL_RenderDrawPoint(ren, x, y);
}

static int in_circle(int x, int y) {
    int dx = x - CX, dy = y - CY;
    return dx * dx + dy * dy <= CR * CR;
}

static void fill_circle(SDL_Renderer *ren, int cx, int cy, int rad) {
    for (int y = -rad; y <= rad; y++) {
        int xx = (int)sqrt((double)(rad * rad - y * y));
        SDL_RenderDrawLine(ren, cx - xx, cy + y, cx + xx, cy + y);
    }
}

static void stroke_circle(SDL_Renderer *ren, int cx, int cy, int rad) {
    int x = rad, y = 0, err = 0;
    while (x >= y) {
        put_pixel(ren, cx + x, cy + y);
        put_pixel(ren, cx + y, cy + x);
        put_pixel(ren, cx - y, cy + x);
        put_pixel(ren, cx - x, cy + y);
        put_pixel(ren, cx - x, cy - y);
        put_pixel(ren, cx - y, cy - x);
        put_pixel(ren, cx + y, cy - x);
        put_pixel(ren, cx + x, cy - y);
        y++;
        if (err <= 0) {
            err += 2 * y + 1;
        }
        if (err > 0) {
            x--;
            err -= 2 * x + 1;
        }
    }
}

static void stroke_arc(SDL_Renderer *ren, int cx, int cy, int rad, float a0, float a1, int thick) {
    if (a1 <= a0) {
        return;
    }
    int n = (int)((a1 - a0) * (float)rad) + 8;
    if (n < 8) {
        n = 8;
    }
    if (n > 720) {
        n = 720;
    }
    for (int i = 0; i <= n; i++) {
        float t = a0 + (a1 - a0) * (float)i / (float)n;
        for (int w = 0; w < thick; w++) {
            float r = (float)(rad - w);
            int x = cx + (int)(cosf(t) * r);
            int y = cy + (int)(sinf(t) * r);
            put_pixel(ren, x, y);
        }
    }
}

static void rgb(SDL_Renderer *ren, int r, int g, int b) {
    SDL_SetRenderDrawColor(ren, (Uint8)r, (Uint8)g, (Uint8)b, 255);
}

static void field_palette(TamiField f, int *bg, int *fg, int *acc) {
    switch (f) {
    case TAMI_FIELD_IRONPIT:
        bg[0] = 58;
        bg[1] = 36;
        bg[2] = 28;
        fg[0] = 196;
        fg[1] = 110;
        fg[2] = 64;
        acc[0] = 220;
        acc[1] = 160;
        acc[2] = 80;
        break;
    case TAMI_FIELD_MOONWOOD:
        bg[0] = 22;
        bg[1] = 44;
        bg[2] = 52;
        fg[0] = 70;
        fg[1] = 140;
        fg[2] = 130;
        acc[0] = 160;
        acc[1] = 210;
        acc[2] = 190;
        break;
    case TAMI_FIELD_BARROW:
        bg[0] = 36;
        bg[1] = 28;
        bg[2] = 52;
        fg[0] = 120;
        fg[1] = 90;
        fg[2] = 160;
        acc[0] = 190;
        acc[1] = 150;
        acc[2] = 220;
        break;
    case TAMI_FIELD_ASHFEN:
        bg[0] = 40;
        bg[1] = 40;
        bg[2] = 38;
        fg[0] = 140;
        fg[1] = 120;
        fg[2] = 90;
        acc[0] = 210;
        acc[1] = 90;
        acc[2] = 50;
        break;
    default:
        bg[0] = 32;
        bg[1] = 52;
        bg[2] = 36;
        fg[0] = 90;
        fg[1] = 140;
        fg[2] = 70;
        acc[0] = 220;
        acc[1] = 180;
        acc[2] = 70;
        break;
    }
}

static void text_at(SDL_Renderer *ren, int x, int y, const char *s, int r, int g, int b, float scale) {
    char tmp[256];
    snprintf(tmp, sizeof(tmp), "%s", s);
    unsigned char col[4] = {(unsigned char)r, (unsigned char)g, (unsigned char)b, 255};
    int nq = stb_easy_font_print(0, 0, tmp, col, g_vbuf, (int)sizeof(g_vbuf));
    rgb(ren, r, g, b);
    for (int i = 0; i < nq; i++) {
        char *q = g_vbuf + i * 64;
        float minx = 1e9f, miny = 1e9f, maxx = -1e9f, maxy = -1e9f;
        for (int k = 0; k < 4; k++) {
            float px = *(float *)(q + k * 16);
            float py = *(float *)(q + k * 16 + 4);
            if (px < minx) {
                minx = px;
            }
            if (py < miny) {
                miny = py;
            }
            if (px > maxx) {
                maxx = px;
            }
            if (py > maxy) {
                maxy = py;
            }
        }
        SDL_Rect d;
        d.x = x + (int)(minx * scale);
        d.y = y + (int)(miny * scale);
        d.w = (int)((maxx - minx) * scale);
        d.h = (int)((maxy - miny) * scale);
        if (d.w < 1) {
            d.w = 1;
        }
        if (d.h < 1) {
            d.h = 1;
        }
        if (in_circle(d.x, d.y) || in_circle(d.x + d.w, d.y + d.h)) {
            SDL_RenderFillRect(ren, &d);
        }
    }
}

static void fill_round_rect(SDL_Renderer *ren, int x, int y, int w, int h, int rad);

static void text_cx(SDL_Renderer *ren, int y, const char *s, int r, int g, int b, float scale) {
    char tmp[256];
    snprintf(tmp, sizeof(tmp), "%s", s);
    int w = stb_easy_font_width(tmp);
    text_at(ren, CX - (int)((float)w * scale / 2.0f), y, s, r, g, b, scale);
}

static void text_cx_hud(SDL_Renderer *ren, int y, const char *s, int r, int g, int b, float scale) {
    static const int ox[8] = {-1, 1, 0, 0, -1, -1, 1, 1};
    static const int oy[8] = {0, 0, -1, 1, -1, 1, -1, 1};
    char tmp[256];
    snprintf(tmp, sizeof(tmp), "%s", s);
    int w = stb_easy_font_width(tmp);
    int cx = CX - (int)((float)w * scale / 2.0f);
    for (int i = 0; i < 8; i++) {
        text_at(ren, cx + ox[i], y + oy[i], s, 8, 6, 8, scale);
    }
    text_cx(ren, y, s, r, g, b, scale);
}

static void chip_box(SDL_Renderer *ren, int x, int y, int w, int h) {
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren, 8, 6, 10, 250);
    fill_round_rect(ren, x, y, w, h, h > 28 ? 14 : 10);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
}

static void text_cx_chip(SDL_Renderer *ren, int y, const char *s, int r, int g, int b, float scale) {
    char tmp[256];
    snprintf(tmp, sizeof(tmp), "%s", s);
    int tw = (int)((float)stb_easy_font_width(tmp) * scale);
    int th = (int)(8.0f * scale + 0.5f);
    int pad_x = 16;
    int pad_y = 6;
    int w = tw + pad_x * 2;
    int h = th + pad_y * 2;
    if (w < 72) {
        w = 72;
    }
    chip_box(ren, CX - w / 2, y - pad_y, w, h);
    text_cx_hud(ren, y, s, r, g, b, scale);
}

static void fill_ellipse(SDL_Renderer *ren, int cx, int cy, int rx, int ry) {
    for (int y = -ry; y <= ry; y++) {
        float t = 1.0f - (float)(y * y) / (float)(ry * ry);
        if (t < 0) {
            continue;
        }
        int xx = (int)((float)rx * sqrtf(t));
        SDL_RenderDrawLine(ren, cx - xx, cy + y, cx + xx, cy + y);
    }
}

static void fill_round_rect(SDL_Renderer *ren, int x, int y, int w, int h, int rad) {
    if (w < 2 || h < 2) {
        return;
    }
    if (rad < 1) {
        rad = 1;
    }
    if (rad * 2 > w) {
        rad = w / 2;
    }
    if (rad * 2 > h) {
        rad = h / 2;
    }
    SDL_Rect mid = {x + rad, y, w - 2 * rad, h};
    SDL_RenderFillRect(ren, &mid);
    SDL_Rect side = {x, y + rad, w, h - 2 * rad};
    SDL_RenderFillRect(ren, &side);
    fill_circle(ren, x + rad, y + rad, rad);
    fill_circle(ren, x + w - rad - 1, y + rad, rad);
    fill_circle(ren, x + rad, y + h - rad - 1, rad);
    fill_circle(ren, x + w - rad - 1, y + h - rad - 1, rad);
}

static void pill_box(SDL_Renderer *ren, SDL_Rect r, int hot) {
    if (hot) {
        rgb(ren, 255, 214, 130);
    } else {
        rgb(ren, 196, 168, 104);
    }
    fill_round_rect(ren, r.x, r.y, r.w, r.h, 12);
    if (hot) {
        rgb(ren, 92, 64, 36);
    } else {
        rgb(ren, 18, 14, 16);
    }
    fill_round_rect(ren, r.x + 2, r.y + 2, r.w - 4, r.h - 4, 10);
}

static void people_skin(TamiPeople p, int *r, int *g, int *b) {
    switch (p) {
    case TAMI_PEOPLE_ORC:
        *r = 72;
        *g = 128;
        *b = 64;
        break;
    case TAMI_PEOPLE_ELF:
        *r = 196;
        *g = 210;
        *b = 170;
        break;
    case TAMI_PEOPLE_UNDEAD:
        *r = 150;
        *g = 168;
        *b = 158;
        break;
    default:
        *r = 196;
        *g = 154;
        *b = 110;
        break;
    }
}

static void metal(int score, int *r, int *g, int *b) {
    if (score >= 14) {
        *r = 220;
        *g = 180;
        *b = 70; /* gilt */
    } else if (score >= 8) {
        *r = 190;
        *g = 195;
        *b = 200; /* steel */
    } else {
        *r = 120;
        *g = 110;
        *b = 95; /* iron */
    }
}

static void leather(int *r, int *g, int *b) {
    *r = 110;
    *g = 72;
    *b = 42;
}

static void draw_weapon(SDL_Renderer *ren, int ax, int ay, float arm, const TamiItem *w) {
    if (!tami_item_on(w)) {
        return;
    }
    int mr, mg, mb;
    metal(w->score, &mr, &mg, &mb);
    tint_rarity(w->rarity, &mr, &mg, &mb);
    int wx = ax + 24 + (int)arm;
    int wy = ay + 8 - (int)(arm * 0.35f);
    int look = w->look;
    rgb(ren, 96, 70, 40);
    if (look == TAMI_LOOK_STICK) {
        SDL_RenderDrawLine(ren, ax + 10, ay + 16, wx + 6, wy + 8);
        rgb(ren, 130, 100, 60);
        fill_circle(ren, wx + 6, wy + 8, 3);
    } else if (look == TAMI_LOOK_SHIV) {
        SDL_RenderDrawLine(ren, ax + 12, ay + 14, wx, wy);
        rgb(ren, mr, mg, mb);
        fill_ellipse(ren, wx + 4, wy - 2, 7, 3);
    } else if (look == TAMI_LOOK_HATCHET || look == TAMI_LOOK_PICK) {
        SDL_RenderDrawLine(ren, ax + 12, ay + 18, wx, wy - 6);
        rgb(ren, mr, mg, mb);
        fill_ellipse(ren, wx + 6, wy - 8, 10, 5);
    } else if (look == TAMI_LOOK_SPEAR || look == TAMI_LOOK_POLE) {
        SDL_RenderDrawLine(ren, ax + 10, ay + 20, wx + 18, wy - 36);
        rgb(ren, mr, mg, mb);
        fill_ellipse(ren, wx + 20, wy - 40, 5, 9);
    } else { /* blade */
        SDL_RenderDrawLine(ren, ax + 12, ay + 16, wx + 4, wy - 8);
        rgb(ren, mr, mg, mb);
        fill_ellipse(ren, wx + 8, wy - 16, 5, 16);
    }
}

static SDL_Texture *weapon_tex(uint8_t look) {
    if (look == TAMI_LOOK_HATCHET || look == TAMI_LOOK_PICK) {
        return g_wep[1];
    }
    if (look == TAMI_LOOK_SPEAR || look == TAMI_LOOK_POLE) {
        return g_wep[2];
    }
    if (look == TAMI_LOOK_BLADE || look == TAMI_LOOK_SHIV) {
        return g_wep[3];
    }
    return g_wep[0];
}

static SDL_Texture *helm_tex(uint8_t look) {
    if (look == TAMI_LOOK_CAP) {
        return g_helm[0];
    }
    if (look == TAMI_LOOK_COIF) {
        return g_helm[1];
    }
    return g_helm[2];
}

static void draw_gear_on_sprite(SDL_Renderer *ren, const TamiAdventurer *a, int fx, int fy, float arm) {
    (void)arm;
    const TamiItem *head = &a->equip[TAMI_SLOT_HELM];
    const TamiItem *sh = &a->equip[TAMI_SLOT_SHIELD];
    const TamiItem *wep = &a->equip[TAMI_SLOT_WEAPON];
    /* Off-hand shield on the viewer's left, then weapon in the right hand, helm last. */
    if (tami_item_on(sh)) {
        int tower = sh->look == TAMI_LOOK_SHIELD_TOWER;
        int hh = tower ? 64 : 48;
        blit_feet(ren, tower ? g_shield[1] : g_shield[0], fx - 40, fy - (tower ? 42 : 50), hh, 255, 0,
                  SDL_FLIP_NONE);
    }
    if (tami_item_on(wep) && weapon_tex(wep->look)) {
        int wh = 110, wx = fx + 32, wy = fy - 8;
        if (wep->look == TAMI_LOOK_HATCHET || wep->look == TAMI_LOOK_PICK) {
            wh = 72;
            wx = fx + 36;
            wy = fy - 24;
        } else if (wep->look == TAMI_LOOK_SPEAR || wep->look == TAMI_LOOK_POLE) {
            wh = 120;
            wx = fx + 34;
            wy = fy - 6;
        } else if (wep->look == TAMI_LOOK_BLADE || wep->look == TAMI_LOOK_SHIV) {
            wh = 90;
            wx = fx + 34;
            wy = fy - 12;
        }
        blit_feet(ren, weapon_tex(wep->look), wx, wy, wh, 255, 0, SDL_FLIP_NONE);
    }
    if (tami_item_on(head) && helm_tex(head->look)) {
        int hh = 52, hy = fy - 112;
        if (head->look == TAMI_LOOK_CAP) {
            hh = 40;
            hy = fy - 128;
        } else if (head->look == TAMI_LOOK_COIF) {
            hh = 58;
            hy = fy - 100;
        }
        blit_feet(ren, helm_tex(head->look), fx, hy, hh, 255, 0, SDL_FLIP_NONE);
    }
}

static void draw_cutscene(SDL_Renderer *ren, float tleft) {
    if (g_cut == TAMI_CUT_NONE || tleft <= 0) {
        return;
    }
    float u = 1.0f - tleft / 0.85f;
    if (u < 0) {
        u = 0;
    }
    int alpha = u < 0.25f ? (int)(255.0f * u / 0.25f) : (int)(255.0f * (1.0f - (u - 0.25f) / 0.75f));
    if (alpha < 0) {
        alpha = 0;
    }
    if (alpha > 255) {
        alpha = 255;
    }
    int h = 90 + (int)(u * 50.0f);
    SDL_Texture *fx = g_fx_slash;
    int x = CX + 40, y = CY + 40;
    double ang = -25.0 + u * 40.0;
    if (g_cut == TAMI_CUT_LOOT || g_cut == TAMI_CUT_SELL) {
        fx = g_fx_coins;
        x = CX - 20;
        y = CY + 20 - (int)(u * 40.0f);
        ang = u * 25.0;
        h = 80 + (int)(u * 30.0f);
    } else if (g_cut == TAMI_CUT_LEVEL || g_cut == TAMI_CUT_PET || g_cut == TAMI_CUT_GLEAM) {
        fx = g_fx_spark;
        x = CX - 40;
        y = CY;
        ang = u * 90.0;
        h = 100 + (int)(u * 40.0f);
    } else if (g_cut == TAMI_CUT_CAMP) {
        fx = g_fx_spark;
        x = CX - 110;
        y = CY + 70;
        h = 70;
        ang = 0;
    } else if (g_cut == TAMI_CUT_KILL) {
        fx = g_fx_slash;
        x = CX + 30;
        y = CY + 30;
    }
    if (g_cut == TAMI_CUT_KILL && u > 0.35f && g_fx_dust) {
        blit_feet(ren, g_fx_dust, CX + 90, CY + 90, 70 + (int)(u * 20.0f), alpha / 2, 0, SDL_FLIP_NONE);
    }
    if (fx) {
        blit_feet(ren, fx, x, y, h, alpha, ang, SDL_FLIP_NONE);
    }
    const char *cap = tami_cut_caption(g_cut);
    if (cap && cap[0]) {
        text_cx_chip(ren, 88, cap, 255, 230, 140, 1.8f);
    }
}

static void draw_adventurer(SDL_Renderer *ren, const TamiAdventurer *a, float t) {
    TamiAnimPose pose;
    tami_anim_pose(a, t, &pose);
    int alpha = pose.alpha;
    double angle = (double)pose.fig_angle_tenth / 10.0;

    int tier = armor_tier(a);
    SDL_Texture *fig = g_fig[a->people][tier];
    if (fig) {
        int feet_x = CX - 56 + pose.fig_dx;
        int feet_y = CY + 48 + pose.fig_dy;
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 90);
        fill_ellipse(ren, feet_x, feet_y - 4, 28, 8);
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
        if (a->morale < 30) {
            SDL_SetTextureColorMod(fig, 170, 170, 190);
        } else if (a->wounds > 55) {
            SDL_SetTextureColorMod(fig, 255, 170, 170);
        } else {
            SDL_SetTextureColorMod(fig, 255, 255, 255);
        }
        blit_feet(ren, fig, feet_x, feet_y, 170, alpha, angle, SDL_FLIP_NONE);
        SDL_SetTextureColorMod(fig, 255, 255, 255);
        if (!a->faded) {
            float arm = tami_task_is_fight(a) ? (float)pose.fig_dx * 0.8f : 0;
            draw_gear_on_sprite(ren, a, feet_x, feet_y, arm);
        }
        if (a->task.kind == TAMI_TASK_CAMP && !a->faded && g_fire) {
            int fh = 58 + pose.fire_h_add;
            blit_feet(ren, g_fire, CX - 120, CY + 52, fh < 40 ? 40 : fh, 255, 0, SDL_FLIP_NONE);
        }
        if (tami_task_is_fight(a) && !a->faded) {
            SDL_Texture *mob = g_enemy;
            int fam = tami_kill_family(a);
            if (fam >= 0 && fam < TAMI_FAMILY_COUNT && g_mob[fam]) {
                mob = g_mob[fam];
            }
            if (mob) {
                int eh = tami_kill_is_elite(a) ? 132 : 118;
                blit_feet(ren, mob, CX + 118 + pose.enemy_dx, CY + 52 + pose.enemy_dy, eh,
                          pose.enemy_alpha, (double)pose.enemy_angle_tenth / 10.0,
                          SDL_FLIP_HORIZONTAL);
            }
        }
        if (a->faded && a->heirloom.name[0]) {
            char heir[80];
            snprintf(heir, sizeof(heir), "%s", a->heirloom.name);
            text_cx(ren, feet_y + 12, heir, 180, 160, 120, 1.2f);
        }
        {
            const TamiPet *hp = tami_pet_beside(a);
            if (hp && hp->kind < TAMI_PET_KIND_COUNT && g_pet_tex[hp->kind]) {
                int pal = tami_pet_heel(a) ? alpha : alpha * 3 / 5;
                int px = feet_x - 78 + pose.pet_dx;
                int py = feet_y - 28 + pose.pet_dy;
                blit_feet(ren, g_pet_tex[hp->kind], px, py, 64, pal, 0, SDL_FLIP_NONE);
                text_at(ren, px - 36, py - 72, hp->name, 255, 244, 220, 1.3f);
            }
        }
        draw_gleam(ren, a, t, feet_x, feet_y);
        return;
    }

    int sr, sg, sb;
    people_skin(a->people, &sr, &sg, &sb);
    float arm = 0;
    int bob = 0;
    int sit = 0;
    if (a->faded) {
        sit = 24;
        sr = (sr + 80) / 2;
        sg = (sg + 80) / 2;
        sb = (sb + 90) / 2;
    } else if (a->task.kind == TAMI_TASK_DOWNED) {
        sit = 34;
        arm = -8;
        sr = sr + 40 > 255 ? 255 : sr + 40;
        sg = sg * 2 / 3;
    } else if (a->task.kind == TAMI_TASK_CAMP) {
        sit = 22;
        bob = 1;
    } else if (tami_task_is_fight(a)) {
        arm = sinf(t * 10.0f) * 14.0f;
        bob = (int)(sinf(t * 8.0f) * 2.0f);
    } else if (a->task.kind == TAMI_TASK_ROAD_MARKET || a->task.kind == TAMI_TASK_ROAD_FIELDS) {
        bob = (int)(sinf(t * 7.0f) * 4.0f);
    } else if (a->task.kind == TAMI_TASK_SELL || a->task.kind == TAMI_TASK_BUY) {
        arm = sinf(t * 3.0f) * 6.0f;
    }
    if (a->hunger < 35) {
        sit += 10;
    }
    if (a->rest < 35) {
        sit += 8;
        bob = 0;
    }
    if (a->wounds > 55) {
        sr = sr + 50 > 255 ? 255 : sr + 50;
        sg = sg * 3 / 4;
    }
    if (a->morale < 30) {
        sr = (sr + 90) / 2;
        sg = (sg + 90) / 2;
        sb = (sb + 90) / 2;
    }
    int ax = CX - 36, ay = CY + 8 + bob + sit;
    const TamiItem *head = &a->equip[TAMI_SLOT_HELM];
    const TamiItem *haub = &a->equip[TAMI_SLOT_HAUBERK];
    const TamiItem *gamb = &a->equip[TAMI_SLOT_GAMBESON];
    const TamiItem *bras = &a->equip[TAMI_SLOT_BRASSAIRTS];
    const TamiItem *body = haub;
    if (tami_item_on(gamb) && (!tami_item_on(body) || gamb->score > body->score)) {
        body = gamb;
    }
    if (tami_item_on(bras) && (!tami_item_on(body) || bras->score > body->score)) {
        body = bras;
    }
    if (!tami_item_on(body)) {
        body = haub;
    }
    const TamiItem *hands = &a->equip[TAMI_SLOT_GAUNTLETS];
    if (!tami_item_on(hands) && tami_item_on(&a->equip[TAMI_SLOT_VAMBRACES])) {
        hands = &a->equip[TAMI_SLOT_VAMBRACES];
    }
    const TamiItem *feet = &a->equip[TAMI_SLOT_GREAVES];
    if (!tami_item_on(feet) && tami_item_on(&a->equip[TAMI_SLOT_SOLLERETS])) {
        feet = &a->equip[TAMI_SLOT_SOLLERETS];
    }
    if (!tami_item_on(feet) && tami_item_on(&a->equip[TAMI_SLOT_CUISSES])) {
        feet = &a->equip[TAMI_SLOT_CUISSES];
    }
    const TamiItem *sh = &a->equip[TAMI_SLOT_SHIELD];
    const TamiItem *wep = &a->equip[TAMI_SLOT_WEAPON];

    /* feet first */
    if (tami_item_on(feet)) {
        int r, g, b;
        if (feet->look == TAMI_LOOK_SANDAL) {
            leather(&r, &g, &b);
        } else {
            metal(feet->score, &r, &g, &b);
        }
        rgb(ren, r, g, b);
        fill_ellipse(ren, ax - 10, ay + 58, 8, 5);
        fill_ellipse(ren, ax + 10, ay + 58, 8, 5);
        if (feet->look == TAMI_LOOK_GREAVES) {
            fill_ellipse(ren, ax - 10, ay + 48, 7, 10);
            fill_ellipse(ren, ax + 10, ay + 48, 7, 10);
        }
    }

    /* skin body */
    rgb(ren, sr, sg, sb);
    fill_ellipse(ren, ax, ay + 28, 20, 30);
    fill_circle(ren, ax, ay - 8, 16);

    /* torso armor */
    if (tami_item_on(body)) {
        int r, g, b;
        int look = body->look;
        if (look == TAMI_LOOK_CLOTH) {
            rgb(ren, 90, 70, 110);
            fill_ellipse(ren, ax, ay + 24, 18, 18);
        } else if (look == TAMI_LOOK_LEATHER) {
            leather(&r, &g, &b);
            rgb(ren, r, g, b);
            fill_ellipse(ren, ax, ay + 26, 21, 22);
        } else {
            metal(body->score, &r, &g, &b);
            rgb(ren, r, g, b);
            fill_ellipse(ren, ax, ay + 26, 22, 24);
            if (look == TAMI_LOOK_PLATE || look == TAMI_LOOK_HAUBERK) {
                rgb(ren, r + 20 > 255 ? 255 : r + 20, g + 20 > 255 ? 255 : g + 20, b);
                fill_ellipse(ren, ax - 16, ay + 10, 8, 7); /* pauldron */
                fill_ellipse(ren, ax + 16, ay + 10, 8, 7);
            }
            if (look == TAMI_LOOK_MAIL) {
                rgb(ren, 40, 40, 44);
                for (int i = -12; i <= 12; i += 4) {
                    put_pixel(ren, ax + i, ay + 20);
                    put_pixel(ren, ax + i + 2, ay + 24);
                    put_pixel(ren, ax + i, ay + 28);
                }
            }
        }
    }

    /* hands / gauntlets */
    if (tami_item_on(hands)) {
        int r, g, b;
        if (hands->look == TAMI_LOOK_WRAPS) {
            leather(&r, &g, &b);
        } else {
            metal(hands->score, &r, &g, &b);
        }
        rgb(ren, r, g, b);
        fill_circle(ren, ax - 20, ay + 22, hands->look == TAMI_LOOK_GAUNTLETS ? 7 : 5);
        fill_circle(ren, ax + 20, ay + 22, hands->look == TAMI_LOOK_GAUNTLETS ? 7 : 5);
    }

    /* face marks (under helm) */
    if (a->people == TAMI_PEOPLE_ORC) {
        rgb(ren, 230, 220, 200);
        SDL_RenderDrawLine(ren, ax - 10, ay - 2, ax - 16, ay + 6);
        SDL_RenderDrawLine(ren, ax + 10, ay - 2, ax + 16, ay + 6);
    }
    if (a->people == TAMI_PEOPLE_ELF && !(tami_item_on(head) && head->look == TAMI_LOOK_HELM)) {
        rgb(ren, sr, sg, sb);
        SDL_RenderDrawLine(ren, ax - 14, ay - 10, ax - 22, ay - 2);
        SDL_RenderDrawLine(ren, ax + 14, ay - 10, ax + 22, ay - 2);
    }
    if (a->people == TAMI_PEOPLE_UNDEAD) {
        rgb(ren, 20, 20, 24);
        fill_circle(ren, ax - 5, ay - 10, 3);
        fill_circle(ren, ax + 5, ay - 10, 3);
    }

    /* helm last on the skull */
    if (tami_item_on(head)) {
        int r, g, b;
        metal(head->score, &r, &g, &b);
        tint_rarity(head->rarity, &r, &g, &b);
        if (head->look == TAMI_LOOK_CAP) {
            leather(&r, &g, &b);
            rgb(ren, r, g, b);
            fill_ellipse(ren, ax, ay - 16, 14, 8);
        } else if (head->look == TAMI_LOOK_COIF) {
            rgb(ren, r, g, b);
            fill_ellipse(ren, ax, ay - 10, 17, 14);
            rgb(ren, sr, sg, sb);
            fill_ellipse(ren, ax, ay - 6, 10, 8); /* face hole */
        } else {
            rgb(ren, r, g, b);
            fill_ellipse(ren, ax, ay - 14, 16, 12);
            fill_ellipse(ren, ax, ay - 22, 10, 6); /* crest */
            rgb(ren, 12, 12, 16);
            SDL_Rect visor = {ax - 8, ay - 12, 16, 4};
            SDL_RenderFillRect(ren, &visor);
        }
    }

    /* shield on the off-hand */
    if (tami_item_on(sh)) {
        int r, g, b;
        metal(sh->score, &r, &g, &b);
        tint_rarity(sh->rarity, &r, &g, &b);
        rgb(ren, r, g, b);
        int rad = sh->look == TAMI_LOOK_SHIELD_TOWER ? 16 : 12;
        fill_circle(ren, ax - 22, ay + 20, rad);
        rgb(ren, 40, 30, 24);
        stroke_circle(ren, ax - 22, ay + 20, rad);
        rgb(ren, 200, 180, 90);
        fill_circle(ren, ax - 22, ay + 20, 3);
    }

    draw_weapon(ren, ax, ay, arm, wep);

    if (a->task.kind == TAMI_TASK_CAMP && !a->faded) {
        rgb(ren, 220, 90, 30);
        fill_circle(ren, ax - 36, ay + 52, 7 + (int)(sinf(t * 6.0f) * 2.0f));
        rgb(ren, 240, 180, 60);
        fill_circle(ren, ax - 36, ay + 50, 3);
    }
    if (a->faded && a->heirloom.name[0]) {
        char heir[80];
        snprintf(heir, sizeof(heir), "%s", a->heirloom.name);
        text_cx(ren, ay + 80, heir, 180, 160, 120, 1.2f);
    }

    if (tami_task_is_fight(a) && !a->faded) {
        int ex = CX + 70, ey = CY + 20;
        rgb(ren, 30, 28, 32);
        fill_ellipse(ren, ex, ey + 20, 18, 26);
        fill_circle(ren, ex, ey - 6, 14);
        rgb(ren, 80, 40, 40);
        fill_circle(ren, ex - 4, ey - 8, 3);
        fill_circle(ren, ex + 4, ey - 8, 3);
    }
}

static void draw_bar(SDL_Renderer *ren, int x, int y, int w, int h, int pct, int r, int g, int b) {
    rgb(ren, 36, 28, 30);
    SDL_Rect back = {x, y, w, h};
    SDL_RenderFillRect(ren, &back);
    rgb(ren, 196, 168, 104);
    SDL_RenderDrawRect(ren, &back);
    int fw = (w - 4) * pct / 100;
    if (fw < 0) {
        fw = 0;
    }
    rgb(ren, r, g, b);
    SDL_Rect fill = {x + 2, y + 2, fw, h - 4};
    SDL_RenderFillRect(ren, &fill);
}

static void draw_stat(SDL_Renderer *ren, int x, int y, const char *lab, unsigned v) {
    text_at(ren, x, y + 3, lab, 168, 148, 118, 1.2f);
    char n[8];
    snprintf(n, sizeof(n), "%u", v);
    int nw = (int)((float)stb_easy_font_width(n) * 1.85f);
    text_at(ren, x + 108 - nw, y, n, 255, 244, 220, 1.85f);
}

static void draw_vital(SDL_Renderer *ren, int x, int y, const char *lab, unsigned v, int r, int g,
                       int b) {
    SDL_Rect box = {x, y, 118, 26};
    pill_box(ren, box, 0);
    text_at(ren, x + 10, y + 8, lab, r, g, b, 1.2f);
    char n[8];
    snprintf(n, sizeof(n), "%u", v);
    int tw = stb_easy_font_width(n);
    text_at(ren, x + 108 - (int)((float)tw * 1.6f), y + 6, n, 255, 244, 220, 1.6f);
}

static void toast_set(const char *s) {
    snprintf(g_toast, sizeof(g_toast), "%s", s);
    g_toast_left = 1.8f;
}

static int in_rect(int x, int y, SDL_Rect r) {
    return x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}

static void hatch_people_geom(int i, SDL_Rect *r) {
    r->w = 110;
    r->h = 38;
    r->x = CX - 114 + (i % 2) * 118;
    r->y = 210 + (i / 2) * 42;
}

static void hatch_call_geom(int i, SDL_Rect *r) {
    r->w = 110;
    r->h = 38;
    r->x = CX - 114 + (i % 2) * 118;
    r->y = 298 + (i / 2) * 42;
}

static void hatch_go_geom(SDL_Rect *r) {
    r->w = 168;
    r->h = 40;
    r->x = CX - 84;
    r->y = 386;
}

static void hatch_list_geom(SDL_Rect *r) {
    r->w = 100;
    r->h = 36;
    r->x = CX - 50;
    r->y = 168;
}

static void roster_slot_geom(int i, SDL_Rect *r) {
    r->w = 280;
    r->h = 52;
    r->x = CX - 140;
    r->y = 92 + i * 58;
}

static void gfx_park(void) {
    if (g_hatched && g_adv && g_rng && tami_blob_sane(g_adv)) {
        (void)tami_roster_write_slot(&g_roster, g_roster.active < TAMI_ROSTER_SLOTS ? (int)g_roster.active : 0,
                                     g_adv, g_rng);
    }
}

static int gfx_wake(int slot) {
    if (!g_adv || !g_rng) {
        return 0;
    }
    if (tami_roster_read_slot(slot, g_adv, g_rng) != 0) {
        return 0;
    }
    g_roster.active = (uint8_t)slot;
    (void)tami_roster_save_meta(&g_roster);
    g_hatched = 1;
    g_view = VIEW_HOME;
    return 1;
}

static void blit_portal_ring(SDL_Renderer *ren) {
    if (!g_portal_ring) {
        return;
    }
    SDL_Rect full = {0, 0, PANEL, PANEL};
    SDL_RenderCopy(ren, g_portal_ring, NULL, &full);
}

static void burger_geom(SDL_Rect *r) {
    r->w = 96;
    r->h = 40;
    r->x = CX - 48;
    r->y = 404;
}

static void nav_geom(int i, SDL_Rect *r) {
    const int w = 100, h = 48, gap = 8;
    r->w = w;
    r->h = h;
    if (i < 3) {
        int total = 3 * w + 2 * gap;
        r->x = CX - total / 2 + i * (w + gap);
        r->y = 292;
    } else {
        int total = 2 * w + gap;
        r->x = CX - total / 2 + (i - 3) * (w + gap);
        r->y = 348;
    }
}

static void care_geom(int i, SDL_Rect *r) {
    r->w = 142;
    r->h = 42;
    r->x = CX - 148 + (i % 2) * 154;
    r->y = 198 + (i / 2) * 48;
}

static void field_geom(int i, SDL_Rect *r) {
    r->x = 86;
    r->y = 108 + i * 40;
    r->w = 294;
    r->h = 36;
}

static void draw_pill(SDL_Renderer *ren, SDL_Rect r, int hot, const char *lab) {
    pill_box(ren, r, hot);
    if (!lab || !lab[0]) {
        return;
    }
    char tmp[32];
    snprintf(tmp, sizeof(tmp), "%s", lab);
    int tw = stb_easy_font_width(tmp);
    float sc = 1.55f;
    int ty = r.y + (r.h - (int)(8.0f * sc)) / 2;
    text_at(ren, r.x + (r.w - (int)((float)tw * sc)) / 2, ty, lab, 255, 244, 220, sc);
}

static void draw_nav(SDL_Renderer *ren) {
    if (g_hud) {
        for (int i = 0; i < 5; i++) {
            SDL_Rect r;
            nav_geom(i, &r);
            int hot = (g_view == i + 1);
            draw_pill(ren, r, hot, k_nav_lab[i]);
        }
    }
    SDL_Rect b;
    burger_geom(&b);
    draw_pill(ren, b, g_hud, g_hud ? "hide" : "menu");
}

static void draw_menu_panel(SDL_Renderer *ren);

static void pet_row_geom(int i, SDL_Rect *r) {
    r->x = 86;
    r->y = 104 + i * 42;
    r->w = 294;
    r->h = 38;
}

static void pet_act_geom(int i, SDL_Rect *r) {
    r->w = 142;
    r->h = 40;
    r->x = CX - 148 + (i % 2) * 154;
    r->y = 196 + (i / 2) * 46;
}

static void draw_pets_menu(SDL_Renderer *ren, const TamiAdventurer *a) {
    draw_menu_panel(ren);
    text_cx(ren, 88, "pets", 230, 214, 180, 1.8f);
    if (a->pet_count == 0) {
        text_cx(ren, 150, "none yet", 180, 170, 150, 1.4f);
        text_cx(ren, 172, "they find you on the road", 150, 140, 130, 1.2f);
        return;
    }
    if (g_pet_sel >= a->pet_count) {
        g_pet_sel = 0;
    }
    for (uint8_t i = 0; i < a->pet_count; i++) {
        SDL_Rect r;
        pet_row_geom((int)i, &r);
        int hot = (g_pet_sel == (int)i);
        draw_pill(ren, r, hot, "");
        const TamiPet *p = &a->pets[i];
        char line[80];
        snprintf(line, sizeof(line), "%s%s  %s", p->name, i == a->pet_active ? " *" : "",
                 tami_pet_kind_name((TamiPetKind)p->kind));
        text_at(ren, r.x + 10, r.y + 6, line, 230, 220, 190, 1.25f);
        draw_bar(ren, r.x + 10, r.y + 22, 110, 8, p->hunger, 180, 90, 50);
        draw_bar(ren, r.x + 130, r.y + 22, 110, 8, p->mood, 200, 170, 80);
        if (p->kind < TAMI_PET_KIND_COUNT && g_pet_tex[p->kind]) {
            blit_feet(ren, g_pet_tex[p->kind], r.x + r.w - 22, r.y + r.h - 4, 40, 255, 0,
                      SDL_FLIP_NONE);
        }
    }
    for (int i = 0; i < 4; i++) {
        SDL_Rect r;
        pet_act_geom(i, &r);
        draw_pill(ren, r, 0, k_pet_act[i]);
    }
}

static void draw_menu_panel(SDL_Renderer *ren) {
    rgb(ren, 196, 168, 104);
    fill_round_rect(ren, 66, 76, 334, 240, 18);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren, 10, 8, 12, 252);
    fill_round_rect(ren, 68, 78, 330, 236, 16);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
}

static void draw_care_menu(SDL_Renderer *ren, const TamiAdventurer *a) {
    draw_menu_panel(ren);
    text_cx(ren, 88, "camp", 230, 214, 180, 1.8f);
    const char *hname = a->people == TAMI_PEOPLE_UNDEAD ? "ichor" : "hunger";
    char line[64];
    snprintf(line, sizeof(line), "%s", hname);
    text_at(ren, 100, 108, line, 200, 160, 130, 1.2f);
    draw_bar(ren, 168, 106, 180, 12, a->hunger, 180, 80, 50);
    text_at(ren, 100, 128, "rest", 160, 180, 200, 1.2f);
    draw_bar(ren, 168, 126, 180, 12, a->rest, 80, 140, 200);
    text_at(ren, 100, 148, "morale", 200, 180, 140, 1.2f);
    draw_bar(ren, 168, 146, 180, 12, a->morale, 200, 170, 80);
    text_at(ren, 100, 168, "wounds", 200, 140, 140, 1.2f);
    draw_bar(ren, 168, 166, 180, 12, a->wounds, 180, 60, 60);
    text_cx(ren, 188, "optional hurry", 160, 150, 130, 1.1f);
    for (int i = 0; i < 4; i++) {
        SDL_Rect r;
        care_geom(i, &r);
        draw_pill(ren, r, 0, k_care_lab[i]);
    }
}

static void draw_bag_menu(SDL_Renderer *ren, const TamiAdventurer *a) {
    draw_menu_panel(ren);
    char line[128];
    snprintf(line, sizeof(line), "bag   %ug   pack %d/%d", a->gold, tami_encumbrance(a),
             tami_encumbrance_max(a));
    text_cx(ren, 88, line, 230, 214, 180, 1.5f);
    text_at(ren, 96, 108, "loot on the way to market", 160, 150, 130, 1.15f);
    int y = 128;
    if (a->inv_count == 0) {
        text_at(ren, 96, y, "empty — kill more", 140, 130, 120, 1.2f);
        return;
    }
    int n = a->inv_count < 10 ? (int)a->inv_count : 10;
    for (int i = 0; i < n && y < 305; i++) {
        int idx = (int)a->inv_count - 1 - i;
        uint8_t q = a->inv[idx].qty ? a->inv[idx].qty : 1;
        const char *mk = tami_rarity_mark(a->inv[idx].rarity);
        if (q > 1) {
            snprintf(line, sizeof(line), "%s%s x%u", mk, a->inv[idx].name, q);
        } else {
            snprintf(line, sizeof(line), "%s%s", mk, a->inv[idx].name);
        }
        {
            int cr, cg, cb;
            rarity_rgb(a->inv[idx].rarity, &cr, &cg, &cb);
            text_at(ren, 96, y, line, cr, cg, cb, 1.15f);
        }
        y += 14;
    }
    if (a->inv_count > 10) {
        snprintf(line, sizeof(line), "... %u more", a->inv_count - 10);
        text_at(ren, 96, y, line, 150, 140, 130, 1.15f);
    }
}

static void draw_fields_menu(SDL_Renderer *ren, const TamiAdventurer *a) {
    draw_menu_panel(ren);
    text_cx(ren, 88, "fields", 230, 214, 180, 1.8f);
    for (int i = 0; i < TAMI_FIELD_COUNT; i++) {
        SDL_Rect r;
        field_geom(i, &r);
        int here = (a->field == (TamiField)i);
        draw_pill(ren, r, here, "");
        char line[80];
        int lo = tami_field_band_lo((TamiField)i);
        int hi = tami_field_band_hi((TamiField)i);
        const char *tag = "";
        if (here) {
            tag = "  here";
        } else if (a->level < lo) {
            tag = "  hard";
        } else if (a->level > hi) {
            tag = "  easy";
        }
        snprintf(line, sizeof(line), "%s   lv %d-%d%s", tami_field_name((TamiField)i), lo, hi, tag);
        text_at(ren, r.x + 16, r.y + 11, line, 230, 220, 190, 1.3f);
    }
}

static void sheet_tab_geom(int i, SDL_Rect *r) {
    r->w = 140;
    r->h = 30;
    r->x = CX - 148 + i * 156;
    r->y = 108;
}

static void draw_sheet_menu(SDL_Renderer *ren, const TamiAdventurer *a) {
    draw_menu_panel(ren);
    char line[128];
    snprintf(line, sizeof(line), "%s  lv %u", a->given_name, (unsigned)a->level);
    text_cx(ren, 86, line, 255, 244, 220, 1.7f);
    for (int i = 0; i < 2; i++) {
        SDL_Rect tr;
        sheet_tab_geom(i, &tr);
        draw_pill(ren, tr, g_sheet_page == i, i ? "worn" : "stats");
    }

    const int top = 144;
    const int bot = 310;
    int o = g_sheet_scroll;
    if (g_sheet_page == 1) {
        for (int s = 0; s < TAMI_SLOT_COUNT; s++) {
            int y = top + s * 16 - o;
            if (y < top || y >= bot) {
                continue;
            }
            text_at(ren, 88, y, tami_slot_name((TamiSlot)s), 160, 150, 130, 1.15f);
            if (tami_item_on(&a->equip[s])) {
                snprintf(line, sizeof(line), "%s%s", tami_rarity_mark(a->equip[s].rarity),
                         a->equip[s].name);
                if (strlen(line) > 22) {
                    line[19] = '.';
                    line[20] = '.';
                    line[21] = '.';
                    line[22] = 0;
                }
                int cr, cg, cb;
                rarity_rgb(a->equip[s].rarity, &cr, &cg, &cb);
                text_at(ren, 186, y, line, cr, cg, cb, 1.15f);
            } else {
                text_at(ren, 186, y, "-", 140, 130, 120, 1.15f);
            }
        }
        return;
    }

    int xp_pct = tami_xp_pct(a);
    snprintf(line, sizeof(line), "%u/%u", a->xp, a->xp_need);
    int frac_w = (int)((float)stb_easy_font_width(line) * 1.15f);
    int frac_x = 368 - frac_w;
    if (frac_x < 250) {
        frac_x = 250;
    }
    text_at(ren, 88, 146, "xp", 168, 148, 118, 1.2f);
    int bar_w = frac_x - 122;
    if (bar_w < 80) {
        bar_w = 80;
    }
    draw_bar(ren, 114, 144, bar_w, 12, xp_pct, 220, 180, 70);
    text_at(ren, frac_x, 146, line, 230, 214, 170, 1.15f);

    draw_vital(ren, 88, 164, "HP", a->hp_max, 210, 110, 110);
    draw_vital(ren, 220, 164, "MP", a->mp_max, 120, 150, 210);

    draw_stat(ren, 92, 200, "STR", a->str);
    draw_stat(ren, 92, 224, "CON", a->con);
    draw_stat(ren, 92, 248, "DEX", a->dex);
    draw_stat(ren, 250, 200, "INT", a->intel);
    draw_stat(ren, 250, 224, "WIS", a->wis);
    draw_stat(ren, 250, 248, "CHA", a->cha);

    snprintf(line, sizeof(line), "%s", tami_plot_name(a->plot_act));
    text_at(ren, 88, 274, line, 180, 176, 210, 1.2f);
    draw_bar(ren, 168, 274, 140, 10, (int)a->plot_progress, 140, 130, 190);
    snprintf(line, sizeof(line), "%u%%", a->plot_progress);
    text_at(ren, 316, 274, line, 180, 176, 210, 1.15f);

    if (a->quest_label[0]) {
        snprintf(line, sizeof(line), "%s  %u/%u", a->quest_label, (unsigned)a->quest_have,
                 (unsigned)a->quest_need);
        if (strlen(line) > 34) {
            line[31] = '.';
            line[32] = '.';
            line[33] = '.';
            line[34] = 0;
        }
        text_at(ren, 88, 292, line, 170, 190, 150, 1.15f);
    }
}

static void draw_log_menu(SDL_Renderer *ren, const TamiAdventurer *a) {
    draw_menu_panel(ren);
    text_cx(ren, 86, "story", 255, 244, 220, 1.7f);
    int o = g_sheet_scroll;
    int y = 108 - o;
    char story[480];
    tami_format_chronicle(a, story, sizeof(story));
    char *p = story;
    while (*p && y < 318) {
        char *nl = strchr(p, '\n');
        if (nl) {
            *nl = 0;
        }
        if (y >= 100) {
            char clip[48];
            snprintf(clip, sizeof(clip), "%s", p);
            if (strlen(clip) > 36) {
                clip[33] = '.';
                clip[34] = '.';
                clip[35] = '.';
                clip[36] = 0;
            }
            text_at(ren, 88, y, clip, 230, 220, 196, 1.15f);
        }
        y += 14;
        if (!nl) {
            break;
        }
        p = nl + 1;
    }
    y += 6;
    char log[640];
    tami_format_log(a, log, sizeof(log));
    p = log;
    while (*p && y < 318) {
        char *nl = strchr(p, '\n');
        if (nl) {
            *nl = 0;
        }
        if (y >= 100) {
            char clip[48];
            snprintf(clip, sizeof(clip), "%s", p);
            if (strlen(clip) > 36) {
                clip[33] = '.';
                clip[34] = '.';
                clip[35] = '.';
                clip[36] = 0;
            }
            text_at(ren, 88, y, clip, 200, 190, 160, 1.1f);
        }
        y += 14;
        if (!nl) {
            break;
        }
        p = nl + 1;
    }
}

static int handle_click(TamiAdventurer *a, int x, int y) {
    SDL_Rect r;
    if (g_view == VIEW_ROSTER) {
        for (int i = 0; i < TAMI_ROSTER_SLOTS; i++) {
            roster_slot_geom(i, &r);
            if (!in_rect(x, y, r)) {
                continue;
            }
            if (!g_roster.card[i].used) {
                g_create_slot = i;
                g_hatched = 0;
                g_view = VIEW_HATCH;
                return 1;
            }
            gfx_park();
            if (gfx_wake(i)) {
                toast_set("woke");
            }
            return 1;
        }
        return 1;
    }
    if (!g_hatched || g_view == VIEW_HATCH) {
        for (int i = 0; i < 4; i++) {
            hatch_people_geom(i, &r);
            if (in_rect(x, y, r)) {
                g_pick_people = i;
                return 1;
            }
        }
        for (int i = 0; i < 4; i++) {
            hatch_call_geom(i, &r);
            if (in_rect(x, y, r)) {
                g_pick_call = i;
                return 1;
            }
        }
        if (tami_roster_used(&g_roster) > 0) {
            hatch_list_geom(&r);
            if (in_rect(x, y, r)) {
                g_view = VIEW_ROSTER;
                g_hatched = 0;
                return 1;
            }
        }
        hatch_go_geom(&r);
        if (in_rect(x, y, r) && g_rng && g_adv) {
            int slot = g_create_slot;
            if (slot < 0 || slot >= TAMI_ROSTER_SLOTS || g_roster.card[slot].used) {
                slot = tami_roster_first_empty(&g_roster);
            }
            if (slot < 0) {
                toast_set("roster full");
                return 1;
            }
            tami_hatch(g_adv, (TamiPeople)g_pick_people, (TamiCalling)g_pick_call, g_rng, 1);
            (void)tami_roster_write_slot(&g_roster, slot, g_adv, g_rng);
            g_create_slot = slot;
            g_hatched = 1;
            g_view = VIEW_HOME;
            toast_set("hatched");
            return 1;
        }
        return 1;
    }
    {
        SDL_Rect name = {70, 12, 326, 36};
        if (in_rect(x, y, name)) {
            gfx_park();
            g_hatched = 0;
            g_view = VIEW_ROSTER;
            return 1;
        }
    }
    if (g_hatched && g_view == VIEW_HOME && a->faded) {
        hatch_go_geom(&r);
        if (in_rect(x, y, r) && g_rng) {
            if (tami_whelp(a, g_rng, a->last_tick_unix + 1) == 0) {
                gfx_park();
                toast_set("rose");
            }
            return 1;
        }
    }
    if (g_hatched && a->faded == 0) {
        burger_geom(&r);
        if (in_rect(x, y, r)) {
            g_hud = !g_hud;
            return 1;
        }
    }
    if (g_hatched && g_hud) {
        for (int i = 0; i < 5; i++) {
            nav_geom(i, &r);
            if (in_rect(x, y, r)) {
                int next = i + 1;
                g_view = (g_view == next) ? VIEW_HOME : next;
                g_hud = 0;
                if (g_view == VIEW_SHEET || g_view == VIEW_LOG) {
                    g_sheet_scroll = 0;
                }
                return 1;
            }
        }
    }
    if (g_view == VIEW_HOME) {
        SDL_Rect meta = {80, 44, 306, 32};
        if (in_rect(x, y, meta)) {
            g_view = VIEW_FIELDS;
            return 1;
        }
    }
    if (g_view == VIEW_CARE) {
        for (int i = 0; i < 4; i++) {
            care_geom(i, &r);
            if (!in_rect(x, y, r)) {
                continue;
            }
            if (i == 0) {
                tami_feed(a);
                toast_set(a->people == TAMI_PEOPLE_UNDEAD ? "ichor up" : "fed");
            } else if (i == 1) {
                tami_pep(a);
                toast_set("pep");
            } else if (i == 2) {
                tami_bandage(a);
                toast_set("bandaged");
            } else {
                tami_force_camp(a);
                toast_set("camped");
                g_view = VIEW_HOME;
            }
            return 1;
        }
        g_view = VIEW_HOME;
        return 1;
    }
    if (g_view == VIEW_FIELDS) {
        for (int i = 0; i < TAMI_FIELD_COUNT; i++) {
            field_geom(i, &r);
            if (in_rect(x, y, r)) {
                if (tami_set_field(a, (TamiField)i) == 0) {
                    char line[48];
                    snprintf(line, sizeof(line), "sent to %s", tami_field_name((TamiField)i));
                    toast_set(line);
                    g_view = VIEW_HOME;
                }
                return 1;
            }
        }
        g_view = VIEW_HOME;
        return 1;
    }
    if (g_view == VIEW_PETS) {
        if (a->pet_count) {
            for (int i = 0; i < a->pet_count; i++) {
                pet_row_geom(i, &r);
                if (in_rect(x, y, r)) {
                    g_pet_sel = i;
                    return 1;
                }
            }
            for (int i = 0; i < 4; i++) {
                pet_act_geom(i, &r);
                if (!in_rect(x, y, r)) {
                    continue;
                }
                uint8_t s = (uint8_t)g_pet_sel;
                if (s >= a->pet_count) {
                    s = 0;
                }
                if (i == 0) {
                    tami_pet_feed(a, s);
                    toast_set("fed pet");
                } else if (i == 1) {
                    tami_pet_play(a, s);
                    toast_set("played");
                } else if (i == 2) {
                    tami_pet_heel_set(a, s);
                    toast_set("at heel");
                } else {
                    if (g_gone_arm <= 0) {
                        g_gone_arm = 90;
                        char warn[64];
                        snprintf(warn, sizeof(warn), "gone again to banish %s", a->pets[s].name);
                        toast_set(warn);
                        return 1;
                    }
                    char line[48];
                    snprintf(line, sizeof(line), "let %s go", a->pets[s].name);
                    toast_set(line);
                    tami_pet_release(a, s);
                    g_gone_arm = 0;
                    if (g_pet_sel >= a->pet_count) {
                        g_pet_sel = 0;
                    }
                }
                return 1;
            }
        }
        g_view = VIEW_HOME;
        return 1;
    }
    if (g_view == VIEW_SHEET) {
        for (int i = 0; i < 2; i++) {
            sheet_tab_geom(i, &r);
            if (in_rect(x, y, r)) {
                g_sheet_page = i;
                g_sheet_scroll = 0;
                return 1;
            }
        }
        g_view = VIEW_HOME;
        return 1;
    }
    if (g_view == VIEW_BAG || g_view == VIEW_LOG) {
        g_view = VIEW_HOME;
        return 1;
    }
    tami_pep(a);
    toast_set("pep");
    return 1;
}

static void draw_card(SDL_Renderer *ren, const char *card) {
    rgb(ren, 18, 16, 22);
    fill_ellipse(ren, CX, CY, 170, 110);
    rgb(ren, 200, 180, 120);
    stroke_circle(ren, CX, CY, 2); /* no-op-ish */
    text_cx(ren, 150, "check-in", 220, 200, 140, 1.8f);
    char line[64];
    int y = 178;
    const char *p = card;
    while (*p && y < 300) {
        int n = 0;
        while (p[n] && p[n] != '\n' && n < 60) {
            n++;
        }
        snprintf(line, sizeof(line), "%.*s", n, p);
        text_cx(ren, y, line, 230, 220, 200, 1.4f);
        p += n;
        if (*p == '\n') {
            p++;
        }
        y += 20;
    }
    text_cx(ren, 318, "space to dismiss", 140, 130, 110, 1.2f);
}

static void draw_hatch_menu(SDL_Renderer *ren, float t) {
    rgb(ren, 0, 0, 0);
    SDL_RenderClear(ren);
    SDL_Rect full = {0, 0, PANEL, PANEL};
    if (g_portal_void) {
        SDL_RenderCopy(ren, g_portal_void, NULL, &full);
    }
    SDL_Texture *fig = g_fig[g_pick_people][0];
    if (fig) {
        int feet_y = 200 + (int)(sinf(t * 2.2f) * 3.0f);
        blit_feet(ren, fig, CX, feet_y, 118, 255, 0, SDL_FLIP_NONE);
    }
    blit_portal_ring(ren);
    text_cx_chip(ren, 28, "who wakes?", 255, 244, 220, 2.0f);
    char sub[64];
    snprintf(sub, sizeof(sub), "%s  %s", k_people_lab[g_pick_people], k_call_lab[g_pick_call]);
    text_cx_chip(ren, 54, sub, 230, 214, 170, 1.5f);
    SDL_Rect r;
    for (int i = 0; i < 4; i++) {
        hatch_people_geom(i, &r);
        draw_pill(ren, r, g_pick_people == i, k_people_lab[i]);
    }
    for (int i = 0; i < 4; i++) {
        hatch_call_geom(i, &r);
        draw_pill(ren, r, g_pick_call == i, k_call_lab[i]);
    }
    hatch_go_geom(&r);
    draw_pill(ren, r, 1, "hatch");
    if (tami_roster_used(&g_roster) > 0) {
        hatch_list_geom(&r);
        draw_pill(ren, r, 0, "list");
    }
}

static void draw_roster_menu(SDL_Renderer *ren) {
    rgb(ren, 0, 0, 0);
    SDL_RenderClear(ren);
    SDL_Rect full = {0, 0, PANEL, PANEL};
    if (g_portal_void) {
        SDL_RenderCopy(ren, g_portal_void, NULL, &full);
    }
    blit_portal_ring(ren);
    text_cx_chip(ren, 28, "who walks?", 255, 244, 220, 2.0f);
    text_cx_chip(ren, 54, "pick a soul, or hatch new", 230, 214, 170, 1.4f);
    SDL_Rect r;
    for (int i = 0; i < TAMI_ROSTER_SLOTS; i++) {
        roster_slot_geom(i, &r);
        char lab[64];
        tami_roster_label(&g_roster, i, lab, sizeof(lab));
        int hot = g_roster.card[i].used && g_roster.active == (uint8_t)i;
        draw_pill(ren, r, hot, lab);
    }
}

static void draw_home(SDL_Renderer *ren, const TamiAdventurer *a, float t, const char *card) {
    if (g_view == VIEW_ROSTER) {
        draw_roster_menu(ren);
        return;
    }
    if (g_view == VIEW_HATCH || !g_hatched) {
        draw_hatch_menu(ren, t);
        return;
    }
    int bg[3], fg[3], acc[3];
    field_palette(a->field, bg, fg, acc);
    if (tami_is_night(a->last_tick_unix)) {
        bg[0] = bg[0] * 2 / 5;
        bg[1] = bg[1] * 2 / 5;
        bg[2] = bg[2] * 3 / 5;
        if (a->field == TAMI_FIELD_BARROW) {
            bg[2] = bg[2] + 30 > 80 ? 80 : bg[2] + 30;
        }
    }
    rgb(ren, 0, 0, 0);
    SDL_RenderClear(ren);
    if (g_field_tex[a->field]) {
        SDL_Texture *ft = g_field_tex[a->field];
        if (tami_is_night(a->last_tick_unix)) {
            SDL_SetTextureColorMod(ft, 90, 90, 130);
        } else {
            SDL_SetTextureColorMod(ft, 255, 255, 255);
        }
        SDL_Rect full = {0, 0, PANEL, PANEL};
        SDL_RenderCopy(ren, ft, NULL, &full);
        if (g_disc_mask) {
            SDL_RenderCopy(ren, g_disc_mask, NULL, &full);
        }
    } else {
        rgb(ren, bg[0], bg[1], bg[2]);
        fill_circle(ren, CX, CY, CR);
    }
    if (!g_portal_ring) {
        rgb(ren, fg[0], fg[1], fg[2]);
        stroke_circle(ren, CX, CY, CR - 1);
        stroke_circle(ren, CX, CY, CR - 2);
    }

    int xp_pct = tami_xp_pct(a);
    float two_pi = 6.2831853f;
    float a0 = -1.5708f;
    rgb(ren, 40, 36, 28);
    stroke_arc(ren, CX, CY, CR - 8, a0, a0 + two_pi, 6);
    rgb(ren, 220, 180, 70);
    stroke_arc(ren, CX, CY, CR - 8, a0, a0 + two_pi * (float)xp_pct / 100.0f, 6);

    /* need pips at inner rim */
    float nh = (float)a->hunger / 100.0f;
    float nr = (float)a->rest / 100.0f;
    rgb(ren, 180, 80, 50);
    stroke_arc(ren, CX, CY, CR - 18, 0.6f, 0.6f + 1.2f * nh, 3);
    rgb(ren, 80, 140, 200);
    stroke_arc(ren, CX, CY, CR - 22, 0.6f, 0.6f + 1.2f * nr, 3);

    draw_adventurer(ren, a, t);
    blit_portal_ring(ren);
    if (g_view == VIEW_HOME) {
        draw_cutscene(ren, g_cut_t);
    }

    char line[128];
    snprintf(line, sizeof(line), "%s  %s %s", a->given_name, a->people_name, a->calling_name);
    text_cx_chip(ren, 28, line, 255, 244, 220, 2.0f);
    snprintf(line, sizeof(line), "lv %u  %d%%  %s", a->level, xp_pct, tami_field_name(a->field));
    text_cx_chip(ren, 54, line, 230, 214, 170, 1.6f);

    if (g_view == VIEW_HOME) {
        chip_box(ren, 108, 78, 250, 18);
        draw_bar(ren, 116, 81, 234, 12, xp_pct, 220, 180, 70);
        int pct = tami_task_pct(a);
        const char *task = (a->banner_left && a->banner[0]) ? a->banner : a->task.label;
        text_cx_chip(ren, 276, task, 255, 244, 220, 1.45f);
        chip_box(ren, 80, 296, 306, 26);
        draw_bar(ren, 88, 300, 290, 18, pct, acc[0], acc[1], acc[2]);
    }

    if (g_view == VIEW_CARE) {
        draw_care_menu(ren, a);
    } else if (g_view == VIEW_BAG) {
        draw_bag_menu(ren, a);
    } else if (g_view == VIEW_LOG) {
        draw_log_menu(ren, a);
    } else if (g_view == VIEW_FIELDS) {
        draw_fields_menu(ren, a);
    } else if (g_view == VIEW_SHEET) {
        draw_sheet_menu(ren, a);
    } else if (g_view == VIEW_PETS) {
        draw_pets_menu(ren, a);
    }

    if (g_hatched && !(a->faded && g_view == VIEW_HOME)) {
        draw_nav(ren);
    }
    if (g_hatched && g_view == VIEW_HOME && a->faded) {
        SDL_Rect wr;
        hatch_go_geom(&wr);
        draw_pill(ren, wr, 1, "wake");
    }

    if (g_toast_left > 0 && g_toast[0]) {
        text_cx_chip(ren, 248, g_toast, 255, 220, 140, 1.5f);
    }

    if (a->faded && !(card && card[0])) {
        text_cx(ren, CY, "faded", 220, 80, 80, 3.0f);
    }
    if (card && card[0] && g_view == VIEW_HOME) {
        draw_card(ren, card);
    }
}

static TamiField parse_field(const char *s) {
    if (s && !strcmp(s, "ironpit")) {
        return TAMI_FIELD_IRONPIT;
    }
    if (s && !strcmp(s, "moonwood")) {
        return TAMI_FIELD_MOONWOOD;
    }
    if (s && !strcmp(s, "barrow")) {
        return TAMI_FIELD_BARROW;
    }
    if (s && !strcmp(s, "ashfen")) {
        return TAMI_FIELD_ASHFEN;
    }
    return TAMI_FIELD_GREENROAD;
}

static TamiPeople parse_people(const char *s) {
    if (s && !strcmp(s, "human")) {
        return TAMI_PEOPLE_HUMAN;
    }
    if (s && !strcmp(s, "elf")) {
        return TAMI_PEOPLE_ELF;
    }
    if (s && !strcmp(s, "undead")) {
        return TAMI_PEOPLE_UNDEAD;
    }
    return TAMI_PEOPLE_ORC;
}

static TamiCalling parse_call(const char *s) {
    if (s && !strcmp(s, "ranger")) {
        return TAMI_CALL_RANGER;
    }
    if (s && !strcmp(s, "mage")) {
        return TAMI_CALL_MAGE;
    }
    if (s && !strcmp(s, "rogue")) {
        return TAMI_CALL_ROGUE;
    }
    return TAMI_CALL_WARRIOR;
}

static int save_bmp(SDL_Renderer *ren, const char *path) {
    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(0, PANEL, PANEL, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!surf) {
        fprintf(stderr, "surface: %s\n", SDL_GetError());
        return 1;
    }
    if (SDL_RenderReadPixels(ren, NULL, SDL_PIXELFORMAT_ARGB8888, surf->pixels, surf->pitch) != 0) {
        fprintf(stderr, "readpixels: %s\n", SDL_GetError());
        SDL_FreeSurface(surf);
        return 1;
    }
    if (SDL_SaveBMP(surf, path) != 0) {
        fprintf(stderr, "savebmp: %s\n", SDL_GetError());
        SDL_FreeSurface(surf);
        return 1;
    }
    SDL_FreeSurface(surf);
    printf("wrote %s\n", path);
    return 0;
}

int main(int argc, char **argv) {
    TamiPeople people = TAMI_PEOPLE_ORC;
    TamiCalling calling = TAMI_CALL_WARRIOR;
    TamiField field = TAMI_FIELD_GREENROAD;
    int have_field = 0;
    uint32_t hours = 0;
    uint32_t seed = (uint32_t)time(NULL);
    const char *shot = NULL;
    int rate = 24; /* sim seconds per real second */
    int windowed = 1;
    int no_card = 0;
    int want_camp = 0;
    int start_view = VIEW_HOME;
    int want_pet = -1;
    int want_gleam = 0;
    int want_mob = -1;
    const char *roster_dir = "build/roster";

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--people") && i + 1 < argc) {
            people = parse_people(argv[++i]);
        } else if (!strcmp(argv[i], "--calling") && i + 1 < argc) {
            calling = parse_call(argv[++i]);
        } else if (!strcmp(argv[i], "--hours") && i + 1 < argc) {
            hours = (uint32_t)atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--seed") && i + 1 < argc) {
            seed = (uint32_t)strtoul(argv[++i], NULL, 10);
        } else if (!strcmp(argv[i], "--screenshot") && i + 1 < argc) {
            shot = argv[++i];
            windowed = 0;
        } else if (!strcmp(argv[i], "--rate") && i + 1 < argc) {
            rate = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--armor") && i + 1 < argc) {
            const char *ar = argv[++i];
            if (!strcmp(ar, "cloth")) {
                g_force_tier = 0;
            } else if (!strcmp(ar, "leather")) {
                g_force_tier = 1;
            } else if (!strcmp(ar, "mail")) {
                g_force_tier = 2;
            } else if (!strcmp(ar, "plate")) {
                g_force_tier = 3;
            }
        } else if (!strcmp(argv[i], "--no-card")) {
            no_card = 1;
        } else if (!strcmp(argv[i], "--camp")) {
            want_camp = 1;
        } else if (!strcmp(argv[i], "--menu") && i + 1 < argc) {
            const char *m = argv[++i];
            if (!strcmp(m, "care") || !strcmp(m, "camp")) {
                start_view = VIEW_CARE;
            } else if (!strcmp(m, "bag")) {
                start_view = VIEW_BAG;
            } else if (!strcmp(m, "log") || !strcmp(m, "story")) {
                start_view = VIEW_LOG;
            } else if (!strcmp(m, "map") || !strcmp(m, "fields")) {
                start_view = VIEW_FIELDS;
            } else if (!strcmp(m, "sheet")) {
                start_view = VIEW_SHEET;
                g_sheet_page = 0;
            } else if (!strcmp(m, "worn") || !strcmp(m, "gear")) {
                start_view = VIEW_SHEET;
                g_sheet_page = 1;
            } else if (!strcmp(m, "pets") || !strcmp(m, "pet")) {
                start_view = VIEW_PETS;
            } else if (!strcmp(m, "hatch") || !strcmp(m, "select")) {
                start_view = VIEW_HATCH;
            } else if (!strcmp(m, "roster") || !strcmp(m, "chars")) {
                start_view = VIEW_ROSTER;
            }
        } else if (!strcmp(argv[i], "--pet") && i + 1 < argc) {
            const char *pk = argv[++i];
            if (!strcmp(pk, "toad")) {
                want_pet = TAMI_PET_TOAD;
            } else if (!strcmp(pk, "rat")) {
                want_pet = TAMI_PET_RAT;
            } else if (!strcmp(pk, "moth")) {
                want_pet = TAMI_PET_MOTH;
            } else if (!strcmp(pk, "crow")) {
                want_pet = TAMI_PET_CROW;
            }
        } else if (!strcmp(argv[i], "--gleam")) {
            want_gleam = 1;
        } else if (!strcmp(argv[i], "--mob") && i + 1 < argc) {
            const char *mn = argv[++i];
            for (int m = 0; m < TAMI_FAMILY_COUNT; m++) {
                if (!strcmp(mn, k_mob_file[m]) || !strcmp(mn, tami_mob_name((uint8_t)m))) {
                    want_mob = m;
                }
            }
        } else if (!strcmp(argv[i], "--field") && i + 1 < argc) {
            field = parse_field(argv[++i]);
            have_field = 1;
        } else if (!strcmp(argv[i], "--roster") && i + 1 < argc) {
            roster_dir = argv[++i];
        } else if (!strcmp(argv[i], "--help")) {
            printf("tami-gfx — 466x466 round desk sim (same app/ as the device)\n"
                   "  --people orc|human|elf|undead  --calling warrior|ranger|mage|rogue\n"
                   "  --hours N   --seed N   --rate N   --screenshot file.bmp\n"
                   "  --armor cloth|leather|mail|plate   --field greenroad|ironpit|moonwood|barrow|ashfen\n"
                   "  --no-card   --camp   --menu camp|bag|log|sheet|worn|pets|hatch|roster|fields\n"
                   "  --roster DIR   --pet toad|rat|moth|crow   --gleam   --mob gnoll|...|hag\n"
                   "keys: space pep  F camp  B bag  L/M log  S sheet  P pets  R roster  H bandage\n"
                   "      menu opens camp/bag/log/sheet/pets  tap name for roster  Tab  Esc  Q\n");
            return 0;
        }
    }

    TamiRng rng;
    tami_rng_seed(&rng, seed);
    TamiAdventurer adv;
    tami_hatch(&adv, people, calling, &rng, 1);
    g_rng = &rng;
    g_adv = &adv;
    g_pick_people = (int)people;
    g_pick_call = (int)calling;
    tami_roster_set_dir(roster_dir);
    tami_roster_load(&g_roster);
    if (start_view == VIEW_ROSTER) {
        g_hatched = 0;
    } else if (start_view == VIEW_HATCH) {
        g_hatched = 0;
        g_create_slot = tami_roster_first_empty(&g_roster);
        if (g_create_slot < 0) {
            g_create_slot = 0;
        }
    } else if (!shot && hours == 0) {
        if (g_roster.active != 0xFF && gfx_wake((int)g_roster.active)) {
            start_view = VIEW_HOME;
        } else {
            g_hatched = 0;
            start_view = VIEW_HATCH;
        }
    } else if (hours > 0 || shot) {
        (void)tami_roster_write_slot(&g_roster, 0, &adv, &rng);
    }
    if (have_field) {
        tami_set_field(&adv, field);
    }
    if (want_camp) {
        tami_force_camp(&adv);
    }
    if (want_pet >= 0) {
        tami_pet_add(&adv, &rng, (TamiPetKind)want_pet);
    }
    if (want_gleam) {
        tami_give_relic(&adv, &rng);
    }
    if (want_mob >= 0) {
        adv.task.kind = TAMI_TASK_KILL;
        adv.task.family = (uint8_t)want_mob;
        if (adv.task.duration_sec < 10) {
            adv.task.duration_sec = 20;
        }
        adv.task.elapsed_sec = adv.task.duration_sec / 2;
        snprintf(adv.task.label, sizeof(adv.task.label), "Executing a %s",
                 tami_mob_name((uint8_t)want_mob));
    }
    char card[256] = {0};
    int show_card = 0;
    if (hours) {
        TamiReport r;
        tami_catchup(&adv, &rng, 1 + (int64_t)hours * 3600, &r);
        tami_format_card(&adv, &r, card, sizeof(card));
        show_card = no_card ? 0 : 1;
        printf("%s\n", card);
    }

    find_asset_root(argv[0]);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *win = NULL;
    SDL_Renderer *ren = NULL;
    if (windowed) {
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
        win = SDL_CreateWindow("Tami 466", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, PANEL * 2,
                               PANEL * 2, SDL_WINDOW_ALLOW_HIGHDPI);
        if (!win) {
            fprintf(stderr, "window: %s\n", SDL_GetError());
            return 1;
        }
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!ren) {
            ren = SDL_CreateRenderer(win, -1, 0);
        }
        SDL_RenderSetLogicalSize(ren, PANEL, PANEL);
        load_art(ren);
    } else {
        SDL_Surface *surf =
            SDL_CreateRGBSurfaceWithFormat(0, PANEL, PANEL, 32, SDL_PIXELFORMAT_ARGB8888);
        if (!surf) {
            fprintf(stderr, "surf: %s\n", SDL_GetError());
            return 1;
        }
        ren = SDL_CreateSoftwareRenderer(surf);
        if (!ren) {
            fprintf(stderr, "sw renderer: %s\n", SDL_GetError());
            return 1;
        }
        load_art(ren);
        g_view = start_view;
        draw_home(ren, &adv, 0.4f, show_card ? card : NULL);
        SDL_RenderPresent(ren);
        int rc = SDL_SaveBMP(surf, shot);
        if (rc != 0) {
            /* software renderer present may need ReadPixels path */
            rc = save_bmp(ren, shot);
        } else {
            printf("wrote %s\n", shot);
        }
        SDL_DestroyRenderer(ren);
        SDL_FreeSurface(surf);
        SDL_Quit();
        return rc;
    }

    int running = 1;
    g_view = start_view;
    Uint32 last = SDL_GetTicks();
    float anim = 0;
    printf("Tami gfx 466. click camp/bag/log/sheet   space pep  L log  S sheet  tap field name\n");
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = 0;
            } else if (e.type == SDL_KEYDOWN) {
                SDL_Keycode k = e.key.keysym.sym;
                if (k == SDLK_q) {
                    running = 0;
                } else if (k == SDLK_ESCAPE) {
                    if (g_view == VIEW_ROSTER) {
                        if (g_roster.active != 0xFF && gfx_wake((int)g_roster.active)) {
                            /* resume */
                        } else {
                            g_view = VIEW_HATCH;
                            g_hatched = 0;
                        }
                    } else if (g_view == VIEW_HATCH && tami_roster_used(&g_roster) > 0) {
                        g_view = VIEW_ROSTER;
                        g_hatched = 0;
                    } else if (g_view != VIEW_HOME) {
                        g_view = VIEW_HOME;
                    } else {
                        running = 0;
                    }
                } else if (k == SDLK_SPACE) {
                    if (show_card) {
                        show_card = 0;
                    } else {
                        tami_pep(&adv);
                        toast_set("pep");
                    }
                } else if (k == SDLK_f) {
                    if (g_view == VIEW_CARE) {
                        tami_feed(&adv);
                        toast_set(adv.people == TAMI_PEOPLE_UNDEAD ? "ichor up" : "fed");
                    } else {
                        g_view = VIEW_CARE;
                    }
                } else if (k == SDLK_c) {
                    tami_force_camp(&adv);
                    toast_set("camped");
                    g_view = VIEW_HOME;
                } else if (k == SDLK_b) {
                    g_view = (g_view == VIEW_BAG) ? VIEW_HOME : VIEW_BAG;
                } else if (k == SDLK_m || k == SDLK_l) {
                    g_view = (g_view == VIEW_LOG) ? VIEW_HOME : VIEW_LOG;
                    g_sheet_scroll = 0;
                } else if (k == SDLK_s) {
                    g_view = (g_view == VIEW_SHEET) ? VIEW_HOME : VIEW_SHEET;
                    g_sheet_scroll = 0;
                } else if (k == SDLK_UP) {
                    if ((g_view == VIEW_SHEET || g_view == VIEW_LOG) && g_sheet_scroll > 0) {
                        g_sheet_scroll -= 24;
                        if (g_sheet_scroll < 0) {
                            g_sheet_scroll = 0;
                        }
                    } else if (g_view == VIEW_PETS && g_adv && g_adv->pet_count) {
                        g_pet_sel = (g_pet_sel + (int)g_adv->pet_count - 1) % (int)g_adv->pet_count;
                    }
                } else if (k == SDLK_DOWN) {
                    if (g_view == VIEW_SHEET || g_view == VIEW_LOG) {
                        g_sheet_scroll += 24;
                        if (g_sheet_scroll > 360) {
                            g_sheet_scroll = 360;
                        }
                    } else if (g_view == VIEW_PETS && g_adv && g_adv->pet_count) {
                        g_pet_sel = (g_pet_sel + 1) % (int)g_adv->pet_count;
                    }
                } else if (k == SDLK_TAB) {
                    g_hud = !g_hud;
                } else if (k == SDLK_r) {
                    gfx_park();
                    g_hatched = 0;
                    g_view = VIEW_ROSTER;
                } else if (k == SDLK_p) {
                    g_view = (g_view == VIEW_PETS) ? VIEW_HOME : VIEW_PETS;
                } else if (k == SDLK_h) {
                    tami_bandage(&adv);
                    toast_set("bandaged");
                } else if (k == SDLK_1) {
                    tami_set_field(&adv, TAMI_FIELD_GREENROAD);
                } else if (k == SDLK_2) {
                    tami_set_field(&adv, TAMI_FIELD_IRONPIT);
                } else if (k == SDLK_3) {
                    tami_set_field(&adv, TAMI_FIELD_MOONWOOD);
                } else if (k == SDLK_4) {
                    tami_set_field(&adv, TAMI_FIELD_BARROW);
                } else if (k == SDLK_5) {
                    tami_set_field(&adv, TAMI_FIELD_ASHFEN);
                } else if (k == SDLK_LEFTBRACKET && rate > 1) {
                    rate /= 2;
                    printf("rate %d\n", rate);
                } else if (k == SDLK_RIGHTBRACKET && rate < 400) {
                    rate *= 2;
                    printf("rate %d\n", rate);
                }
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (show_card) {
                    show_card = 0;
                } else {
                    float lx = (float)e.button.x, ly = (float)e.button.y;
                    SDL_RenderWindowToLogical(ren, e.button.x, e.button.y, &lx, &ly);
                    handle_click(&adv, (int)lx, (int)ly);
                }
            }
        }
        Uint32 now = SDL_GetTicks();
        float dt = (float)(now - last) / 1000.0f;
        last = now;
        if (dt > 0.1f) {
            dt = 0.1f;
        }
        anim += dt;
        if (g_gone_arm > 0) {
            g_gone_arm--;
        }
        if (g_toast_left > 0) {
            g_toast_left -= dt;
            if (g_toast_left < 0) {
                g_toast_left = 0;
            }
        }
        if (g_cut_t > 0) {
            g_cut_t -= dt;
            if (g_cut_t < 0) {
                g_cut_t = 0;
                g_cut = TAMI_CUT_NONE;
            }
        }
        int step = (int)(dt * (float)rate);
        if (step < 1) {
            static float acc;
            acc += dt * (float)rate;
            if (acc >= 1.0f) {
                step = (int)acc;
                acc -= (float)step;
            }
        }
        if (step > 0) {
            TamiReport r;
            tami_catchup(&adv, &rng, adv.last_tick_unix + step, &r);
            TamiCut c = tami_cut_from_report(&r);
            if (c != TAMI_CUT_NONE) {
                g_cut = c;
                g_cut_t = 0.85f;
            }
        }
        draw_home(ren, &adv, anim, show_card ? card : NULL);
        SDL_RenderPresent(ren);
    }
    if (shot) {
        save_bmp(ren, shot);
    }
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
