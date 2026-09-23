#include "tami/anim.h"

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static float clampf(float v, float lo, float hi) {
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

void tami_anim_pose(const TamiAdventurer *a, float t, TamiAnimPose *out) {
    TamiAnimPose z;
    z.fig_dx = 0;
    z.fig_dy = 0;
    z.fig_angle_tenth = 0;
    z.fig_scale = 256;
    z.enemy_dx = 0;
    z.enemy_dy = 0;
    z.enemy_angle_tenth = 0;
    z.enemy_alpha = 255;
    z.pet_dx = 0;
    z.pet_dy = 0;
    z.fire_h_add = 0;
    z.alpha = 255;

    float p = (float)tami_task_pct(a) / 100.0f;
    p = clampf(p, 0.0f, 1.0f);

    if (a->faded) {
        z.fig_dy = 18;
        z.fig_angle_tenth = 80;
        z.alpha = 110;
        z.enemy_alpha = 0;
        *out = z;
        return;
    }

    if (a->hunger < 35) {
        z.fig_dy += 8;
    }
    if (a->rest < 35) {
        z.fig_dy += 6;
    }

    switch (a->task.kind) {
    case TAMI_TASK_DUNGEON:
    case TAMI_TASK_RESCUE:
    case TAMI_TASK_RAID:
    case TAMI_TASK_KILL: {
        /* Bar maps to wind-up then lunge so the strike lands as the bar fills. */
        if (p < 0.38f) {
            float u = p / 0.38f;
            z.fig_dy += (int)(sinf(t * 7.0f) * 3.0f);
            z.fig_angle_tenth = (int)(sinf(t * 5.0f) * 25.0f);
            z.enemy_dy = (int)(sinf(t * 6.0f + 1.0f) * 3.0f);
            (void)u;
        } else if (p < 0.72f) {
            float u = (p - 0.38f) / 0.34f;
            z.fig_dx = (int)(-16.0f * u);
            z.fig_angle_tenth = (int)(-90.0f * u);
            z.fig_dy += (int)(4.0f * u);
            z.enemy_dx = (int)(-4.0f * u);
        } else {
            float u = (p - 0.72f) / 0.28f;
            z.fig_dx = (int)(-16.0f + 42.0f * u);
            z.fig_angle_tenth = (int)(-90.0f + 220.0f * u);
            z.fig_dy += (int)(4.0f - 10.0f * u);
            z.enemy_dx = (int)(8.0f + 28.0f * u);
            z.enemy_angle_tenth = (int)(180.0f * u);
            z.enemy_dy = (int)(-6.0f * u);
            if (u > 0.7f) {
                z.enemy_alpha = (int)(255.0f * (1.0f - (u - 0.7f) / 0.3f));
            }
        }
        z.pet_dx = (int)(4.0f + (p > 0.72f ? 18.0f * ((p - 0.72f) / 0.28f) : 0.0f));
        z.pet_dy = (int)(sinf(t * 8.0f) * 4.0f);
        break;
    }
    case TAMI_TASK_ROAD_MARKET:
    case TAMI_TASK_ROAD_FIELDS: {
        float step = t * 6.0f + p * 12.0f;
        z.fig_dx = (int)(sinf(step) * 10.0f);
        z.fig_dy += (int)(fabsf(sinf(step * 2.0f)) * 7.0f);
        z.fig_angle_tenth = (int)(sinf(step) * 40.0f);
        z.pet_dx = (int)(sinf(step - 0.6f) * 8.0f);
        z.pet_dy = (int)(fabsf(sinf(step * 2.0f - 0.6f)) * 5.0f);
        z.enemy_alpha = 0;
        break;
    }
    case TAMI_TASK_SELL:
    case TAMI_TASK_BUY: {
        float bow = sinf(p * (float)M_PI);
        z.fig_dx = (int)(8.0f * bow);
        z.fig_angle_tenth = (int)(70.0f * bow);
        z.fig_dy += (int)(6.0f * bow);
        z.pet_dy = (int)(sinf(t * 3.0f) * 2.0f);
        z.enemy_alpha = 0;
        break;
    }
    case TAMI_TASK_CAMP: {
        z.fig_dy += 12 + (int)(sinf(t * 2.2f) * 2.0f);
        z.fig_angle_tenth = -30 + (int)(sinf(t * 2.2f) * 15.0f);
        z.fire_h_add = (int)(sinf(t * 7.0f) * 6.0f);
        z.pet_dx = (int)(sinf(t * 1.7f) * 3.0f);
        z.enemy_alpha = 0;
        break;
    }
    case TAMI_TASK_DOWNED: {
        /* Slump with offset, not a 78° sprite rotate — LVGL software-rotates
         * RGB565A8 inside the round clip and stalls the disc. */
        z.fig_dy += 28;
        z.fig_dx += 8;
        z.fig_angle_tenth = 90;
        z.alpha = 180;
        z.enemy_alpha = 0;
        break;
    }
    default:
        z.fig_dy += (int)(sinf(t * 3.0f) * 2.0f);
        z.enemy_alpha = 0;
        break;
    }

    if (a->morale < 30) {
        z.fig_angle_tenth -= 20;
    }
    *out = z;
}

TamiCut tami_cut_from_report(const TamiReport *r) {
    if (!r) {
        return TAMI_CUT_NONE;
    }
    if (r->legendaries_looted || r->best_loot_rarity >= TAMI_RARITY_RARE) {
        return TAMI_CUT_GLEAM;
    }
    if (r->levels_gained) {
        return TAMI_CUT_LEVEL;
    }
    if (r->pets_found) {
        return TAMI_CUT_PET;
    }
    if (r->kills) {
        return TAMI_CUT_KILL;
    }
    if (r->items_looted) {
        return TAMI_CUT_LOOT;
    }
    if (r->items_sold) {
        return TAMI_CUT_SELL;
    }
    if (r->camped) {
        return TAMI_CUT_CAMP;
    }
    return TAMI_CUT_NONE;
}

const char *tami_cut_caption(TamiCut c) {
    switch (c) {
    case TAMI_CUT_KILL:
        return "killed!";
    case TAMI_CUT_LOOT:
        return "loot!";
    case TAMI_CUT_SELL:
        return "sold";
    case TAMI_CUT_CAMP:
        return "camp";
    case TAMI_CUT_LEVEL:
        return "level up";
    case TAMI_CUT_PET:
        return "a stray!";
    case TAMI_CUT_GLEAM:
        return "gleam!";
    default:
        return "";
    }
}
