#pragma once

#include "tami/sim.h"

void tami_ui_init(TamiAdventurer *adv, TamiRng *rng, int64_t *now);
void tami_ui_refresh(const TamiAdventurer *adv);
void tami_ui_on_report(const TamiReport *r);
int tami_ui_hatched(void);
void tami_ui_do_hatch(TamiPeople p, TamiCalling c);
void tami_ui_resume(void);
void tami_ui_unhatch(void);
void tami_ui_show_roster(void);
/* Side-case keys: +1 PWR (up), -1 BOOT (down). Scrolls sheet/bag or picks a pet. */
void tami_ui_side_key(int dir);
int tami_ui_menu_open(void);
