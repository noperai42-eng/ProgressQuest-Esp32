#include "tami/sim.h"

#include "names.h"

#include <stdio.h>
#include <string.h>

#define CLAMP_U8(v, lo, hi) ((uint8_t)((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v))))

static const char *people_n[TAMI_PEOPLE_COUNT] = {"Human", "Orc", "Elf", "Undead"};
static const char *calling_n[TAMI_CALL_COUNT] = {"Warrior", "Ranger", "Mage", "Rogue"};
static const char *field_n[TAMI_FIELD_COUNT] = {
    "Greenroad", "Ironpit", "Moonwood", "Barrow", "Ashfen",
};
static const int field_lo[TAMI_FIELD_COUNT] = {1, 4, 8, 12, 16};
static const int field_hi[TAMI_FIELD_COUNT] = {5, 10, 14, 18, 22};
static const char *slot_n[TAMI_SLOT_COUNT] = {
    "weapon",     "shield",    "helm",     "hauberk", "brassairts", "vambraces",
    "gauntlets",  "gambeson",  "cuisses",  "greaves", "sollerets",
};
static const char *plot_n[] = {"Prologue", "Act I", "Act II", "Act III", "Act IV", "Act V"};
static const char *plot_ep[] = {
    "The road still smelled of rain.",
    "Greenroad learned their name.",
    "Ironpit paid in scrap.",
    "The woods went quiet.",
    "The barrow went quiet.",
    "Ashfen kept the cinders.",
};
static const char *task_n[] = {
    "kill", "road to market", "sell", "buy", "road to fields", "camp", "downed",
    "delve", "rescue", "raid",
};
static const char *pet_kind_n[TAMI_PET_KIND_COUNT] = {"toad", "rat", "moth", "crow"};
static const char *pet_trait_n[TAMI_PET_KIND_COUNT] = {
    "forages — hunger slower", "crumbs — extra gold", "night hush — rest slower",
    "omen — extra XP",
};
static const char *pet_given[] = {
    "Pip", "Soot", "Nub", "Kip", "Dew", "Peck", "Cinder", "Midge", "Burr", "Noll",
};
static const int pet_given_n = 10;

const char *tami_people_name(TamiPeople p) {
    return (p < TAMI_PEOPLE_COUNT) ? people_n[p] : "?";
}
const char *tami_calling_name(TamiCalling c) {
    return (c < TAMI_CALL_COUNT) ? calling_n[c] : "?";
}
const char *tami_field_name(TamiField f) {
    return (f < TAMI_FIELD_COUNT) ? field_n[f] : "?";
}
const char *tami_slot_name(TamiSlot s) {
    return (s < TAMI_SLOT_COUNT) ? slot_n[s] : "?";
}
const char *tami_task_kind_name(TamiTaskKind k) {
    if ((unsigned)k > TAMI_TASK_RAID) {
        return "?";
    }
    return task_n[k];
}
int tami_field_band_lo(TamiField f) {
    return (f < TAMI_FIELD_COUNT) ? field_lo[f] : 1;
}
int tami_field_band_hi(TamiField f) {
    return (f < TAMI_FIELD_COUNT) ? field_hi[f] : 1;
}

int tami_encumbrance(const TamiAdventurer *a) {
    int n = 0;
    for (uint8_t i = 0; i < a->inv_count; i++) {
        n += a->inv[i].qty ? (int)a->inv[i].qty : 1;
    }
    return n;
}
int tami_encumbrance_max(const TamiAdventurer *a) {
    return 10 + (int)a->str;
}

uint32_t tami_xp_need(uint8_t level) {
    uint32_t n = 50;
    for (uint8_t i = 1; i < level; i++) {
        n = n + n / 2 + 20;
    }
    return n;
}

int tami_xp_pct(const TamiAdventurer *a) {
    if (!a || a->xp_need == 0) {
        return 0;
    }
    if (a->xp >= a->xp_need) {
        return 100;
    }
    return (int)((a->xp * 100ull) / a->xp_need);
}

static void set_task(TamiAdventurer *a, TamiTaskKind k, uint32_t dur, const char *label) {
    a->task.kind = k;
    a->task.duration_sec = dur ? dur : 1;
    a->task.elapsed_sec = 0;
    snprintf(a->task.label, sizeof(a->task.label), "%s", label);
}

static int inv_full(const TamiAdventurer *a) {
    return a->inv_count >= TAMI_INV_MAX || tami_encumbrance(a) >= tami_encumbrance_max(a);
}

static void push_inv(TamiAdventurer *a, const TamiItem *it) {
    TamiItem add = *it;
    if (add.qty == 0) {
        add.qty = 1;
    }
    for (uint8_t i = 0; i < a->inv_count; i++) {
        if (strcmp(a->inv[i].name, add.name) == 0) {
            uint16_t q = (uint16_t)a->inv[i].qty + add.qty;
            if (q > 99) {
                a->gold += (uint32_t)(q - 99) * 2u;
                q = 99;
            }
            a->inv[i].qty = (uint8_t)q;
            return;
        }
    }
    if (a->inv_count >= TAMI_INV_MAX) {
        a->gold += 2u + it->score;
        return;
    }
    a->inv[a->inv_count++] = add;
}

static uint32_t sell_value(const TamiAdventurer *a, const TamiItem *it) {
    uint32_t v = 3u + (uint32_t)it->score * 2u;
    if (it->of_suffix) {
        v *= 2;
    }
    if (it->rarity > 0) {
        v += v * (uint32_t)it->rarity;
    }
    v += (uint32_t)a->cha / 4u;
    if (a->people == TAMI_PEOPLE_HUMAN) {
        v += v / 10u;
    }
    return v;
}

static uint32_t kill_duration(const TamiAdventurer *a) {
    int lo = tami_field_band_lo(a->field);
    int hi = tami_field_band_hi(a->field);
    int mid = (lo + hi) / 2;
    int overlevel = (int)a->level - mid;
    int dur = 22 + (int)a->level * 3 - (int)a->con / 3 - (int)a->spell_count / 2;
    if (overlevel > 3) {
        dur += 6; /* easy field, still a bar */
    }
    if (overlevel < -3) {
        dur += 8 - overlevel; /* hard field */
    }
    if (a->morale < 30) {
        dur += dur / 2;
    }
    if (a->people == TAMI_PEOPLE_ORC && a->calling == TAMI_CALL_WARRIOR) {
        dur -= 2;
    }
    dur -= (int)tami_worn_rarity(a);
    if (dur < 10) {
        dur = 10;
    }
    if (dur > 120) {
        dur = 120;
    }
    return (uint32_t)dur;
}

int tami_kill_family(const TamiAdventurer *a) {
    if (!a || a->task.family == TAMI_FAMILY_PASSING) {
        return -1;
    }
    return (int)(a->task.family & 0x7Fu);
}

int tami_kill_is_elite(const TamiAdventurer *a) {
    if (!a) {
        return 0;
    }
    return a->task.family != TAMI_FAMILY_PASSING && (a->task.family & TAMI_FAMILY_ELITE) != 0;
}

const char *tami_mob_name(uint8_t family) {
    if (family >= TAMI_FAMILY_COUNT) {
        return "?";
    }
    return tami_family_name[family];
}

int tami_task_is_fight(const TamiAdventurer *a) {
    if (!a) {
        return 0;
    }
    TamiTaskKind k = a->task.kind;
    return k == TAMI_TASK_KILL || k == TAMI_TASK_DUNGEON || k == TAMI_TASK_RESCUE ||
           k == TAMI_TASK_RAID;
}

static void start_kill(TamiAdventurer *a, TamiRng *rng) {
    char label[64];
    uint32_t r = tami_rng_below(rng, 100);
    if (r < 4) {
        a->task.family = TAMI_FAMILY_PASSING;
        snprintf(label, sizeof(label), "Executing a passing %s %s",
                 tami_people_name((TamiPeople)tami_rng_below(rng, TAMI_PEOPLE_COUNT)),
                 tami_calling_name((TamiCalling)tami_rng_below(rng, TAMI_CALL_COUNT)));
        set_task(a, TAMI_TASK_KILL, kill_duration(a) + 6, label);
        return;
    }
    uint32_t elite_p = 3;
    if (a->field == TAMI_FIELD_ASHFEN) {
        elite_p += 2;
    }
    if (a->field == TAMI_FIELD_BARROW && tami_is_night(a->last_tick_unix)) {
        elite_p += 2;
    }
    if (a->plot_act >= 3) {
        elite_p += 2;
    }
    r -= 4;
    if (r < elite_p) {
        tami_gen_elite(rng, &a->task.family, label, sizeof(label));
        set_task(a, TAMI_TASK_KILL, kill_duration(a) + 12, label);
        return;
    }
    r -= elite_p;
    uint8_t f = (uint8_t)tami_rng_below(rng, TAMI_FAMILY_COUNT);
    a->task.family = f;
    const char *fam = tami_mob_name(f);
    const char *pl = tami_mob_plural(f);
    static const char *place[TAMI_FIELD_COUNT][3] = {
        {"a root-hollow", "a greenroad cellar", "a thorn-warren"},
        {"a slag-cut", "an ironpit shaft", "a drowned adit"},
        {"a moonlit den", "a silver-root cave", "a hush-hollow"},
        {"a barrow-cut", "a bone-stair", "a sealed crypt"},
        {"a cinder vault", "an ash-kiln", "a fen-sink"},
    };
    if (r < 5) {
        TamiField fld = a->field < TAMI_FIELD_COUNT ? a->field : TAMI_FIELD_GREENROAD;
        uint32_t pi = tami_rng_below(rng, 3);
        uint32_t vi = tami_rng_below(rng, 3);
        const char *verb = vi == 0 ? "Delving" : (vi == 1 ? "Crawling" : "Opening");
        snprintf(label, sizeof(label), "%s %s", verb, place[fld][pi]);
        set_task(a, TAMI_TASK_DUNGEON, kill_duration(a) * 2u + 18u, label);
        return;
    }
    if (r < 10) {
        uint32_t v = tami_rng_below(rng, 3);
        const char *art = (fam[0] == 'a' || fam[0] == 'e' || fam[0] == 'i' || fam[0] == 'o' ||
                           fam[0] == 'u')
                              ? "an"
                              : "a";
        if (v == 0) {
            snprintf(label, sizeof(label), "Storming %s %s camp", art, fam);
        } else if (v == 1) {
            snprintf(label, sizeof(label), "Burning %s %s palisade", art, fam);
        } else {
            snprintf(label, sizeof(label), "Raiding %s %s den", art, fam);
        }
        set_task(a, TAMI_TASK_RAID, kill_duration(a) + 16u, label);
        return;
    }
    if (r < 15) {
        uint32_t v = tami_rng_below(rng, 3);
        if (v == 0) {
            snprintf(label, sizeof(label), "Saving a traveler from the %s", pl);
        } else if (v == 1) {
            snprintf(label, sizeof(label), "Cutting a merchant free of the %s", pl);
        } else {
            snprintf(label, sizeof(label), "Pulling a child from the %s", pl);
        }
        set_task(a, TAMI_TASK_RESCUE, kill_duration(a) + 10u, label);
        return;
    }
    tami_gen_monster(rng, &a->task.family, label, sizeof(label));
    set_task(a, TAMI_TASK_KILL, kill_duration(a), label);
}

static void start_road_market(TamiAdventurer *a) {
    set_task(a, TAMI_TASK_ROAD_MARKET, 12, "Heading to market to sell loot");
}

static void start_sell(TamiAdventurer *a) {
    if (a->inv_count == 0) {
        set_task(a, TAMI_TASK_ROAD_FIELDS, 12, "Heading to the killing fields");
        return;
    }
    char buf[64];
    const TamiItem *it = &a->inv[a->inv_count - 1];
    uint8_t q = it->qty ? it->qty : 1;
    if (q > 1) {
        snprintf(buf, sizeof(buf), "Selling %s x%u", it->name, q);
    } else {
        snprintf(buf, sizeof(buf), "Selling %s", it->name);
    }
    set_task(a, TAMI_TASK_SELL, 4, buf);
}

static int best_upgrade_slot(const TamiAdventurer *a, uint32_t *cost_out) {
    int best = -1;
    uint32_t best_gain = 0;
    uint32_t best_cost = 0;
    for (int s = 0; s < TAMI_SLOT_COUNT; s++) {
        uint32_t cost = 20u + (uint32_t)a->level * 8u + (uint32_t)a->equip[s].score * 3u;
        cost -= (uint32_t)a->cha / 2u;
        if (cost < 8) {
            cost = 8;
        }
        if (a->gold < cost) {
            continue;
        }
        uint32_t gain = 2u + (uint32_t)a->level / 2u;
        if (gain > best_gain) {
            best_gain = gain;
            best = s;
            best_cost = cost;
        }
    }
    if (cost_out) {
        *cost_out = best_cost;
    }
    return best;
}

static void start_buy_or_fields(TamiAdventurer *a) {
    uint32_t cost = 0;
    if (best_upgrade_slot(a, &cost) >= 0) {
        set_task(a, TAMI_TASK_BUY, 8, "Negotiating better equipment");
        return;
    }
    set_task(a, TAMI_TASK_ROAD_FIELDS, 12, "Heading to the killing fields");
}

static void start_camp(TamiAdventurer *a) {
    set_task(a, TAMI_TASK_CAMP, 20, "Camped — recovering");
}

static void start_downed(TamiAdventurer *a) {
    set_task(a, TAMI_TASK_DOWNED, 90, "Downed — crawling to camp");
}

static void maybe_level(TamiAdventurer *a, TamiRng *rng, TamiReport *rep) {
    while (a->level < 40 && a->xp >= a->xp_need) {
        a->xp -= a->xp_need;
        a->level++;
        a->xp_need = tami_xp_need(a->level);
        a->str = CLAMP_U8(a->str + ((a->calling == TAMI_CALL_WARRIOR) ? 2 : 1), 1, 30);
        a->con = CLAMP_U8(a->con + 1, 1, 30);
        if (a->calling == TAMI_CALL_RANGER) {
            a->dex = CLAMP_U8(a->dex + 2, 1, 30);
        } else {
            a->dex = CLAMP_U8(a->dex + (tami_rng_below(rng, 2) ? 1 : 0), 1, 30);
        }
        if (a->calling == TAMI_CALL_MAGE) {
            a->intel = CLAMP_U8(a->intel + 2, 1, 30);
        }
        if (a->calling == TAMI_CALL_ROGUE) {
            a->dex = CLAMP_U8(a->dex + 1, 1, 30);
            a->cha = CLAMP_U8(a->cha + 1, 1, 30);
        }
        a->wis = CLAMP_U8(a->wis + (tami_rng_below(rng, 3) == 0 ? 1 : 0), 1, 30);
        a->hp_max = (uint16_t)(a->hp_max + a->con / 3u + 1u);
        a->mp_max = (uint16_t)(a->mp_max + a->intel / 3u + 1u);
        if (a->calling == TAMI_CALL_MAGE || tami_rng_below(rng, 3) == 0) {
            char sp[24];
            tami_gen_spell(rng, sp, sizeof(sp));
            tami_learn_spell(a, sp);
        }
        snprintf(a->banner, sizeof(a->banner), "%s is now level %u", a->given_name, a->level);
        a->banner_left = 8;
        if (rep) {
            rep->levels_gained++;
        }
    }
}

static void quest_bump(TamiAdventurer *a, TamiRng *rng, int kill_family, TamiReport *rep) {
    int add = 1;
    if ((a->quest_type == TAMI_QUEST_EXTERMINATE || a->quest_type == TAMI_QUEST_CLEAR) &&
        kill_family == (int)a->quest_family) {
        add = 2;
    }
    if (a->task.kind == TAMI_TASK_RESCUE && a->quest_type == TAMI_QUEST_RESCUE) {
        add += 2;
    }
    if (a->task.kind == TAMI_TASK_DUNGEON && a->quest_type == TAMI_QUEST_RELIC) {
        add += 1;
    }
    if (a->task.kind == TAMI_TASK_RAID && a->quest_type == TAMI_QUEST_CLEAR) {
        add += 1;
    }
    uint8_t have = (uint8_t)(a->quest_have + add);
    if (have >= a->quest_need) {
        a->quest_have = a->quest_need;
        /* reward */
        uint32_t r = tami_rng_below(rng, 4);
        if (r == 0) {
            a->str = CLAMP_U8(a->str + 1, 1, 30);
        } else if (r == 1) {
            char sp[24];
            tami_gen_spell(rng, sp, sizeof(sp));
            tami_learn_spell(a, sp);
        } else {
            TamiItem it;
            tami_gen_item(rng, a->level, &it);
            if (it.rarity < TAMI_RARITY_UNCOMMON) {
                it.rarity = TAMI_RARITY_UNCOMMON;
                it.score = (uint8_t)(it.score + 2);
            }
            if (it.slot < TAMI_SLOT_COUNT && it.score > a->equip[it.slot].score) {
                a->equip[it.slot] = it;
            } else {
                push_inv(a, &it);
            }
        }
        if (rep) {
            rep->quests_done++;
        }
        if (a->quest_log_n < TAMI_QUEST_LOG) {
            snprintf(a->quest_log[a->quest_log_n], 64, "%s", a->quest_label);
            a->quest_log_n++;
        } else {
            memmove(a->quest_log[0], a->quest_log[1], (TAMI_QUEST_LOG - 1) * 64);
            snprintf(a->quest_log[TAMI_QUEST_LOG - 1], 64, "%s", a->quest_label);
        }
        a->plot_progress = CLAMP_U8(a->plot_progress + 5, 0, 100);
        if (a->plot_progress >= 100 && a->plot_act < 5) {
            a->plot_act++;
            a->plot_progress = 0;
        }
        tami_gen_quest(rng, a->level, a);
    } else {
        a->quest_have = have;
    }
}

static const TamiPet *heel_ok(const TamiAdventurer *a) {
    if (a->pet_count == 0 || a->pet_active >= a->pet_count) {
        return NULL;
    }
    const TamiPet *p = &a->pets[a->pet_active];
    if (p->hunger < 20 || p->mood < 20) {
        return NULL;
    }
    return p;
}

static TamiPetKind field_pet_kind(TamiField f) {
    switch (f) {
    case TAMI_FIELD_IRONPIT:
    case TAMI_FIELD_ASHFEN:
        return TAMI_PET_RAT;
    case TAMI_FIELD_MOONWOOD:
        return TAMI_PET_MOTH;
    case TAMI_FIELD_BARROW:
        return TAMI_PET_CROW;
    default:
        return TAMI_PET_TOAD;
    }
}

static void pet_compact(TamiAdventurer *a, uint8_t gone) {
    if (gone >= a->pet_count) {
        return;
    }
    for (uint8_t i = gone; i + 1 < a->pet_count; i++) {
        a->pets[i] = a->pets[i + 1];
    }
    memset(&a->pets[a->pet_count - 1], 0, sizeof(a->pets[0]));
    a->pet_count--;
    if (a->pet_count == 0) {
        a->pet_active = 0xFF;
    } else if (a->pet_active == gone) {
        a->pet_active = 0;
    } else if (a->pet_active != 0xFF && a->pet_active > gone) {
        a->pet_active--;
    }
}

static void apply_pet_needs(TamiAdventurer *a, uint32_t sec, TamiReport *rep) {
    uint8_t i = 0;
    while (i < a->pet_count) {
        TamiPet *p = &a->pets[i];
        int h = (int)p->hunger - (int)(sec / 180u);
        int m = (int)p->mood - (int)(sec / 220u);
        p->hunger = CLAMP_U8(h, 0, 100);
        p->mood = CLAMP_U8(m, 0, 100);
        if (p->hunger == 0) {
            p->starve_sec += sec;
        } else {
            p->starve_sec = 0;
        }
        if (p->starve_sec >= 3u * 3600u) {
            snprintf(a->banner, sizeof(a->banner), "%s the %s wandered off", p->name,
                     tami_pet_kind_name((TamiPetKind)p->kind));
            a->banner_left = 12;
            pet_compact(a, i);
            if (rep) {
                rep->pets_lost++;
            }
            continue;
        }
        i++;
    }
}

static void maybe_find_pet(TamiAdventurer *a, TamiRng *rng, TamiReport *rep) {
    if (a->pet_count >= TAMI_PET_MAX) {
        return;
    }
    uint32_t chance = 4u + (uint32_t)a->cha / 12u;
    if (tami_rng_below(rng, 100) >= chance) {
        return;
    }
    TamiPetKind k = field_pet_kind(a->field);
    if (tami_pet_add(a, rng, k) == 0) {
        const TamiPet *p = &a->pets[a->pet_count - 1];
        snprintf(a->banner, sizeof(a->banner), "a %s took a liking (%s)",
                 tami_pet_kind_name(k), p->name);
        a->banner_left = 10;
        if (rep) {
            rep->pets_found++;
        }
    }
}

static void apply_activity_needs(TamiAdventurer *a, uint32_t sec) {
    /* 1 hunger / 90s, 1 rest / 70s, 1 morale / 180s while active */
    uint32_t h_div = (a->people == TAMI_PEOPLE_ORC) ? 60u : 90u;
    if (a->people == TAMI_PEOPLE_UNDEAD) {
        h_div = 100u; /* ichor */
    }
    uint32_t r_div = (a->people == TAMI_PEOPLE_ELF) ? 110u : 70u;
    const TamiPet *h = heel_ok(a);
    if (h && h->kind == TAMI_PET_TOAD) {
        h_div += h_div / 4u;
    }
    if (h && h->kind == TAMI_PET_MOTH) {
        r_div += r_div / 4u;
    }
    int hunger = (int)a->hunger - (int)(sec / h_div);
    int rest = (int)a->rest - (int)(sec / r_div);
    int morale = (int)a->morale - (int)(sec / 180u);
    if (a->wounds > 50) {
        morale -= (int)(sec / 120u);
    }
    a->hunger = CLAMP_U8(hunger, 0, 100);
    a->rest = CLAMP_U8(rest, 0, 100);
    a->morale = CLAMP_U8(morale, 0, 100);
    if (a->hunger < 20 && a->rest < 20) {
        a->starved_sec += sec;
    } else if (a->hunger >= 50 && a->rest >= 50) {
        a->starved_sec = 0;
    }
}

static int should_camp(const TamiAdventurer *a) {
    return a->hunger == 0 || a->rest == 0;
}

static void do_fade(TamiAdventurer *a, TamiReport *rep) {
    TamiItem best;
    memset(&best, 0, sizeof(best));
    for (int s = 0; s < TAMI_SLOT_COUNT; s++) {
        if (tami_item_on(&a->equip[s]) && a->equip[s].score >= best.score) {
            best = a->equip[s];
        }
    }
    memset(a->equip, 0, sizeof(a->equip));
    memset(a->inv, 0, sizeof(a->inv));
    memset(a->pets, 0, sizeof(a->pets));
    a->inv_count = 0;
    a->pet_count = 0;
    a->pet_active = 0xFF;
    a->gold = 0;
    if (best.name[0]) {
        best.qty = 1;
        a->heirloom = best;
        a->inv[0] = best;
        a->inv_count = 1;
        snprintf(a->banner, sizeof(a->banner), "%.12s faded. %.20s remains.", a->given_name, best.name);
    } else {
        snprintf(a->banner, sizeof(a->banner), "%s faded.", a->given_name);
    }
    a->banner_left = 20;
    a->faded = 1;
    if (rep) {
        rep->faded = 1;
    }
    set_task(a, TAMI_TASK_CAMP, 30, "Faded — a whelp remains");
}

static void complete_task(TamiAdventurer *a, TamiRng *rng, TamiReport *rep);

static void note_loot(TamiAdventurer *a, TamiReport *rep, const TamiItem *it) {
    if (it->rarity >= TAMI_RARITY_RARE) {
        if (a->rares_found < 0xFFFFu) {
            a->rares_found++;
        }
        if (rep && rep->rares_looted < 0xFFFFu) {
            rep->rares_looted++;
        }
    }
    if (it->rarity >= TAMI_RARITY_LEGENDARY) {
        if (a->legendaries_found < 0xFFFFu) {
            a->legendaries_found++;
        }
        if (rep && rep->legendaries_looted < 0xFFFFu) {
            rep->legendaries_looted++;
        }
    }
    if (rep) {
        rep->items_looted++;
        if (it->rarity > rep->best_loot_rarity) {
            rep->best_loot_rarity = it->rarity;
        }
    }
}

static void take_loot(TamiAdventurer *a, TamiRng *rng, TamiReport *rep, uint8_t luck) {
    TamiItem it;
    tami_gen_item_luck(rng, a->level, luck, &it);
    if (a->people == TAMI_PEOPLE_ELF && tami_rng_below(rng, 100) < 8 &&
        it.rarity < TAMI_RARITY_LEGENDARY) {
        it.rarity++;
        it.score = (uint8_t)(it.score + 3);
    }
    note_loot(a, rep, &it);
    if (it.slot < TAMI_SLOT_COUNT && it.score > a->equip[it.slot].score) {
        if (a->equip[it.slot].name[0]) {
            push_inv(a, &a->equip[it.slot]);
        }
        a->equip[it.slot] = it;
    } else {
        push_inv(a, &it);
    }
}

static void after_active(TamiAdventurer *a, TamiRng *rng) {
    if (a->faded) {
        return;
    }
    if (a->wounds >= 100) {
        start_downed(a);
        return;
    }
    if (should_camp(a)) {
        start_camp(a);
        return;
    }
    if (inv_full(a) && a->task.kind != TAMI_TASK_ROAD_MARKET && a->task.kind != TAMI_TASK_SELL) {
        start_road_market(a);
        return;
    }
    start_kill(a, rng);
}

static void complete_task(TamiAdventurer *a, TamiRng *rng, TamiReport *rep) {
    TamiTaskKind k = a->task.kind;
    uint32_t dur = a->task.duration_sec;

    if (tami_task_is_fight(a) || k == TAMI_TASK_ROAD_MARKET || k == TAMI_TASK_SELL ||
        k == TAMI_TASK_BUY || k == TAMI_TASK_ROAD_FIELDS) {
        apply_activity_needs(a, dur);
    }
    apply_pet_needs(a, dur, rep);

    switch (k) {
    case TAMI_TASK_DUNGEON:
    case TAMI_TASK_RESCUE:
    case TAMI_TASK_RAID:
    case TAMI_TASK_KILL: {
        int lo = tami_field_band_lo(a->field);
        int hi = tami_field_band_hi(a->field);
        int mid = (lo + hi) / 2;
        int overlevel = (int)a->level - mid;
        uint32_t bad_chance = 3;
        if (overlevel < -4) {
            bad_chance = 22; /* field too hard */
        }
        if (overlevel > 4) {
            bad_chance = 1; /* field too easy */
        }
        int bad = tami_rng_below(rng, 100) < bad_chance;
        if (bad) {
            int w = (int)a->wounds + 12 - (int)a->con / 4;
            a->wounds = CLAMP_U8(w, 0, 100);
            TamiItem junk;
            tami_gen_item(rng, 1, &junk);
            snprintf(junk.name, TAMI_NAME_MAX, "ruined scrap");
            junk.score = 1;
            junk.slot = 0xFF;
            junk.of_suffix = 0;
            junk.look = TAMI_LOOK_NONE;
            junk.qty = 1;
            junk.rarity = TAMI_RARITY_COMMON;
            push_inv(a, &junk);
            if (rep) {
                rep->items_looted++;
            }
        } else {
            uint32_t xp = 4u + (uint32_t)a->level / 2u + (uint32_t)a->equip[TAMI_SLOT_WEAPON].score / 6u;
            if (a->morale < 30) {
                xp /= 2;
            }
            if (a->people == TAMI_PEOPLE_UNDEAD && a->field == TAMI_FIELD_BARROW) {
                xp += xp / 5u;
            }
            if (tami_is_night(a->last_tick_unix) && a->field == TAMI_FIELD_BARROW) {
                xp += xp / 10u;
            }
            a->xp += xp;
            {
                const TamiPet *h = heel_ok(a);
                if (h && h->kind == TAMI_PET_CROW) {
                    a->xp += 1u + (uint32_t)a->level / 8u;
                }
                if (h && h->kind == TAMI_PET_RAT) {
                    uint32_t crumb = 1u + (uint32_t)a->level / 6u;
                    a->gold += crumb;
                    if (rep) {
                        rep->gold_gained += crumb;
                    }
                }
            }
            a->kills++;
            if (rep) {
                rep->kills++;
            }
            if (tami_kill_is_elite(a)) {
                if (a->elites_slain < 0xFFFFu) {
                    a->elites_slain++;
                }
                if (rep && rep->elites_killed < 0xFFFFu) {
                    rep->elites_killed++;
                }
                a->xp += xp / 2u;
            }
            {
                uint8_t luck = 0;
                if (a->task.family == TAMI_FAMILY_PASSING) {
                    luck = 1;
                }
                if (tami_kill_is_elite(a)) {
                    luck = 2;
                }
                if (k == TAMI_TASK_DUNGEON) {
                    luck = 2;
                    a->xp += 6u + (uint32_t)a->level;
                    if (a->delves < 0xFFFFu) {
                        a->delves++;
                    }
                    snprintf(a->banner, sizeof(a->banner), "The hollow is quiet");
                    a->banner_left = 10;
                } else if (k == TAMI_TASK_RAID) {
                    luck = 2;
                    a->gold += 12u + (uint32_t)a->level * 2u;
                    if (rep) {
                        rep->gold_gained += 12u + (uint32_t)a->level * 2u;
                    }
                    a->wounds = CLAMP_U8((int)a->wounds + 8 - (int)a->con / 5, 0, 100);
                    if (a->camps_stormed < 0xFFFFu) {
                        a->camps_stormed++;
                    }
                    snprintf(a->banner, sizeof(a->banner), "The camp is broken");
                    a->banner_left = 10;
                } else if (k == TAMI_TASK_RESCUE) {
                    luck = 1;
                    a->morale = CLAMP_U8((int)a->morale + 10, 0, 100);
                    if (a->rescues < 0xFFFFu) {
                        a->rescues++;
                    }
                    snprintf(a->banner, sizeof(a->banner), "A traveler lives");
                    a->banner_left = 10;
                }
                take_loot(a, rng, rep, luck);
            }
            quest_bump(a, rng, tami_kill_family(a), rep);
            maybe_level(a, rng, rep);
            maybe_find_pet(a, rng, rep);
        }
        after_active(a, rng);
        break;
    }
    case TAMI_TASK_ROAD_MARKET:
        start_sell(a);
        break;
    case TAMI_TASK_SELL:
        if (a->inv_count > 0) {
            TamiItem *it = &a->inv[a->inv_count - 1];
            uint32_t g = sell_value(a, it);
            a->gold += g;
            uint8_t q = it->qty ? it->qty : 1;
            if (q <= 1) {
                a->inv_count--;
            } else {
                it->qty = (uint8_t)(q - 1);
            }
            if (rep) {
                rep->gold_gained += g;
                rep->items_sold++;
            }
        }
        if (a->inv_count > 0) {
            start_sell(a);
        } else {
            start_buy_or_fields(a);
        }
        break;
    case TAMI_TASK_BUY: {
        uint32_t cost = 0;
        int s = best_upgrade_slot(a, &cost);
        if (s >= 0 && a->gold >= cost) {
            a->gold -= cost;
            TamiItem it;
            tami_gen_item_slot_luck(rng, (uint8_t)(a->level + 1), (uint8_t)s, 1, &it);
            if (it.score <= a->equip[s].score) {
                it.score = (uint8_t)(a->equip[s].score + 2);
            }
            a->equip[s] = it;
        }
        set_task(a, TAMI_TASK_ROAD_FIELDS, 12, "Heading to the killing fields");
        break;
    }
    case TAMI_TASK_ROAD_FIELDS:
        after_active(a, rng);
        break;
    case TAMI_TASK_CAMP: {
        int h = (int)a->hunger + 8;
        int r = (int)a->rest + 10;
        int m = (int)a->morale + 3;
        int w = (int)a->wounds - 2;
        a->hunger = CLAMP_U8(h, 0, 100);
        a->rest = CLAMP_U8(r, 0, 100);
        a->morale = CLAMP_U8(m, 0, 100);
        a->wounds = CLAMP_U8(w, 0, 100);
        if (a->hunger < 20 && a->rest < 20) {
            a->starved_sec += dur;
        }
        if (a->starved_sec >= 6u * 3600u) {
            do_fade(a, rep);
            break;
        }
        if (rep) {
            rep->camped = 1;
        }
        if (a->hunger >= 40 && a->rest >= 40 && !a->faded) {
            start_kill(a, rng);
        } else {
            start_camp(a);
        }
        break;
    }
    case TAMI_TASK_DOWNED: {
        if (rep) {
            rep->downed = 1;
        }
        a->wounds = CLAMP_U8((int)a->wounds - 30, 0, 100);
        start_camp(a);
        break;
    }
    }
}

void tami_hatch(TamiAdventurer *a, TamiPeople p, TamiCalling c, TamiRng *rng, int64_t now) {
    memset(a, 0, sizeof(*a));
    a->version = TAMI_BLOB_VERSION;
    a->people = p;
    a->calling = c;
    snprintf(a->people_name, sizeof(a->people_name), "%s", tami_people_name(p));
    snprintf(a->calling_name, sizeof(a->calling_name), "%s", tami_calling_name(c));
    tami_gen_given_name(rng, p, a->given_name, sizeof(a->given_name));
    a->str = (uint8_t)(8 + tami_rng_below(rng, 7));
    a->con = (uint8_t)(8 + tami_rng_below(rng, 7));
    a->dex = (uint8_t)(8 + tami_rng_below(rng, 7));
    a->intel = (uint8_t)(8 + tami_rng_below(rng, 7));
    a->wis = (uint8_t)(8 + tami_rng_below(rng, 7));
    a->cha = (uint8_t)(8 + tami_rng_below(rng, 7));
    if (c == TAMI_CALL_WARRIOR) {
        a->str = CLAMP_U8(a->str + 2, 1, 30);
    }
    if (c == TAMI_CALL_RANGER) {
        a->dex = CLAMP_U8(a->dex + 2, 1, 30);
    }
    if (c == TAMI_CALL_MAGE) {
        a->intel = CLAMP_U8(a->intel + 2, 1, 30);
    }
    if (c == TAMI_CALL_ROGUE) {
        a->dex = CLAMP_U8(a->dex + 1, 1, 30);
        a->cha = CLAMP_U8(a->cha + 1, 1, 30);
    }
    a->level = 1;
    a->xp = 0;
    a->xp_need = tami_xp_need(1);
    a->hp_max = (uint16_t)(8 + a->con);
    a->mp_max = (uint16_t)(4 + a->intel);
    a->gold = 0;
    a->hunger = 100;
    a->rest = 100;
    a->morale = 80;
    a->wounds = 0;
    a->field = TAMI_FIELD_GREENROAD;
    a->last_tick_unix = now;
    TamiItem stick;
    memset(&stick, 0, sizeof(stick));
    snprintf(stick.name, TAMI_NAME_MAX, "sharp stick");
    stick.score = 1;
    stick.slot = TAMI_SLOT_WEAPON;
    stick.look = TAMI_LOOK_STICK;
    stick.qty = 1;
    a->equip[TAMI_SLOT_WEAPON] = stick;
    tami_gen_quest(rng, 1, a);
    {
        char sp[24];
        tami_gen_spell(rng, sp, sizeof(sp));
        tami_learn_spell(a, sp);
    }
    start_kill(a, rng);
}

int tami_whelp(TamiAdventurer *a, TamiRng *rng, int64_t now) {
    if (!a || !a->faded) {
        return -1;
    }
    a->faded = 0;
    a->starved_sec = 0;
    a->hunger = 80;
    a->rest = 80;
    a->morale = 70;
    a->wounds = 0;
    a->last_tick_unix = now;
    if (tami_item_on(&a->heirloom) && a->heirloom.slot < TAMI_SLOT_COUNT) {
        a->equip[a->heirloom.slot] = a->heirloom;
    }
    if (!tami_item_on(&a->equip[TAMI_SLOT_WEAPON])) {
        TamiItem stick;
        memset(&stick, 0, sizeof(stick));
        snprintf(stick.name, TAMI_NAME_MAX, "sharp stick");
        stick.score = 1;
        stick.slot = TAMI_SLOT_WEAPON;
        stick.look = TAMI_LOOK_STICK;
        stick.qty = 1;
        a->equip[TAMI_SLOT_WEAPON] = stick;
    }
    snprintf(a->banner, sizeof(a->banner), "%s rose with the heirloom", a->given_name);
    a->banner_left = 16;
    if (rng) {
        start_kill(a, rng);
    } else {
        start_camp(a);
    }
    return 0;
}

void tami_catchup(TamiAdventurer *a, TamiRng *rng, int64_t now, TamiReport *out) {
    TamiReport z;
    memset(&z, 0, sizeof(z));
    if (!out) {
        out = &z;
    } else {
        memset(out, 0, sizeof(*out));
    }
    if (a->faded) {
        a->last_tick_unix = now;
        return;
    }
    if (now < a->last_tick_unix) {
        a->last_tick_unix = now;
        out->rolled_back = 1;
        return;
    }
    int64_t raw = now - a->last_tick_unix;
    uint32_t apply = (raw > (int64_t)TAMI_CATCHUP_CAP_SEC) ? TAMI_CATCHUP_CAP_SEC : (uint32_t)raw;
    if (raw > (int64_t)apply) {
        out->discarded_sec = (uint32_t)(raw - (int64_t)apply);
    }
    uint32_t left = apply;
    /* Bound iterations so a bug cannot hang tests or the device loop. */
    for (int i = 0; i < 20000 && left > 0 && !a->faded; i++) {
        if (a->task.duration_sec == 0) {
            a->task.duration_sec = 1;
        }
        if (a->task.elapsed_sec >= a->task.duration_sec) {
            uint32_t el0 = a->task.elapsed_sec;
            complete_task(a, rng, out);
            if (a->task.elapsed_sec >= a->task.duration_sec && a->task.elapsed_sec == el0) {
                after_active(a, rng);
            }
            continue;
        }
        uint32_t remain = a->task.duration_sec - a->task.elapsed_sec;
        if (left < remain) {
            a->task.elapsed_sec += left;
            if (tami_task_is_fight(a) || a->task.kind == TAMI_TASK_ROAD_MARKET ||
                a->task.kind == TAMI_TASK_SELL || a->task.kind == TAMI_TASK_BUY ||
                a->task.kind == TAMI_TASK_ROAD_FIELDS) {
                apply_activity_needs(a, left);
            }
            apply_pet_needs(a, left, out);
            left = 0;
            break;
        }
        a->task.elapsed_sec = a->task.duration_sec;
        left -= remain;
        complete_task(a, rng, out);
    }
    out->applied_sec = apply - left;
    if (a->banner_left) {
        if (a->banner_left > out->applied_sec) {
            a->banner_left = (uint16_t)(a->banner_left - out->applied_sec);
        } else {
            a->banner_left = 0;
        }
    }
    a->last_tick_unix = now;
    if (!a->faded && a->starved_sec >= 6u * 3600u) {
        do_fade(a, out);
    }
}

void tami_feed(TamiAdventurer *a) {
    if (a->faded) {
        return;
    }
    a->hunger = CLAMP_U8((int)a->hunger + 40, 0, 100);
    a->starved_sec = 0;
}

void tami_pep(TamiAdventurer *a) {
    if (a->faded) {
        return;
    }
    a->morale = CLAMP_U8((int)a->morale + 20, 0, 100);
}

void tami_bandage(TamiAdventurer *a) {
    if (a->faded) {
        return;
    }
    a->wounds = CLAMP_U8((int)a->wounds - 40, 0, 100);
    if (a->task.kind == TAMI_TASK_DOWNED && a->wounds < 80) {
        start_camp(a);
    }
}

void tami_force_camp(TamiAdventurer *a) {
    if (a->faded) {
        return;
    }
    start_camp(a);
}

int tami_set_field(TamiAdventurer *a, TamiField f) {
    if (a->faded || f >= TAMI_FIELD_COUNT) {
        return -1;
    }
    a->field = f;
    return 0;
}

const char *tami_pet_kind_name(TamiPetKind k) {
    return (k < TAMI_PET_KIND_COUNT) ? pet_kind_n[k] : "?";
}

const char *tami_pet_trait(TamiPetKind k) {
    return (k < TAMI_PET_KIND_COUNT) ? pet_trait_n[k] : "";
}

const TamiPet *tami_pet_heel(const TamiAdventurer *a) {
    return heel_ok(a);
}

const TamiPet *tami_pet_beside(const TamiAdventurer *a) {
    if (!a || a->faded || a->pet_count == 0 || a->pet_active >= a->pet_count) {
        return NULL;
    }
    return &a->pets[a->pet_active];
}

int tami_pet_add(TamiAdventurer *a, TamiRng *rng, TamiPetKind k) {
    if (!a || a->faded || k >= TAMI_PET_KIND_COUNT || a->pet_count >= TAMI_PET_MAX) {
        return -1;
    }
    TamiPet *p = &a->pets[a->pet_count];
    memset(p, 0, sizeof(*p));
    p->kind = (uint8_t)k;
    p->hunger = 80;
    p->mood = 70;
    snprintf(p->name, sizeof(p->name), "%s", pet_given[tami_rng_below(rng, (uint32_t)pet_given_n)]);
    if (a->pet_count == 0) {
        a->pet_active = 0;
    }
    a->pet_count++;
    return 0;
}

void tami_pet_feed(TamiAdventurer *a, uint8_t i) {
    if (!a || a->faded || i >= a->pet_count) {
        return;
    }
    a->pets[i].hunger = CLAMP_U8((int)a->pets[i].hunger + 40, 0, 100);
    a->pets[i].starve_sec = 0;
}

void tami_pet_play(TamiAdventurer *a, uint8_t i) {
    if (!a || a->faded || i >= a->pet_count) {
        return;
    }
    a->pets[i].mood = CLAMP_U8((int)a->pets[i].mood + 25, 0, 100);
}

int tami_pet_heel_set(TamiAdventurer *a, uint8_t i) {
    if (!a || a->faded || i >= a->pet_count) {
        return -1;
    }
    a->pet_active = i;
    return 0;
}

int tami_pet_release(TamiAdventurer *a, uint8_t i) {
    if (!a || a->faded || i >= a->pet_count) {
        return -1;
    }
    pet_compact(a, i);
    return 0;
}

int tami_task_pct(const TamiAdventurer *a) {
    if (a->task.duration_sec == 0) {
        return 0;
    }
    return (int)((a->task.elapsed_sec * 100u) / a->task.duration_sec);
}

int tami_item_on(const TamiItem *it) {
    return it && it->name[0] && it->look != TAMI_LOOK_NONE;
}

const char *tami_rarity_name(uint8_t r) {
    static const char *n[TAMI_RARITY_COUNT] = {"common", "uncommon", "rare", "legendary"};
    return r < TAMI_RARITY_COUNT ? n[r] : n[0];
}

const char *tami_rarity_mark(uint8_t r) {
    if (r >= TAMI_RARITY_LEGENDARY) {
        return "**";
    }
    if (r >= TAMI_RARITY_RARE) {
        return "*";
    }
    if (r >= TAMI_RARITY_UNCOMMON) {
        return "+";
    }
    return "";
}

void tami_rarity_rgb(uint8_t r, uint8_t *R, uint8_t *G, uint8_t *B) {
    uint8_t rr = 210, rg = 200, rb = 180;
    if (r >= TAMI_RARITY_LEGENDARY) {
        rr = 255;
        rg = 210;
        rb = 90;
    } else if (r >= TAMI_RARITY_RARE) {
        rr = 130;
        rg = 180;
        rb = 255;
    } else if (r >= TAMI_RARITY_UNCOMMON) {
        rr = 140;
        rg = 210;
        rb = 150;
    }
    if (R) {
        *R = rr;
    }
    if (G) {
        *G = rg;
    }
    if (B) {
        *B = rb;
    }
}

uint8_t tami_worn_rarity(const TamiAdventurer *a) {
    uint8_t best = 0;
    if (!a) {
        return 0;
    }
    for (int s = 0; s < TAMI_SLOT_COUNT; s++) {
        if (tami_item_on(&a->equip[s]) && a->equip[s].rarity > best) {
            best = a->equip[s].rarity;
        }
    }
    return best;
}

const TamiItem *tami_best_gleam(const TamiAdventurer *a) {
    const TamiItem *best = NULL;
    if (!a) {
        return NULL;
    }
    for (int s = 0; s < TAMI_SLOT_COUNT; s++) {
        const TamiItem *it = &a->equip[s];
        if (!tami_item_on(it) || it->rarity < TAMI_RARITY_RARE) {
            continue;
        }
        if (!best || it->rarity > best->rarity ||
            (it->rarity == best->rarity && it->score > best->score)) {
            best = it;
        }
    }
    return best;
}

void tami_learn_spell(TamiAdventurer *a, const char *name) {
    if (!name || !name[0]) {
        return;
    }
    for (uint8_t i = 0; i < a->spell_count; i++) {
        if (strcmp(a->spells[i].name, name) == 0) {
            if (a->spells[i].rank < 99) {
                a->spells[i].rank++;
            }
            return;
        }
    }
    if (a->spell_count >= TAMI_SPELL_MAX) {
        return;
    }
    snprintf(a->spells[a->spell_count].name, sizeof(a->spells[0].name), "%s", name);
    a->spells[a->spell_count].rank = 1;
    a->spell_count++;
}

const char *tami_plot_name(uint8_t act) {
    if (act > 5) {
        act = 5;
    }
    return plot_n[act];
}

const char *tami_plot_epitaph(uint8_t act) {
    if (act > 5) {
        act = 5;
    }
    return plot_ep[act];
}

int tami_blob_sane(const TamiAdventurer *a) {
    if (!a || a->version != TAMI_BLOB_VERSION) {
        return 0;
    }
    if (a->people >= TAMI_PEOPLE_COUNT || a->calling >= TAMI_CALL_COUNT) {
        return 0;
    }
    if (a->field >= TAMI_FIELD_COUNT) {
        return 0;
    }
    if (a->level < 1 || a->level > 40) {
        return 0;
    }
    if (a->inv_count > TAMI_INV_MAX || a->spell_count > TAMI_SPELL_MAX) {
        return 0;
    }
    if (a->quest_log_n > TAMI_QUEST_LOG || a->pet_count > TAMI_PET_MAX) {
        return 0;
    }
    if (a->given_name[0] == 0) {
        return 0;
    }
    return 1;
}

int tami_save(const TamiAdventurer *a, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        return -1;
    }
    size_t n = fwrite(a, sizeof(*a), 1, f);
    fclose(f);
    return n == 1 ? 0 : -1;
}

int tami_load(TamiAdventurer *a, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return -1;
    }
    TamiAdventurer tmp;
    size_t n = fread(&tmp, sizeof(tmp), 1, f);
    fclose(f);
    if (n != 1 || !tami_blob_sane(&tmp)) {
        return -1;
    }
    *a = tmp;
    return 0;
}

void tami_add_item(TamiAdventurer *a, const TamiItem *it) {
    push_inv(a, it);
}

void tami_give_relic(TamiAdventurer *a, TamiRng *rng) {
    TamiItem it;
    if (!a || a->faded) {
        return;
    }
    tami_gen_item_slot_luck(rng, a->level < 8 ? 12 : a->level, TAMI_SLOT_HELM, 3, &it);
    if (a->equip[TAMI_SLOT_HELM].name[0]) {
        push_inv(a, &a->equip[TAMI_SLOT_HELM]);
    }
    a->equip[TAMI_SLOT_HELM] = it;
    note_loot(a, NULL, &it);
}

int tami_hour(int64_t unix_ts) {
    int64_t s = unix_ts % 86400;
    if (s < 0) {
        s += 86400;
    }
    return (int)(s / 3600);
}

int tami_is_night(int64_t unix_ts) {
    int h = tami_hour(unix_ts);
    return h < 6 || h >= 20;
}

void tami_format_card(const TamiAdventurer *a, const TamiReport *r, char *buf, size_t n) {
    unsigned h = r->applied_sec / 3600u;
    unsigned m = (r->applied_sec % 3600u) / 60u;
    const char *note = "";
    if (r->faded || a->faded) {
        note = ", faded";
    } else if (r->downed) {
        note = ", downed";
    } else if (r->camped) {
        note = ", camped";
    } else if (a->hunger < 40) {
        note = ", hungry";
    } else if (a->rest < 40) {
        note = ", weary";
    }
    snprintf(buf, n, "%uh %um\n%u kills   +%ug\n%u loot   %u sold\n%u quests   +%u lv%s", h, m,
             (unsigned)r->kills, (unsigned)r->gold_gained, (unsigned)r->items_looted,
             (unsigned)r->items_sold, (unsigned)r->quests_done, (unsigned)r->levels_gained, note);
}

void tami_format_status(const TamiAdventurer *a, char *buf, size_t n) {
    if (!buf || n == 0) {
        return;
    }
    if (!a) {
        buf[0] = 0;
        return;
    }
    const char *task = (a->banner_left && a->banner[0]) ? a->banner : a->task.label;
    snprintf(buf, n, "%s  lv %u %d%%  %s  |  %s %d%%  |  h%u r%u w%u  |  %s", a->given_name,
             (unsigned)a->level, tami_xp_pct(a), tami_field_name(a->field), task, tami_task_pct(a),
             (unsigned)a->hunger, (unsigned)a->rest, (unsigned)a->wounds,
             tami_task_kind_name(a->task.kind));
}

void tami_format_story(const TamiAdventurer *a, char *buf, size_t n) {
    if (!buf || n == 0) {
        return;
    }
    if (!a) {
        buf[0] = 0;
        return;
    }
    const char *pet = "-";
    const TamiPet *h = tami_pet_heel(a);
    if (h) {
        pet = tami_pet_kind_name((TamiPetKind)h->kind);
    } else if (a->pet_count > 0) {
        pet = tami_pet_kind_name((TamiPetKind)a->pets[0].kind);
    }
    snprintf(buf, n,
             "STORY\t%s\t%s\t%s\tlv=%u\txp=%d\t%s\tplot=%s\tpct=%u\tkills=%u\tgold=%u\tpets=%u\t"
             "pet=%s\tquest=%s\ttask=%s\tepitaph=%s\tfaded=%u\tspells=%u\trares=%u\tlegend=%u\t"
             "elites=%u\tgleam=%s\tdelves=%u\trescues=%u\tcamps=%u",
             a->given_name, a->people_name, a->calling_name, (unsigned)a->level, tami_xp_pct(a),
             tami_field_name(a->field), tami_plot_name(a->plot_act), (unsigned)a->plot_progress,
             (unsigned)a->kills, (unsigned)a->gold, (unsigned)a->pet_count, pet, a->quest_label,
             a->task.label, tami_plot_epitaph(a->plot_act), (unsigned)a->faded,
             (unsigned)a->spell_count, (unsigned)a->rares_found, (unsigned)a->legendaries_found,
             (unsigned)a->elites_slain, tami_rarity_name(tami_worn_rarity(a)), (unsigned)a->delves,
             (unsigned)a->rescues, (unsigned)a->camps_stormed);
}

void tami_format_recap(const TamiAdventurer *a, char *buf, size_t n) {
    if (!buf || n == 0) {
        return;
    }
    if (!a) {
        buf[0] = 0;
        return;
    }
    const char *pet = "";
    const TamiPet *heel = tami_pet_heel(a);
    if (heel) {
        pet = tami_pet_kind_name((TamiPetKind)heel->kind);
    } else if (a->pet_count > 0) {
        pet = tami_pet_kind_name((TamiPetKind)a->pets[0].kind);
    }
    if (a->faded) {
        snprintf(buf, n, "%s the %s %s — %s. %s Now: %s", a->given_name, a->people_name,
                 a->calling_name, tami_plot_name(a->plot_act), tami_plot_epitaph(a->plot_act),
                 a->task.label[0] ? a->task.label : "Faded");
        return;
    }
    if (a->kills == 0 && pet[0] == 0) {
        snprintf(buf, n, "%s the %s %s — %s. %s Quest: %s Now: %s", a->given_name, a->people_name,
                 a->calling_name, tami_plot_name(a->plot_act), tami_plot_epitaph(a->plot_act),
                 a->quest_label[0] ? a->quest_label : "(none)",
                 a->task.label[0] ? a->task.label : "(idle)");
        return;
    }
    {
        char gleam[80] = "";
        const TamiItem *rel = tami_best_gleam(a);
        if (rel) {
            snprintf(gleam, sizeof(gleam), " Relic: %s %s.", tami_rarity_name(rel->rarity), rel->name);
        } else if (a->legendaries_found) {
            snprintf(gleam, sizeof(gleam), " %u relic%s found.", (unsigned)a->legendaries_found,
                     a->legendaries_found == 1 ? "" : "s");
        }
        if (pet[0]) {
            snprintf(buf, n,
                     "%s the %s %s — %s. %s After %u fights, a %s at heel.%s Quest: %s Now: %s",
                     a->given_name, a->people_name, a->calling_name, tami_plot_name(a->plot_act),
                     tami_plot_epitaph(a->plot_act), (unsigned)a->kills, pet, gleam,
                     a->quest_label[0] ? a->quest_label : "(none)",
                     a->task.label[0] ? a->task.label : "(idle)");
        } else {
            snprintf(buf, n, "%s the %s %s — %s. %s After %u fights.%s Quest: %s Now: %s",
                     a->given_name, a->people_name, a->calling_name, tami_plot_name(a->plot_act),
                     tami_plot_epitaph(a->plot_act), (unsigned)a->kills, gleam,
                     a->quest_label[0] ? a->quest_label : "(none)",
                     a->task.label[0] ? a->task.label : "(idle)");
        }
    }
}

void tami_format_chronicle(const TamiAdventurer *a, char *buf, size_t n) {
    if (!buf || n == 0) {
        return;
    }
    if (!a) {
        buf[0] = 0;
        return;
    }
    const char *task = (a->banner_left && a->banner[0]) ? a->banner
                       : (a->task.label[0] ? a->task.label : "(idle)");
    char company[48] = "alone on the road";
    const TamiPet *heel = tami_pet_heel(a);
    if (heel) {
        snprintf(company, sizeof(company), "%s the %s at heel", heel->name,
                 tami_pet_kind_name((TamiPetKind)heel->kind));
    } else if (a->pet_count > 0) {
        snprintf(company, sizeof(company), "a %s in the pack",
                 tami_pet_kind_name((TamiPetKind)a->pets[0].kind));
    }
    char relic[80] = "";
    const TamiItem *rel = tami_best_gleam(a);
    if (rel) {
        snprintf(relic, sizeof(relic), "Relic: %s %s.\n", tami_rarity_name(rel->rarity), rel->name);
    }
    if (a->faded) {
        snprintf(buf, n, "%s the %s %s\n%s. %s\n\nFaded. A whelp remains.", a->given_name,
                 a->people_name, a->calling_name, tami_plot_name(a->plot_act),
                 tami_plot_epitaph(a->plot_act));
        return;
    }
    snprintf(buf, n,
             "%s the %s %s\n%s. %s\n\nAfter %u fights on %s.\n%u delves, %u rescues, %u camps "
             "stormed.\n%u rares, %u relics, %u elites.\n%s.\n%s\nNow: %s\nQuest: %s",
             a->given_name, a->people_name, a->calling_name, tami_plot_name(a->plot_act),
             tami_plot_epitaph(a->plot_act), (unsigned)a->kills, tami_field_name(a->field),
             (unsigned)a->delves, (unsigned)a->rescues, (unsigned)a->camps_stormed,
             (unsigned)a->rares_found, (unsigned)a->legendaries_found, (unsigned)a->elites_slain,
             company, relic, task, a->quest_label[0] ? a->quest_label : "(none)");
}

void tami_format_log(const TamiAdventurer *a, char *buf, size_t n) {
    if (!buf || n == 0) {
        return;
    }
    if (!a) {
        buf[0] = 0;
        return;
    }
    size_t used = 0;
    int w = snprintf(buf, n, "now: %s\n", a->quest_label[0] ? a->quest_label : "(none)");
    if (w > 0) {
        used = (size_t)w;
    }
    if (used >= n) {
        return;
    }
    if (a->quest_log_n == 0) {
        snprintf(buf + used, n - used, "(no finished quests yet)");
        return;
    }
    w = snprintf(buf + used, n - used, "-- done --\n");
    if (w > 0) {
        used += (size_t)w;
    }
    for (int i = (int)a->quest_log_n - 1; i >= 0 && used + 8 < n; i--) {
        w = snprintf(buf + used, n - used, "%s\n", a->quest_log[i]);
        if (w > 0) {
            used += (size_t)w;
        }
    }
}

void tami_task_bar(const TamiAdventurer *a, char *buf, size_t n) {
    int pct = tami_task_pct(a);
    if (pct > 100) {
        pct = 100;
    }
    int filled = pct / 5;
    char bar[21];
    for (int i = 0; i < 20; i++) {
        bar[i] = (i < filled) ? '#' : '-';
    }
    bar[20] = 0;
    snprintf(buf, n, "[%s] %3d%%  %s", bar, pct, a->task.label);
}
