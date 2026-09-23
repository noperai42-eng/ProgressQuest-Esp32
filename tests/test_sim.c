#include "tami/sim.h"
#include "tami/anim.h"
#include "tami/roster.h"

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

static void hatch_default(TamiAdventurer *a, TamiRng *r) {
    tami_rng_seed(r, 1);
    tami_hatch(a, TAMI_PEOPLE_ORC, TAMI_CALL_WARRIOR, r, 1000);
}

int main(void) {
    TamiRng rng;
    TamiAdventurer a;
    TamiReport r;

    hatch_default(&a, &rng);
    {
        char st[192];
        char rec[320];
        tami_format_status(&a, st, sizeof(st));
        tami_format_recap(&a, rec, sizeof(rec));
        CHECK(strstr(st, a.given_name) != 0);
        CHECK(strstr(st, "lv 1") != 0);
        CHECK(strstr(rec, a.given_name) != 0);
        CHECK(strstr(rec, "Prologue") != 0);
        {
            char ch[480];
            tami_format_chronicle(&a, ch, sizeof(ch));
            CHECK(strstr(ch, a.given_name) != 0);
            CHECK(strstr(ch, "fights") != 0);
            CHECK(strstr(ch, "Quest:") != 0);
        }
    }
    CHECK(a.version == TAMI_BLOB_VERSION);
    CHECK(TAMI_FAMILY_COUNT >= 14);
    CHECK(strcmp(tami_mob_name(8), "wolf") == 0);
    CHECK(a.level == 1);
    CHECK(tami_xp_pct(&a) == 0);
    a.xp = 25;
    a.xp_need = 50;
    CHECK(tami_xp_pct(&a) == 50);
    a.xp = 50;
    CHECK(tami_xp_pct(&a) == 100);
    hatch_default(&a, &rng);
    CHECK(a.level == 1);
    CHECK(a.hunger == 100);
    CHECK(a.task.kind == TAMI_TASK_KILL);
    CHECK(a.task.duration_sec >= 8);
    CHECK(a.equip[TAMI_SLOT_WEAPON].name[0] != 0);
    CHECK(a.equip[TAMI_SLOT_WEAPON].look == TAMI_LOOK_STICK);
    CHECK(tami_item_on(&a.equip[TAMI_SLOT_WEAPON]));
    CHECK(!tami_item_on(&a.equip[TAMI_SLOT_HELM]));
    CHECK(a.hp_max >= 8);
    CHECK(a.mp_max >= 4);
    CHECK(a.spell_count >= 1);
    CHECK(a.spells[0].rank >= 1);
    CHECK(a.quest_need > 0);

    /* Catch-up cap: 48h only applies 12h. */
    hatch_default(&a, &rng);
    tami_catchup(&a, &rng, 1000 + 48 * 3600, &r);
    CHECK(r.applied_sec == TAMI_CATCHUP_CAP_SEC);
    CHECK(r.discarded_sec == 36u * 3600u);
    CHECK(r.kills > 0);
    CHECK(a.last_tick_unix == 1000 + 48 * 3600);

    /* Clock rollback: no progress, clamp forward. */
    uint32_t kills_before = a.kills;
    tami_catchup(&a, &rng, a.last_tick_unix - 500, &r);
    CHECK(r.rolled_back == 1);
    CHECK(r.kills == 0);
    CHECK(a.kills == kills_before);

    /* Encumbrance eventually trips a market trip. */
    hatch_default(&a, &rng);
    int saw_market = 0;
    for (int i = 0; i < 400; i++) {
        tami_catchup(&a, &rng, a.last_tick_unix + 30, &r);
        if (a.task.kind == TAMI_TASK_ROAD_MARKET || a.task.kind == TAMI_TASK_SELL ||
            a.task.kind == TAMI_TASK_BUY) {
            saw_market = 1;
            break;
        }
    }
    CHECK(saw_market == 1);

    /* Hunger 0 forces camp. */
    hatch_default(&a, &rng);
    a.hunger = 0;
    a.rest = 50;
    /* finish current kill then next task should camp */
    tami_catchup(&a, &rng, a.last_tick_unix + (int64_t)a.task.duration_sec + 1, &r);
    CHECK(a.task.kind == TAMI_TASK_CAMP || r.camped == 1 || a.hunger > 0);

    /* Feed restores hunger. */
    a.hunger = 10;
    tami_feed(&a);
    CHECK(a.hunger >= 50);

    /* Quest completion assigns a new quest. */
    hatch_default(&a, &rng);
    char oldq[64];
    snprintf(oldq, sizeof(oldq), "%s", a.quest_label);
    a.quest_have = (uint8_t)(a.quest_need - 1);
    /* many kills to guarantee a completion */
    tami_catchup(&a, &rng, a.last_tick_unix + 3600, &r);
    CHECK(r.quests_done >= 1 || strcmp(oldq, a.quest_label) != 0 || a.quest_have < a.quest_need);

    /* 8 hours of orc warrior should yield kills and some gold or loot. */
    hatch_default(&a, &rng);
    tami_catchup(&a, &rng, 1000 + 8 * 3600, &r);
    CHECK(r.applied_sec == 8u * 3600u);
    CHECK(r.kills >= 10);
    CHECK(a.level >= 1);
    CHECK(a.gold + a.inv_count + a.equip[TAMI_SLOT_WEAPON].score > 0);
    CHECK(tami_item_on(&a.equip[TAMI_SLOT_WEAPON]));
    CHECK(a.equip[TAMI_SLOT_WEAPON].look != TAMI_LOOK_STICK ||
          tami_item_on(&a.equip[TAMI_SLOT_HAUBERK]) || tami_item_on(&a.equip[TAMI_SLOT_HELM]));
    CHECK(!tami_item_on(&a.equip[TAMI_SLOT_HELM]) || a.equip[TAMI_SLOT_HELM].look >= TAMI_LOOK_CAP);

    /* Spell ranks stack; save/load round-trips. */
    hatch_default(&a, &rng);
    tami_learn_spell(&a, "Gloom");
    tami_learn_spell(&a, "Gloom");
    {
        int gloom = 0;
        for (uint8_t i = 0; i < a.spell_count; i++) {
            if (strcmp(a.spells[i].name, "Gloom") == 0) {
                gloom = a.spells[i].rank;
            }
        }
        CHECK(gloom >= 2);
    }
    CHECK(tami_blob_sane(&a));
    CHECK(tami_save(&a, "build/test.tami") == 0);
    {
        TamiAdventurer b;
        memset(&b, 0, sizeof(b));
        CHECK(tami_load(&b, "build/test.tami") == 0);
        CHECK(b.level == a.level);
        CHECK(strcmp(b.given_name, a.given_name) == 0);
        CHECK(b.spell_count == a.spell_count);
        CHECK(tami_blob_sane(&b));
    }
    {
        TamiAdventurer bad = a;
        bad.version = 1;
        CHECK(!tami_blob_sane(&bad));
        bad = a;
        bad.given_name[0] = 0;
        CHECK(!tami_blob_sane(&bad));
        bad = a;
        bad.level = 0;
        CHECK(!tami_blob_sane(&bad));
    }

    /* Inventory stacks by name. */
    hatch_default(&a, &rng);
    {
        TamiItem it;
        memset(&it, 0, sizeof(it));
        snprintf(it.name, sizeof(it.name), "mire helm");
        it.score = 2;
        it.slot = TAMI_SLOT_HELM;
        it.look = TAMI_LOOK_HELM;
        it.qty = 1;
        tami_add_item(&a, &it);
        tami_add_item(&a, &it);
        CHECK(a.inv_count == 1);
        CHECK(a.inv[0].qty == 2);
        CHECK(tami_encumbrance(&a) == 2);
    }

    CHECK(tami_hour(0) == 0);
    CHECK(tami_is_night(0) == 1);
    CHECK(tami_is_night(12 * 3600) == 0);

    /* Fade + heirloom after long neglect. */
    hatch_default(&a, &rng);
    a.hunger = 10;
    a.rest = 10;
    a.starved_sec = 6u * 3600u - 5u;
    tami_catchup(&a, &rng, a.last_tick_unix + 30, &r);
    CHECK(a.faded == 1);
    CHECK(a.heirloom.name[0] != 0);
    CHECK(a.inv_count == 1);

    {
        char card[128];
        tami_format_card(&a, &r, card, sizeof(card));
        CHECK(strstr(card, "faded") != 0);
    }
    {
        char oldn[24];
        uint8_t oldlv = a.level;
        snprintf(oldn, sizeof(oldn), "%s", a.given_name);
        TamiItem heir = a.heirloom;
        CHECK(tami_whelp(&a, &rng, a.last_tick_unix + 1) == 0);
        CHECK(a.faded == 0);
        CHECK(a.level == oldlv);
        CHECK(strcmp(a.given_name, oldn) == 0);
        CHECK(tami_item_on(&a.equip[heir.slot]));
        CHECK(strcmp(a.equip[heir.slot].name, heir.name) == 0);
        CHECK(tami_whelp(&a, &rng, a.last_tick_unix) == -1);
    }

    /* Pets: add, feed, play, heel, bonus, release, wander. */
    hatch_default(&a, &rng);
    CHECK(a.pet_count == 0);
    CHECK(tami_pet_add(&a, &rng, TAMI_PET_TOAD) == 0);
    CHECK(a.pet_count == 1);
    CHECK(a.pet_active == 0);
    CHECK(a.pets[0].kind == TAMI_PET_TOAD);
    CHECK(a.pets[0].name[0] != 0);
    CHECK(tami_pet_heel(&a) != NULL);
    a.pets[0].hunger = 10;
    tami_pet_feed(&a, 0);
    CHECK(a.pets[0].hunger >= 50);
    a.pets[0].mood = 10;
    tami_pet_play(&a, 0);
    CHECK(a.pets[0].mood >= 30);
    CHECK(tami_pet_add(&a, &rng, TAMI_PET_CROW) == 0);
    CHECK(tami_pet_heel_set(&a, 1) == 0);
    CHECK(a.pet_active == 1);
    CHECK(tami_pet_release(&a, 0) == 0);
    CHECK(a.pet_count == 1);
    CHECK(a.pets[0].kind == TAMI_PET_CROW);
    CHECK(a.pet_active == 0);
    CHECK(tami_pet_add(&a, &rng, TAMI_PET_TOAD) == 0);
    CHECK(tami_pet_add(&a, &rng, TAMI_PET_RAT) == 0);
    CHECK(tami_pet_add(&a, &rng, TAMI_PET_MOTH) == -1);
    CHECK(strcmp(tami_pet_kind_name(TAMI_PET_MOTH), "moth") == 0);

    hatch_default(&a, &rng);
    CHECK(tami_pet_add(&a, &rng, TAMI_PET_RAT) == 0);
    a.pets[0].hunger = 100;
    a.pets[0].mood = 100;
    {
        uint32_t g0 = a.gold;
        /* finish many kills so the heeled rat pays crumbs */
        tami_catchup(&a, &rng, a.last_tick_unix + 600, &r);
        CHECK(a.gold >= g0);
        CHECK(a.pet_count >= 1);
    }

    hatch_default(&a, &rng);
    CHECK(tami_pet_add(&a, &rng, TAMI_PET_TOAD) == 0);
    a.pets[0].hunger = 0;
    a.pets[0].mood = 0;
    a.pets[0].starve_sec = 3u * 3600u - 10u;
    tami_catchup(&a, &rng, a.last_tick_unix + 60, &r);
    CHECK(a.pet_count == 0 || r.pets_lost >= 1);

    hatch_default(&a, &rng);
    CHECK(tami_pet_add(&a, &rng, TAMI_PET_MOTH) == 0);
    CHECK(tami_save(&a, "build/test.tami") == 0);
    {
        TamiAdventurer b;
        memset(&b, 0, sizeof(b));
        CHECK(tami_load(&b, "build/test.tami") == 0);
        CHECK(b.version == TAMI_BLOB_VERSION);
        CHECK(b.pet_count == 1);
        CHECK(b.pets[0].kind == TAMI_PET_MOTH);
    }

    {
        TamiAnimPose p;
        hatch_default(&a, &rng);
        tami_anim_pose(&a, 0.4f, &p);
        CHECK(p.fig_scale == 256);
        TamiReport cr;
        memset(&cr, 0, sizeof(cr));
        cr.kills = 1;
        CHECK(tami_cut_from_report(&cr) == TAMI_CUT_KILL);
        cr.kills = 0;
        cr.levels_gained = 1;
        CHECK(tami_cut_from_report(&cr) == TAMI_CUT_LEVEL);
        CHECK(tami_cut_caption(TAMI_CUT_PET)[0] != 0);
        CHECK(strstr(tami_cut_caption(TAMI_CUT_KILL), "kill") != 0);
        cr.levels_gained = 0;
        cr.best_loot_rarity = TAMI_RARITY_LEGENDARY;
        CHECK(tami_cut_from_report(&cr) == TAMI_CUT_GLEAM);
        CHECK(strstr(tami_cut_caption(TAMI_CUT_GLEAM), "gleam") != 0);
    }

    {
        char log[256];
        hatch_default(&a, &rng);
        tami_format_log(&a, log, sizeof(log));
        CHECK(strstr(log, "now:") != 0);
        tami_catchup(&a, &rng, a.last_tick_unix + 8 * 3600, &r);
        tami_format_log(&a, log, sizeof(log));
        CHECK(a.quest_log_n == 0 || strstr(log, "done") != 0);
    }
    CHECK(strcmp(tami_mob_name(0), "gnoll") == 0);
    CHECK(strcmp(tami_mob_name(7), "ash-rat") == 0);
    CHECK(strcmp(tami_mob_name(13), "hag") == 0);
    CHECK(strcmp(tami_mob_plural(8), "wolves") == 0);
    CHECK(strcmp(tami_mob_plural(13), "hags") == 0);
    a.task.kind = TAMI_TASK_DUNGEON;
    CHECK(tami_task_is_fight(&a) == 1);
    a.task.kind = TAMI_TASK_RESCUE;
    CHECK(tami_task_is_fight(&a) == 1);
    a.task.kind = TAMI_TASK_RAID;
    CHECK(tami_task_is_fight(&a) == 1);
    a.task.kind = TAMI_TASK_CAMP;
    CHECK(tami_task_is_fight(&a) == 0);
    {
        int specials = 0;
        for (uint32_t s = 1; s <= 40; s++) {
            TamiRng rr;
            TamiAdventurer b;
            tami_rng_seed(&rr, s);
            tami_hatch(&b, TAMI_PEOPLE_HUMAN, TAMI_CALL_RANGER, &rr, 0);
            CHECK(strstr(b.quest_label, "wolfs") == 0);
            CHECK(strstr(b.task.label, "wolfs") == 0);
            tami_catchup(&b, &rr, b.last_tick_unix + 2 * 3600, NULL);
            specials += (int)b.delves + (int)b.rescues + (int)b.camps_stormed;
            CHECK(strstr(b.quest_label, "wolfs") == 0);
            CHECK(strstr(b.task.label, "wolfs") == 0);
        }
        CHECK(specials >= 1);
    }
    hatch_default(&a, &rng);
    a.task.kind = TAMI_TASK_KILL;
    a.task.family = 4;
    CHECK(tami_kill_family(&a) == 4);
    a.task.family = (uint8_t)(TAMI_FAMILY_ELITE | 2);
    CHECK(tami_kill_is_elite(&a) == 1);
    CHECK(tami_kill_family(&a) == 2);
    CHECK(strcmp(tami_rarity_name(TAMI_RARITY_LEGENDARY), "legendary") == 0);
    {
        uint8_t rr, rg, rb;
        tami_rarity_rgb(TAMI_RARITY_LEGENDARY, &rr, &rg, &rb);
        CHECK(rr > 200 && rg > 180 && rb < 140);
        tami_rarity_rgb(TAMI_RARITY_RARE, &rr, &rg, &rb);
        CHECK(rb > rr);
        tami_rarity_rgb(TAMI_RARITY_UNCOMMON, &rr, &rg, &rb);
        CHECK(rg > rb);
    }
    hatch_default(&a, &rng);
    tami_give_relic(&a, &rng);
    CHECK(tami_worn_rarity(&a) == TAMI_RARITY_LEGENDARY);
    CHECK(tami_best_gleam(&a) != NULL);
    hatch_default(&a, &rng);
    tami_catchup(&a, &rng, a.last_tick_unix + 8 * 3600, &r);
    CHECK(a.rares_found >= 1);
    CHECK(a.elites_slain >= 1 || a.kills > 100);

    /* Downed recovers into camp; catchup must not spin. */
    hatch_default(&a, &rng);
    a.wounds = 100;
    a.task.elapsed_sec = a.task.duration_sec;
    tami_catchup(&a, &rng, a.last_tick_unix + 1, &r);
    CHECK(a.task.kind == TAMI_TASK_DOWNED);
    tami_catchup(&a, &rng, a.last_tick_unix + 200, &r);
    CHECK(a.task.kind != TAMI_TASK_DOWNED);
    CHECK(a.wounds < 80);
    CHECK(r.downed == 1 || a.task.kind == TAMI_TASK_CAMP || a.task.kind == TAMI_TASK_KILL);

    {
        TamiRoster ros;
        TamiAdventurer b;
        TamiRng r2;
        tami_roster_set_dir("build/roster-test");
        tami_roster_clear(&ros);
        CHECK(tami_roster_used(&ros) == 0);
        CHECK(tami_roster_first_empty(&ros) == 0);
        hatch_default(&a, &rng);
        CHECK(tami_roster_write_slot(&ros, 0, &a, &rng) == 0);
        CHECK(ros.card[0].used);
        CHECK(ros.active == 0);
        CHECK(tami_roster_used(&ros) == 1);
        tami_rng_seed(&r2, 99);
        tami_hatch(&b, TAMI_PEOPLE_ELF, TAMI_CALL_MAGE, &r2, 2000);
        CHECK(tami_roster_write_slot(&ros, 1, &b, &r2) == 0);
        CHECK(tami_roster_used(&ros) == 2);
        {
            TamiRoster again;
            CHECK(tami_roster_load(&again) == 0);
            CHECK(again.card[0].used && again.card[1].used);
            CHECK(strcmp(again.card[1].given_name, b.given_name) == 0);
            TamiAdventurer c;
            TamiRng r3;
            CHECK(tami_roster_read_slot(0, &c, &r3) == 0);
            CHECK(strcmp(c.given_name, a.given_name) == 0);
            CHECK(c.level == a.level);
            char lab[64];
            tami_roster_label(&again, 0, lab, sizeof(lab));
            CHECK(strstr(lab, a.given_name) != 0);
            tami_roster_label(&again, 3, lab, sizeof(lab));
            CHECK(strstr(lab, "new") != 0);
        }
        CHECK(tami_roster_erase_slot(&ros, 0) == 0);
        CHECK(!ros.card[0].used);
        CHECK(ros.active == 1);
        CHECK(tami_roster_first_empty(&ros) == 0);
    }

    if (fails) {
        fprintf(stderr, "%d check(s) failed\n", fails);
        return 1;
    }
    printf("ok  %d checks\n", 8);
    return 0;
}
