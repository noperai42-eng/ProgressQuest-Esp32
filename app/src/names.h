#ifndef TAMI_NAMES_H
#define TAMI_NAMES_H

#include "tami/sim.h"

void tami_gen_monster(TamiRng *rng, uint8_t *family, char *label, size_t n);
void tami_gen_elite(TamiRng *rng, uint8_t *family, char *label, size_t n);
void tami_gen_item(TamiRng *rng, uint8_t level, TamiItem *out);
void tami_gen_item_luck(TamiRng *rng, uint8_t level, uint8_t luck, TamiItem *out);
void tami_gen_item_slot(TamiRng *rng, uint8_t level, uint8_t slot, TamiItem *out);
void tami_gen_item_slot_luck(TamiRng *rng, uint8_t level, uint8_t slot, uint8_t luck, TamiItem *out);
void tami_gen_quest(TamiRng *rng, uint8_t level, TamiAdventurer *a);
void tami_gen_spell(TamiRng *rng, char *out, size_t n);
void tami_gen_given_name(TamiRng *rng, TamiPeople p, char *out, size_t n);

extern const char *tami_family_name[];
extern const int tami_family_count;

#endif
