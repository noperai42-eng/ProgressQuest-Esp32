#include "names.h"

#include <stdio.h>
#include <string.h>

const char *tami_family_name[] = {
    "gnoll", "boar", "wight", "moth", "pike", "crow", "mire-eel", "ash-rat",
    "wolf", "spider", "ogre", "brigand", "wyrm", "hag",
};
static const char *family_plural[] = {
    "gnolls", "boars", "wights", "moths", "pikes", "crows", "mire-eels", "ash-rats",
    "wolves", "spiders", "ogres", "brigands", "wyrms", "hags",
};
const int tami_family_count = TAMI_FAMILY_COUNT;

const char *tami_mob_plural(uint8_t family) {
    if (family >= TAMI_FAMILY_COUNT) {
        return "?";
    }
    return family_plural[family];
}

static const char *adj[] = {
    "bristled", "pale", "notched", "mire", "ashen", "moonlit", "barrow", "keen",
    "pitted", "hollow", "cinder", "dew",
};
static const int adj_n = 12;

static const char *weapon_base[] = {
    "stick", "shiv", "hatchet", "spear", "longblade", "halberd", "war-pick",
};
static const char *body_base[] = {
    "wraps", "jerkin", "mail", "hauberk", "plate",
};
static const char *head_base[] = {"cap", "coif", "helm"};
static const char *hands_base[] = {"wraps", "gloves", "gauntlets"};
static const char *feet_base[] = {"sandals", "boots", "greaves"};
static const char *shield_base[] = {"buckler", "round-shield", "kite", "tower-shield"};
static const char *brass_base[] = {"brassairts", "chest-guard", "pectoral"};
static const char *vamb_base[] = {"cuffs", "vambraces", "arm-plates"};
static const char *gamb_base[] = {"gambeson", "padding", "aketon"};
static const char *cuisse_base[] = {"cuisses", "thigh-guards", "tassets"};
static const char *soll_base[] = {"shoes", "sollerets", "sabatons"};

static const char *of_word[] = {
    "of cinders", "of the mire", "of dusk", "of iron", "of the barrow",
};
static const int of_n = 5;

static const char *spells[] = {
    "Spark", "Ward", "Mend", "Gloom", "Hasten", "Bind", "Ember", "Still",
    "Rime", "Thorn", "Veil", "Bolt",
};
static const int spell_n = 12;

static const char *human_n[] = {"Ryn", "Mara", "Edda", "Tomas", "Nila", "Bram"};
static const char *orc_n[] = {"Ghra", "Ushk", "Braka", "Mog", "Vesh", "Druun"};
static const char *elf_n[] = {"Sael", "Iri", "Leth", "Aen", "Sila", "Vaer"};
static const char *undead_n[] = {"Ash", "Moth", "Kiln", "Wick", "Ghast", "Noll"};

static void copy_trunc(char *dst, size_t n, const char *s) {
    if (n == 0) {
        return;
    }
    size_t i = 0;
    while (s[i] && i + 1 < n) {
        dst[i] = s[i];
        i++;
    }
    dst[i] = 0;
}

void tami_gen_monster(TamiRng *rng, uint8_t *family, char *label, size_t n) {
    uint8_t f = (uint8_t)tami_rng_below(rng, (uint32_t)tami_family_count);
    *family = f;
    uint32_t count = 1 + tami_rng_below(rng, 5);
    const char *a = adj[tami_rng_below(rng, (uint32_t)adj_n)];
    if (count == 1) {
        snprintf(label, n, "Executing a %s %s", a, tami_family_name[f]);
    } else {
        snprintf(label, n, "Executing %u %s %s", (unsigned)count, a, tami_mob_plural(f));
    }
}

void tami_gen_elite(TamiRng *rng, uint8_t *family, char *label, size_t n) {
    uint8_t f = (uint8_t)tami_rng_below(rng, (uint32_t)tami_family_count);
    *family = (uint8_t)(TAMI_FAMILY_ELITE | f);
    const char *a = adj[tami_rng_below(rng, (uint32_t)adj_n)];
    snprintf(label, n, "Executing an elite %s %s", a, tami_family_name[f]);
}

static uint8_t pick_slot(TamiRng *rng) {
    static const TamiSlot tab[] = {
        TAMI_SLOT_WEAPON,    TAMI_SLOT_WEAPON,    TAMI_SLOT_SHIELD,     TAMI_SLOT_HELM,
        TAMI_SLOT_HELM,      TAMI_SLOT_HAUBERK,   TAMI_SLOT_HAUBERK,    TAMI_SLOT_BRASSAIRTS,
        TAMI_SLOT_VAMBRACES, TAMI_SLOT_GAUNTLETS, TAMI_SLOT_GAMBESON,   TAMI_SLOT_CUISSES,
        TAMI_SLOT_GREAVES,   TAMI_SLOT_SOLLERETS, TAMI_SLOT_SHIELD,     TAMI_SLOT_WEAPON,
    };
    return (uint8_t)tab[tami_rng_below(rng, 16)];
}

static const uint8_t weapon_look[] = {
    TAMI_LOOK_STICK, TAMI_LOOK_SHIV, TAMI_LOOK_HATCHET, TAMI_LOOK_SPEAR,
    TAMI_LOOK_BLADE, TAMI_LOOK_POLE, TAMI_LOOK_PICK,
};
static const uint8_t body_look[] = {
    TAMI_LOOK_CLOTH, TAMI_LOOK_LEATHER, TAMI_LOOK_MAIL, TAMI_LOOK_HAUBERK, TAMI_LOOK_PLATE,
};
static const uint8_t head_look[] = {TAMI_LOOK_CAP, TAMI_LOOK_COIF, TAMI_LOOK_HELM};
static const uint8_t hands_look[] = {TAMI_LOOK_WRAPS, TAMI_LOOK_GLOVES, TAMI_LOOK_GAUNTLETS};
static const uint8_t feet_look[] = {TAMI_LOOK_SANDAL, TAMI_LOOK_BOOTS, TAMI_LOOK_GREAVES};
static const uint8_t shield_look[] = {TAMI_LOOK_SHIELD, TAMI_LOOK_SHIELD, TAMI_LOOK_SHIELD,
                                      TAMI_LOOK_SHIELD_TOWER};
static const uint8_t brass_look[] = {TAMI_LOOK_LEATHER, TAMI_LOOK_MAIL, TAMI_LOOK_BRASSAIRT};
static const uint8_t vamb_look[] = {TAMI_LOOK_LEATHER, TAMI_LOOK_VAMBRACE, TAMI_LOOK_PLATE};
static const uint8_t gamb_look[] = {TAMI_LOOK_GAMBESON, TAMI_LOOK_CLOTH, TAMI_LOOK_LEATHER};
static const uint8_t cuisse_look[] = {TAMI_LOOK_LEATHER, TAMI_LOOK_CUISSE, TAMI_LOOK_PLATE};
static const uint8_t soll_look[] = {TAMI_LOOK_SANDAL, TAMI_LOOK_SOLLERET, TAMI_LOOK_PLATE};

static const char *base_for(uint8_t slot, uint8_t level, TamiRng *rng, uint8_t *look) {
    (void)rng;
    int tier = (int)level / 5;
    if (tier < 0) {
        tier = 0;
    }
    switch (slot) {
    case TAMI_SLOT_WEAPON:
        if (tier > 6) {
            tier = 6;
        }
        *look = weapon_look[tier];
        return weapon_base[tier];
    case TAMI_SLOT_HAUBERK:
        if (tier > 4) {
            tier = 4;
        }
        *look = body_look[tier];
        return body_base[tier];
    case TAMI_SLOT_HELM:
        if (tier > 2) {
            tier = 2;
        }
        *look = head_look[tier];
        return head_base[tier];
    case TAMI_SLOT_GAUNTLETS:
        if (tier > 2) {
            tier = 2;
        }
        *look = hands_look[tier];
        return hands_base[tier];
    case TAMI_SLOT_GREAVES:
        if (tier > 2) {
            tier = 2;
        }
        *look = feet_look[tier];
        return feet_base[tier];
    case TAMI_SLOT_SHIELD:
        if (tier > 3) {
            tier = 3;
        }
        *look = shield_look[tier];
        return shield_base[tier];
    case TAMI_SLOT_BRASSAIRTS:
        if (tier > 2) {
            tier = 2;
        }
        *look = brass_look[tier];
        return brass_base[tier];
    case TAMI_SLOT_VAMBRACES:
        if (tier > 2) {
            tier = 2;
        }
        *look = vamb_look[tier];
        return vamb_base[tier];
    case TAMI_SLOT_GAMBESON:
        if (tier > 2) {
            tier = 2;
        }
        *look = gamb_look[tier];
        return gamb_base[tier];
    case TAMI_SLOT_CUISSES:
        if (tier > 2) {
            tier = 2;
        }
        *look = cuisse_look[tier];
        return cuisse_base[tier];
    case TAMI_SLOT_SOLLERETS:
        if (tier > 2) {
            tier = 2;
        }
        *look = soll_look[tier];
        return soll_base[tier];
    default:
        *look = TAMI_LOOK_LEATHER;
        return "jerkin";
    }
}

static uint8_t roll_rarity(TamiRng *rng, uint8_t level, uint8_t luck) {
    if (luck >= 3) {
        return TAMI_RARITY_LEGENDARY;
    }
    uint32_t roll = tami_rng_below(rng, 1000);
    uint32_t legend_cut = 3u + (uint32_t)luck; /* ~0.3%, elites ~0.5% */
    uint32_t rare_cut = 38u + (uint32_t)level / 2u + (uint32_t)luck * 30u;
    uint32_t un_cut = 180u + (uint32_t)level + (uint32_t)luck * 60u;
    if (roll < legend_cut) {
        return TAMI_RARITY_LEGENDARY;
    }
    if (roll < rare_cut) {
        return TAMI_RARITY_RARE;
    }
    if (roll < un_cut) {
        return TAMI_RARITY_UNCOMMON;
    }
    return TAMI_RARITY_COMMON;
}

void tami_gen_item_slot_luck(TamiRng *rng, uint8_t level, uint8_t slot, uint8_t luck, TamiItem *out) {
    static const uint8_t rarity_pts[TAMI_RARITY_COUNT] = {0, 2, 6, 14};
    memset(out, 0, sizeof(*out));
    out->slot = slot;
    const char *base = base_for(out->slot, level, rng, &out->look);
    uint8_t plus = (uint8_t)tami_rng_below(rng, (uint32_t)(1 + level / 3));
    out->rarity = roll_rarity(rng, level, luck);
    int use_adj = tami_rng_below(rng, 100) < 55 || out->rarity >= TAMI_RARITY_RARE;
    int use_of = tami_rng_below(rng, 100) < 35 || out->rarity >= TAMI_RARITY_LEGENDARY;
    out->of_suffix = (uint8_t)use_of;
    out->qty = 1;
    uint8_t extra = rarity_pts[out->rarity < TAMI_RARITY_COUNT ? out->rarity : 0];
    int score = (int)level + (int)plus + (use_adj ? 2 : 0) + (use_of ? 3 : 0) + (int)extra;
    if (score > 255) {
        score = 255;
    }
    out->score = (uint8_t)score;
    if (use_adj && use_of) {
        snprintf(out->name, TAMI_NAME_MAX, "+%u %s %s %s", plus,
                 adj[tami_rng_below(rng, (uint32_t)adj_n)], base,
                 of_word[tami_rng_below(rng, (uint32_t)of_n)]);
    } else if (use_adj) {
        snprintf(out->name, TAMI_NAME_MAX, "+%u %s %s", plus,
                 adj[tami_rng_below(rng, (uint32_t)adj_n)], base);
    } else if (use_of) {
        snprintf(out->name, TAMI_NAME_MAX, "+%u %s %s", plus, base,
                 of_word[tami_rng_below(rng, (uint32_t)of_n)]);
    } else if (plus) {
        snprintf(out->name, TAMI_NAME_MAX, "+%u %s", plus, base);
    } else {
        copy_trunc(out->name, TAMI_NAME_MAX, base);
    }
}

void tami_gen_item_slot(TamiRng *rng, uint8_t level, uint8_t slot, TamiItem *out) {
    tami_gen_item_slot_luck(rng, level, slot, 0, out);
}

void tami_gen_item_luck(TamiRng *rng, uint8_t level, uint8_t luck, TamiItem *out) {
    tami_gen_item_slot_luck(rng, level, pick_slot(rng), luck, out);
}

void tami_gen_item(TamiRng *rng, uint8_t level, TamiItem *out) {
    tami_gen_item_luck(rng, level, 0, out);
}

void tami_gen_quest(TamiRng *rng, uint8_t level, TamiAdventurer *a) {
    a->quest_type = (TamiQuestType)tami_rng_below(rng, 8);
    a->quest_family = (uint8_t)tami_rng_below(rng, (uint32_t)tami_family_count);
    a->quest_have = 0;
    const char *fam = tami_family_name[a->quest_family];
    const char *many = tami_mob_plural(a->quest_family);
    uint32_t flavor = tami_rng_below(rng, 2);
    switch (a->quest_type) {
    case TAMI_QUEST_EXTERMINATE:
        a->quest_need = (uint8_t)(8 + level + tami_rng_below(rng, 6));
        if (flavor) {
            snprintf(a->quest_label, sizeof(a->quest_label), "Cull %u %s", a->quest_need, many);
        } else {
            snprintf(a->quest_label, sizeof(a->quest_label), "Exterminate %u %s", a->quest_need,
                     many);
        }
        break;
    case TAMI_QUEST_DELIVER:
        a->quest_need = (uint8_t)(8 + level + tami_rng_below(rng, 6));
        if (flavor) {
            snprintf(a->quest_label, sizeof(a->quest_label), "Run a letter through the %s", many);
        } else {
            snprintf(a->quest_label, sizeof(a->quest_label), "Deliver a parcel past the %s", many);
        }
        break;
    case TAMI_QUEST_SEEK:
        a->quest_need = (uint8_t)(8 + level + tami_rng_below(rng, 6));
        if (flavor) {
            snprintf(a->quest_label, sizeof(a->quest_label), "Find the last sign of the %s", fam);
        } else {
            snprintf(a->quest_label, sizeof(a->quest_label), "Seek the lost mark of the %s", fam);
        }
        break;
    case TAMI_QUEST_PLACATE:
        a->quest_need = (uint8_t)(8 + level + tami_rng_below(rng, 6));
        if (flavor) {
            snprintf(a->quest_label, sizeof(a->quest_label), "Quiet the %s-kin", fam);
        } else {
            snprintf(a->quest_label, sizeof(a->quest_label), "Placate the %s-kin", fam);
        }
        break;
    case TAMI_QUEST_RESCUE:
        a->quest_need = (uint8_t)(2 + tami_rng_below(rng, 3));
        if (flavor) {
            snprintf(a->quest_label, sizeof(a->quest_label), "Free a captive of the %s", many);
        } else {
            snprintf(a->quest_label, sizeof(a->quest_label), "Rescue folk from the %s", many);
        }
        break;
    case TAMI_QUEST_CLEAR:
        a->quest_need = (uint8_t)(3 + tami_rng_below(rng, 4));
        if (flavor) {
            snprintf(a->quest_label, sizeof(a->quest_label), "Burn out a nest of %s", many);
        } else {
            snprintf(a->quest_label, sizeof(a->quest_label), "Clear a den of %s", many);
        }
        break;
    case TAMI_QUEST_ESCORT:
        a->quest_need = (uint8_t)(2 + tami_rng_below(rng, 3));
        if (flavor) {
            snprintf(a->quest_label, sizeof(a->quest_label), "Guard a wagon past the %s", many);
        } else {
            snprintf(a->quest_label, sizeof(a->quest_label), "Walk a pilgrim past the %s", many);
        }
        break;
    case TAMI_QUEST_RELIC:
        a->quest_need = (uint8_t)(2 + tami_rng_below(rng, 3));
        if (flavor) {
            snprintf(a->quest_label, sizeof(a->quest_label), "Lift a relic from the %s", many);
        } else {
            snprintf(a->quest_label, sizeof(a->quest_label), "Recover a relic from the %s", many);
        }
        break;
    }
}

void tami_gen_spell(TamiRng *rng, char *out, size_t n) {
    copy_trunc(out, n, spells[tami_rng_below(rng, (uint32_t)spell_n)]);
}

void tami_gen_given_name(TamiRng *rng, TamiPeople p, char *out, size_t n) {
    const char **tab = human_n;
    int m = 6;
    switch (p) {
    case TAMI_PEOPLE_ORC:
        tab = orc_n;
        break;
    case TAMI_PEOPLE_ELF:
        tab = elf_n;
        break;
    case TAMI_PEOPLE_UNDEAD:
        tab = undead_n;
        break;
    default:
        break;
    }
    copy_trunc(out, n, tab[tami_rng_below(rng, (uint32_t)m)]);
}
