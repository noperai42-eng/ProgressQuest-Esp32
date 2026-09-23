#pragma once

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
