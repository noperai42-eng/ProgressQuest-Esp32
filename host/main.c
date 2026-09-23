#include "tami/sim.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

static void print_bar(int pct, int width) {
    if (pct < 0) {
        pct = 0;
    }
    if (pct > 100) {
        pct = 100;
    }
    int f = (pct * width) / 100;
    putchar('[');
    for (int i = 0; i < width; i++) {
        putchar(i < f ? '#' : '-');
    }
    printf("] %3d%%", pct);
}

static void print_sheet(const TamiAdventurer *a) {
    char tbar[128];
    tami_task_bar(a, tbar, sizeof(tbar));
    printf("\n======== Tami desk sim ========\n");
    printf("%s  %s %s   lv %u   HP %u  MP %u\n", a->given_name, a->people_name, a->calling_name,
           a->level, a->hp_max, a->mp_max);
    printf("Field: %s  (%d-%d)   kills %u\n", tami_field_name(a->field),
           tami_field_band_lo(a->field), tami_field_band_hi(a->field), a->kills);
    printf("XP    ");
    print_bar(tami_xp_pct(a), 24);
    printf("  %u/%u\n", a->xp, a->xp_need);
    printf("Quest ");
    print_bar(a->quest_need ? (int)((a->quest_have * 100) / a->quest_need) : 0, 24);
    printf("  %s\n", a->quest_label);
    printf("Plot  %s ", tami_plot_name(a->plot_act));
    print_bar(a->plot_progress, 16);
    printf("  %s\n", tami_plot_epitaph(a->plot_act));
    printf("Pack  ");
    print_bar((tami_encumbrance_max(a) ? (tami_encumbrance(a) * 100) / tami_encumbrance_max(a) : 0),
              16);
    printf("  %d/%d cubits   gold %u\n", tami_encumbrance(a), tami_encumbrance_max(a), a->gold);
    printf("Needs hunger %3u  rest %3u  morale %3u  wounds %3u\n", a->hunger, a->rest, a->morale,
           a->wounds);
    printf("STR %u  CON %u  DEX %u  INT %u  WIS %u  CHA %u\n", a->str, a->con, a->dex, a->intel,
           a->wis, a->cha);
    if (a->banner_left && a->banner[0]) {
        printf("** %s **\n", a->banner);
    }
    printf("NOW  %s\n", tbar);
    printf("-- equipment --\n");
    for (int s = 0; s < TAMI_SLOT_COUNT; s++) {
        const TamiItem *it = &a->equip[s];
        printf("  %-8s %s\n", tami_slot_name((TamiSlot)s), it->name[0] ? it->name : "(empty)");
    }
    printf("-- inventory (%u) --\n", a->inv_count);
    int show = a->inv_count < 8 ? a->inv_count : 8;
    for (int i = 0; i < show; i++) {
        int idx = (int)a->inv_count - 1 - i;
        uint8_t q = a->inv[idx].qty ? a->inv[idx].qty : 1;
        if (q > 1) {
            printf("  %s x%u%s\n", a->inv[idx].name, q, i == 0 ? "  <new" : "");
        } else {
            printf("  %s%s\n", a->inv[idx].name, i == 0 ? "  <new" : "");
        }
    }
    if (a->inv_count > 8) {
        printf("  … %u more\n", a->inv_count - 8);
    }
    printf("-- spells --");
    for (uint8_t i = 0; i < a->spell_count; i++) {
        if (a->spells[i].rank > 1) {
            printf(" %s %u", a->spells[i].name, a->spells[i].rank);
        } else {
            printf(" %s", a->spells[i].name);
        }
    }
    printf("\n");
    if (a->quest_log_n) {
        printf("-- quest log --\n");
        int n = a->quest_log_n < 4 ? a->quest_log_n : 4;
        for (int i = 0; i < n; i++) {
            printf("  %s\n", a->quest_log[a->quest_log_n - 1 - i]);
        }
    }
    printf("-- pets (%u) --\n", a->pet_count);
    for (uint8_t i = 0; i < a->pet_count; i++) {
        const TamiPet *p = &a->pets[i];
        printf("  %s%s  %s  hung %u  mood %u  (%s)\n", p->name,
               i == a->pet_active ? "*" : " ", tami_pet_kind_name((TamiPetKind)p->kind), p->hunger,
               p->mood, tami_pet_trait((TamiPetKind)p->kind));
    }
    if (a->faded) {
        printf("*** faded ***\n");
    }
    printf("commands: tick N | hours N | feed | pep | camp | bandage | field NAME\n"
           "          pets | feedpet N | playpet N | heel N | release N | sheet | quit\n");
}

static TamiPeople parse_people(const char *s) {
    if (!s) {
        return TAMI_PEOPLE_ORC;
    }
    if (!strcmp(s, "human")) {
        return TAMI_PEOPLE_HUMAN;
    }
    if (!strcmp(s, "elf")) {
        return TAMI_PEOPLE_ELF;
    }
    if (!strcmp(s, "undead")) {
        return TAMI_PEOPLE_UNDEAD;
    }
    return TAMI_PEOPLE_ORC;
}

static TamiCalling parse_call(const char *s) {
    if (!s) {
        return TAMI_CALL_WARRIOR;
    }
    if (!strcmp(s, "ranger")) {
        return TAMI_CALL_RANGER;
    }
    if (!strcmp(s, "mage")) {
        return TAMI_CALL_MAGE;
    }
    if (!strcmp(s, "rogue")) {
        return TAMI_CALL_ROGUE;
    }
    return TAMI_CALL_WARRIOR;
}

static TamiField parse_field(const char *s) {
    for (int i = 0; i < TAMI_FIELD_COUNT; i++) {
        if (!strcasecmp(s, tami_field_name((TamiField)i))) {
            return (TamiField)i;
        }
    }
    return TAMI_FIELD_COUNT;
}

static void apply_hours(TamiAdventurer *a, TamiRng *rng, uint32_t hours) {
    TamiReport r;
    tami_catchup(a, rng, a->last_tick_unix + (int64_t)hours * 3600, &r);
    printf("catch-up %uh: applied %us discarded %us  kills %u  gold+%u  loot %u  sold %u  "
           "quests %u  levels %u%s%s%s%s\n",
           hours, r.applied_sec, r.discarded_sec, r.kills, r.gold_gained, r.items_looted,
           r.items_sold, r.quests_done, r.levels_gained, r.camped ? " camped" : "",
           r.downed ? " downed" : "", r.faded ? " faded" : "",
           r.rolled_back ? " rollback" : "");
}

int main(int argc, char **argv) {
    TamiPeople people = TAMI_PEOPLE_ORC;
    TamiCalling calling = TAMI_CALL_WARRIOR;
    uint32_t hours = 0;
    uint32_t seed = (uint32_t)time(NULL);
    int print_after = 1;
    const char *save_path = NULL;
    const char *load_path = NULL;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--people") && i + 1 < argc) {
            people = parse_people(argv[++i]);
        } else if (!strcmp(argv[i], "--calling") && i + 1 < argc) {
            calling = parse_call(argv[++i]);
        } else if (!strcmp(argv[i], "--hours") && i + 1 < argc) {
            hours = (uint32_t)atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--seed") && i + 1 < argc) {
            seed = (uint32_t)strtoul(argv[++i], NULL, 10);
        } else if (!strcmp(argv[i], "--quiet")) {
            print_after = 0;
        } else if (!strcmp(argv[i], "--save") && i + 1 < argc) {
            save_path = argv[++i];
        } else if (!strcmp(argv[i], "--load") && i + 1 < argc) {
            load_path = argv[++i];
        } else if (!strcmp(argv[i], "--help")) {
            printf("tami-sim [--people orc|human|elf|undead] [--calling warrior|ranger|mage|rogue]\n"
                   "         [--hours N] [--seed N] [--quiet] [--save file.tami] [--load file.tami]\n"
                   "No --hours: interactive desk. Same rules as the device.\n");
            return 0;
        }
    }

    TamiRng rng;
    tami_rng_seed(&rng, seed);
    TamiAdventurer adv;
    tami_hatch(&adv, people, calling, &rng, 1000000);
    if (load_path) {
        if (tami_load(&adv, load_path) != 0) {
            fprintf(stderr, "load failed: %s\n", load_path);
            return 1;
        }
    }

    if (hours) {
        TamiReport last;
        tami_catchup(&adv, &rng, adv.last_tick_unix + (int64_t)hours * 3600, &last);
        char card[256];
        tami_format_card(&adv, &last, card, sizeof(card));
        printf("%s\n", card);
        if (save_path && tami_save(&adv, save_path) != 0) {
            fprintf(stderr, "save failed: %s\n", save_path);
            return 1;
        }
        if (print_after) {
            print_sheet(&adv);
        }
        return adv.faded ? 2 : 0;
    }

    print_sheet(&adv);
    char line[128];
    while (fgets(line, sizeof(line), stdin)) {
        char cmd[32] = {0};
        char arg[64] = {0};
        sscanf(line, "%31s %63s", cmd, arg);
        if (!strcmp(cmd, "quit") || !strcmp(cmd, "q") || !strcmp(cmd, "exit")) {
            if (save_path) {
                tami_save(&adv, save_path);
            }
            break;
        } else if (!strcmp(cmd, "sheet") || !strcmp(cmd, "s") || cmd[0] == 0) {
            print_sheet(&adv);
        } else if (!strcmp(cmd, "log") || !strcmp(cmd, "quests")) {
            char rec[400];
            char log[640];
            tami_format_recap(&adv, rec, sizeof(rec));
            tami_format_log(&adv, log, sizeof(log));
            printf("%s\n%s", rec, log);
        } else if (!strcmp(cmd, "tick")) {
            uint32_t sec = (uint32_t)atoi(arg);
            if (sec == 0) {
                sec = 1;
            }
            TamiReport r;
            tami_catchup(&adv, &rng, adv.last_tick_unix + (int64_t)sec, &r);
            char tbar[128];
            tami_task_bar(&adv, tbar, sizeof(tbar));
            printf("ticked %us (kills+%u gold+%u)  %s\n", r.applied_sec, r.kills, r.gold_gained,
                   tbar);
        } else if (!strcmp(cmd, "hours") || !strcmp(cmd, "h")) {
            uint32_t n = (uint32_t)atoi(arg);
            if (n == 0) {
                n = 1;
            }
            apply_hours(&adv, &rng, n);
            print_sheet(&adv);
        } else if (!strcmp(cmd, "feed")) {
            tami_feed(&adv);
            printf("fed. hunger %u\n", adv.hunger);
        } else if (!strcmp(cmd, "pep")) {
            tami_pep(&adv);
            printf("pep. morale %u\n", adv.morale);
        } else if (!strcmp(cmd, "camp")) {
            tami_force_camp(&adv);
            printf("camped.\n");
        } else if (!strcmp(cmd, "bandage")) {
            tami_bandage(&adv);
            printf("bandaged. wounds %u\n", adv.wounds);
        } else if (!strcmp(cmd, "field")) {
            TamiField f = parse_field(arg);
            if (f == TAMI_FIELD_COUNT) {
                printf("fields: Greenroad Ironpit Moonwood Barrow Ashfen\n");
            } else {
                tami_set_field(&adv, f);
                printf("sent to %s\n", tami_field_name(f));
            }
        } else if (!strcmp(cmd, "pets")) {
            if (adv.pet_count == 0) {
                printf("no pets yet — they find you on the road\n");
            }
            print_sheet(&adv);
        } else if (!strcmp(cmd, "feedpet")) {
            uint8_t i = (uint8_t)atoi(arg);
            tami_pet_feed(&adv, i);
            printf("fed pet %u\n", i);
        } else if (!strcmp(cmd, "playpet")) {
            uint8_t i = (uint8_t)atoi(arg);
            tami_pet_play(&adv, i);
            printf("played with pet %u\n", i);
        } else if (!strcmp(cmd, "heel")) {
            uint8_t i = (uint8_t)atoi(arg);
            if (tami_pet_heel_set(&adv, i) == 0) {
                printf("%s at heel\n", adv.pets[i].name);
            } else {
                printf("no pet %u\n", i);
            }
        } else if (!strcmp(cmd, "release")) {
            uint8_t i = (uint8_t)atoi(arg);
            if (i < adv.pet_count) {
                printf("released %s\n", adv.pets[i].name);
                tami_pet_release(&adv, i);
            }
        } else {
            printf("unknown command. sheet | tick N | hours N | feed | pep | camp | bandage | field NAME\n"
                   "                 pets | feedpet N | playpet N | heel N | release N | quit\n");
        }
    }
    return 0;
}
