/* Desk progression harness: every people×calling, a work-day recap,
 * field tour, late plot, downed, fade. Same app/ the device runs. */
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

static const TamiPeople k_p[TAMI_PEOPLE_COUNT] = {
    TAMI_PEOPLE_HUMAN, TAMI_PEOPLE_ORC, TAMI_PEOPLE_ELF, TAMI_PEOPLE_UNDEAD};
static const TamiCalling k_c[TAMI_CALL_COUNT] = {
    TAMI_CALL_WARRIOR, TAMI_CALL_RANGER, TAMI_CALL_MAGE, TAMI_CALL_ROGUE};

typedef struct {
    int kills, quests, lv, plot, pets, market, camp, passing, rares, legend, elites;
    char recap[400];
    char name[24];
} Day;

static void play(TamiAdventurer *a, TamiRng *rng, int64_t *now, int sec, TamiReport *r) {
    *now += sec;
    tami_catchup(a, rng, *now, r);
}

static Day work_day(TamiPeople p, TamiCalling c, uint32_t seed) {
    TamiRng rng;
    TamiAdventurer a;
    TamiReport r;
    Day d;
    memset(&d, 0, sizeof(d));
    tami_rng_seed(&rng, seed);
    tami_hatch(&a, p, c, &rng, 0);
    int64_t now = 0;
    int market = 0, camp = 0, passing = 0;
    while (now < 8 * 3600) {
        play(&a, &rng, &now, 60, &r);
        if (a.task.kind == TAMI_TASK_ROAD_MARKET || a.task.kind == TAMI_TASK_SELL ||
            a.task.kind == TAMI_TASK_BUY) {
            market = 1;
        }
        if (r.camped || a.task.kind == TAMI_TASK_CAMP) {
            camp = 1;
        }
        if (strstr(a.task.label, "passing")) {
            passing = 1;
        }
    }
    d.kills = (int)a.kills;
    d.quests = (int)a.quest_log_n;
    d.lv = (int)a.level;
    d.plot = (int)a.plot_act;
    d.pets = (int)a.pet_count;
    d.rares = (int)a.rares_found;
    d.legend = (int)a.legendaries_found;
    d.elites = (int)a.elites_slain;
    d.market = market;
    d.camp = camp;
    d.passing = passing;
    snprintf(d.name, sizeof(d.name), "%s", a.given_name);
    tami_format_recap(&a, d.recap, sizeof(d.recap));
    CHECK(a.given_name[0] != 0);
    CHECK(a.kills >= 20);
    CHECK(a.level >= 5);
    CHECK(a.quest_label[0] != 0);
    CHECK(a.faded == 0);
    CHECK(d.recap[0] != 0);
    CHECK(d.rares >= 1);
    CHECK(a.delves + a.rescues + a.camps_stormed >= 1);
    return d;
}

static void field_tour(void) {
    TamiRng rng;
    TamiAdventurer a;
    TamiReport r;
    tami_rng_seed(&rng, 7);
    tami_hatch(&a, TAMI_PEOPLE_ORC, TAMI_CALL_WARRIOR, &rng, 0);
    int64_t now = 0;
    play(&a, &rng, &now, 8 * 3600, &r);
    int seen = 0;
    for (int f = 0; f < TAMI_FIELD_COUNT; f++) {
        CHECK(tami_set_field(&a, (TamiField)f) == 0);
        play(&a, &rng, &now, 20 * 60, &r);
        CHECK(a.field == (TamiField)f);
        seen |= 1 << f;
        printf("  field %s  lv %u  %s\n", tami_field_name((TamiField)f), (unsigned)a.level,
               a.task.label);
    }
    CHECK(seen == (1 << TAMI_FIELD_COUNT) - 1);
}

static void late_game(void) {
    TamiRng rng;
    TamiAdventurer a;
    TamiReport r;
    tami_rng_seed(&rng, 11);
    tami_hatch(&a, TAMI_PEOPLE_ELF, TAMI_CALL_MAGE, &rng, 0);
    int64_t now = 0;
    /* Three 12h gulps: a long weekend on the disc. */
    for (int i = 0; i < 3; i++) {
        play(&a, &rng, &now, 12 * 3600, &r);
    }
    tami_set_field(&a, TAMI_FIELD_ASHFEN);
    play(&a, &rng, &now, 3600, &r);
    char recap[400];
    tami_format_recap(&a, recap, sizeof(recap));
    printf("  late  %s  lv %u  plot %s  field %s  pets %u  spells %u  rares %u  relics %u  elites %u\n",
           recap, (unsigned)a.level,
           tami_plot_name(a.plot_act), tami_field_name(a.field), (unsigned)a.pet_count,
           (unsigned)a.spell_count, (unsigned)a.rares_found, (unsigned)a.legendaries_found,
           (unsigned)a.elites_slain);
    CHECK(a.level >= 10);
    CHECK(a.plot_act >= 2);
    CHECK(a.field == TAMI_FIELD_ASHFEN);
    CHECK(a.faded == 0);
    if (a.plot_act < 5) {
        printf("  note: Act V not reached in 37h (plot %s %u%%) — late-game still thin\n",
               tami_plot_name(a.plot_act), (unsigned)a.plot_progress);
    }
}

static void care_edges(void) {
    TamiRng rng;
    TamiAdventurer a;
    TamiReport r;
    tami_rng_seed(&rng, 3);
    tami_hatch(&a, TAMI_PEOPLE_HUMAN, TAMI_CALL_RANGER, &rng, 0);
    int64_t now = 0;
    a.wounds = 100;
    a.task.elapsed_sec = a.task.duration_sec;
    play(&a, &rng, &now, 1, &r);
    CHECK(a.task.kind == TAMI_TASK_DOWNED);
    play(&a, &rng, &now, 200, &r);
    CHECK(a.task.kind != TAMI_TASK_DOWNED);

    tami_hatch(&a, TAMI_PEOPLE_UNDEAD, TAMI_CALL_ROGUE, &rng, now);
    a.hunger = 0;
    a.rest = 0;
    a.starved_sec = 6u * 3600u - 5u;
    play(&a, &rng, &now, 30, &r);
    CHECK(a.faded == 1);
    CHECK(a.heirloom.name[0] != 0);
    char recap[320];
    tami_format_recap(&a, recap, sizeof(recap));
    printf("  faded  %s  heirloom %s\n", recap, a.heirloom.name);
}

int main(void) {
    printf("=== work day (8h) — 16 hatchlings ===\n");
    uint32_t seed = 1;
    int pet_days = 0;
    int relic_days = 0;
    for (int pi = 0; pi < TAMI_PEOPLE_COUNT; pi++) {
        for (int ci = 0; ci < TAMI_CALL_COUNT; ci++) {
            Day d = work_day(k_p[pi], k_c[ci], seed++);
            if (d.pets > 0) {
                pet_days++;
            }
            if (d.legend > 0) {
                relic_days++;
            }
            printf("  %s %s  %s  lv%d  q%d  plot%d  pet%d  rare%d  relic%d  elite%d  %s%s%s\n",
                   tami_people_name(k_p[pi]), tami_calling_name(k_c[ci]), d.name, d.lv, d.quests,
                   d.plot, d.pets, d.rares, d.legend, d.elites, d.market ? "town " : "",
                   d.camp ? "camp " : "", d.passing ? "passing" : "");
            printf("    %s\n", d.recap);
        }
    }
    printf("=== field tour ===\n");
    field_tour();
    printf("=== late game ===\n");
    late_game();
    printf("=== care edges ===\n");
    care_edges();
    printf("strays on %d/16 work days, relics on %d/16\n", pet_days, relic_days);
    CHECK(relic_days >= 1);

    if (fails) {
        fprintf(stderr, "%d path check(s) failed\n", fails);
        return 1;
    }
    printf("paths ok\n");
    return 0;
}
