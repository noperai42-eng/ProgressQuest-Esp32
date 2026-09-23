#pragma once

#include "tami/sim.h"
#include "tami/roster.h"

/* SPIFFS roster (4 slots) plus a one-time NVS import of the old single save. */
int tami_persist_init(void);
int tami_persist_load(TamiAdventurer *a, TamiRng *rng);
int tami_persist_save(const TamiAdventurer *a, const TamiRng *rng);
int tami_persist_wipe(void);
int tami_persist_select(int slot, TamiAdventurer *a, TamiRng *rng);
int tami_persist_create(int slot, const TamiAdventurer *a, const TamiRng *rng);
int tami_persist_erase_slot(int slot);
int tami_persist_active(void);
const TamiRoster *tami_persist_roster(void);
