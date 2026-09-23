#ifndef TAMI_ANIM_H
#define TAMI_ANIM_H

#include "tami/sim.h"

typedef struct {
    int fig_dx, fig_dy;
    int fig_angle_tenth; /* 0.1 deg, LVGL-native; SDL uses /10.0 */
    int fig_scale;       /* 256 = 1× */
    int enemy_dx, enemy_dy;
    int enemy_angle_tenth;
    int enemy_alpha; /* 0..255 */
    int pet_dx, pet_dy;
    int fire_h_add;
    int alpha;
} TamiAnimPose;

typedef enum {
    TAMI_CUT_NONE = 0,
    TAMI_CUT_KILL,
    TAMI_CUT_LOOT,
    TAMI_CUT_SELL,
    TAMI_CUT_CAMP,
    TAMI_CUT_LEVEL,
    TAMI_CUT_PET,
    TAMI_CUT_GLEAM
} TamiCut;

void tami_anim_pose(const TamiAdventurer *a, float t, TamiAnimPose *out);
TamiCut tami_cut_from_report(const TamiReport *r);
const char *tami_cut_caption(TamiCut c);

#endif
