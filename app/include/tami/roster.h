#ifndef TAMI_ROSTER_H
#define TAMI_ROSTER_H

#include "tami/sim.h"

#define TAMI_ROSTER_SLOTS 4
#define TAMI_ROSTER_VERSION 1

typedef struct {
    uint8_t used;
    uint8_t people;
    uint8_t calling;
    uint8_t level;
    uint8_t faded;
    char given_name[24];
} TamiRosterCard;

typedef struct {
    uint8_t version;
    uint8_t active; /* 0xFF = none */
    TamiRosterCard card[TAMI_ROSTER_SLOTS];
} TamiRoster;

void tami_roster_set_dir(const char *dir);
void tami_roster_clear(TamiRoster *r);
int tami_roster_load(TamiRoster *r);
int tami_roster_save_meta(const TamiRoster *r);
int tami_roster_used(const TamiRoster *r);
int tami_roster_first_empty(const TamiRoster *r); /* -1 if full */
void tami_roster_label(const TamiRoster *r, int slot, char *buf, size_t n);
int tami_roster_write_slot(TamiRoster *r, int slot, const TamiAdventurer *a, const TamiRng *rng);
int tami_roster_read_slot(int slot, TamiAdventurer *a, TamiRng *rng);
int tami_roster_erase_slot(TamiRoster *r, int slot);

#endif
