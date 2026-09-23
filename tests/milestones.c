#include "tami/sim.h"

#include <stdio.h>
#include <string.h>

static int fails;

#define CHECK(cond)                                                                                \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                        \
            fails++;                                                                               \
        }                                                                                          \
    } while (0)

static int slots_filled(const TamiAdventurer *a) {
    int n = 0;
    for (int s = 0; s < TAMI_SLOT_COUNT; s++) {
        if (tami_item_on(&a->equip[s])) {
            n++;
        }
    }
    return n;
}

static int run_seed(uint32_t seed, TamiPeople people, TamiCalling calling, int verbose) {
    TamiRng rng;
    TamiAdventurer a;
    TamiReport r;
    tami_rng_seed(&rng, seed);
    tami_hatch(&a, people, calling, &rng, 0);

    int first_kill = -1, first_loot = -1, first_market = -1, first_gold = -1;
    int first_buy = -1, lv2 = -1, lv5 = -1, lv10 = -1;
    int first_quest = -1, plot1 = -1, passing = 0, camped = 0;
    int saw_sell = 0;

    CHECK(a.level == 1);
    CHECK(tami_task_is_fight(&a));
    CHECK(a.field == TAMI_FIELD_GREENROAD);
    CHECK(tami_item_on(&a.equip[TAMI_SLOT_WEAPON]));
    CHECK(a.quest_need > 0);
    CHECK(a.kills == 0);

    /* Step 5s for 20 minutes to catch market, then 30s to 12h. */
    int64_t now = 0;
    int last_level = 1;
    uint32_t last_kills = 0;
    int last_slots = slots_filled(&a);

    while (now < 12 * 3600) {
        int step = (now < 20 * 60) ? 5 : 30;
        now += step;
        tami_catchup(&a, &rng, now, &r);

        if (first_kill < 0 && a.kills >= 1) {
            first_kill = (int)now;
        }
        if (first_loot < 0 && (a.inv_count > 0 || slots_filled(&a) > last_slots)) {
            first_loot = (int)now;
        }
        if (first_market < 0 && (a.task.kind == TAMI_TASK_ROAD_MARKET || a.task.kind == TAMI_TASK_SELL ||
                                 r.items_sold > 0)) {
            first_market = (int)now;
        }
        if (a.task.kind == TAMI_TASK_SELL) {
            saw_sell = 1;
        }
        if (first_gold < 0 && a.gold > 0) {
            first_gold = (int)now;
        }
        if (first_buy < 0 && (a.task.kind == TAMI_TASK_BUY ||
                              (slots_filled(&a) > 1 && a.equip[TAMI_SLOT_WEAPON].look != TAMI_LOOK_STICK))) {
            first_buy = (int)now;
        }
        if (lv2 < 0 && a.level >= 2) {
            lv2 = (int)now;
        }
        if (lv5 < 0 && a.level >= 5) {
            lv5 = (int)now;
        }
        if (lv10 < 0 && a.level >= 10) {
            lv10 = (int)now;
        }
        if (first_quest < 0 && (r.quests_done > 0 || a.quest_log_n > 0)) {
            first_quest = (int)now;
        }
        if (plot1 < 0 && a.plot_act >= 1) {
            plot1 = (int)now;
        }
        if (strstr(a.task.label, "passing")) {
            passing = 1;
        }
        if (r.camped || a.task.kind == TAMI_TASK_CAMP) {
            camped = 1;
        }
        last_level = a.level;
        last_kills = a.kills;
        last_slots = slots_filled(&a);
        (void)last_level;
        (void)last_kills;
    }

    if (verbose) {
        printf("seed %u %s %s | kill %ds loot %ds market %ds gold %ds buy/gear %ds | "
               "lv2 %ds lv5 %ds lv10 %ds | quest %ds plotI %ds | "
               "12h: lv %u kills %u gold %u slots %d quests_log %u plot %s spells %u%s%s%s\n",
               seed, tami_people_name(people), tami_calling_name(calling), first_kill, first_loot,
               first_market, first_gold, first_buy, lv2, lv5, lv10, first_quest, plot1, a.level,
               a.kills, a.gold, slots_filled(&a), a.quest_log_n, tami_plot_name(a.plot_act),
               a.spell_count, passing ? " passing" : "", camped ? " camp" : "",
               saw_sell ? " sell" : "");
    }

    CHECK(first_kill > 0 && first_kill < 120);          /* first kill under 2 min */
    CHECK(first_loot > 0 && first_loot <= first_kill + 30);
    CHECK(first_market > 0);                            /* pack fills, they go to town */
    CHECK(saw_sell == 1);
    CHECK(first_gold > 0);
    CHECK(lv2 > 0 && lv2 < 30 * 60);                    /* level 2 within 30 min */
    CHECK(lv5 > 0 && lv5 < 4 * 3600);                   /* level 5 within 4 h */
    CHECK(first_quest > 0);                             /* at least one quest done in 12 h */
    CHECK(a.level >= 5);
    CHECK(a.kills >= 50);
    CHECK(slots_filled(&a) >= 4);                       /* dressed, not still a stick-only hatchling */
    CHECK(a.spell_count >= 1);
    CHECK(a.plot_act >= 1 || a.plot_progress >= 20);
    CHECK(a.faded == 0);                                /* 12h of play without total neglect does not fade */

    /* Catch-up cap: 48h from here applies 12h more, not 48. */
    uint32_t kills_at_12 = a.kills;
    tami_catchup(&a, &rng, 12 * 3600 + 48 * 3600, &r);
    CHECK(r.applied_sec == TAMI_CATCHUP_CAP_SEC);
    CHECK(r.discarded_sec == 36u * 3600u);
    CHECK(a.kills > kills_at_12);

    /* Same seed is deterministic at t=600. */
    TamiAdventurer b;
    TamiRng rng2;
    tami_rng_seed(&rng2, seed);
    tami_hatch(&b, people, calling, &rng2, 0);
    tami_catchup(&b, &rng2, 600, &r);
    TamiAdventurer c;
    TamiRng rng3;
    tami_rng_seed(&rng3, seed);
    tami_hatch(&c, people, calling, &rng3, 0);
    tami_catchup(&c, &rng3, 600, &r);
    CHECK(b.kills == c.kills);
    CHECK(b.xp == c.xp);
    CHECK(b.level == c.level);
    CHECK(strcmp(b.task.label, c.task.label) == 0);

    return fails;
}

int main(void) {
    printf("=== progression milestones ===\n");
    run_seed(1, TAMI_PEOPLE_ORC, TAMI_CALL_WARRIOR, 1);
    run_seed(2, TAMI_PEOPLE_ELF, TAMI_CALL_MAGE, 1);
    run_seed(3, TAMI_PEOPLE_HUMAN, TAMI_CALL_RANGER, 1);
    run_seed(4, TAMI_PEOPLE_UNDEAD, TAMI_CALL_ROGUE, 1);
    run_seed(99, TAMI_PEOPLE_ORC, TAMI_CALL_WARRIOR, 1);

    if (fails) {
        fprintf(stderr, "%d milestone check(s) failed\n", fails);
        return 1;
    }
    printf("milestones ok\n");
    return 0;
}
