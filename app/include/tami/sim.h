#ifndef TAMI_SIM_H
#define TAMI_SIM_H

#include <stddef.h>
#include <stdint.h>

#define TAMI_NAME_MAX 48
#define TAMI_INV_MAX 40
#define TAMI_SPELL_MAX 16
#define TAMI_QUEST_LOG 12
#define TAMI_PET_MAX 3
#define TAMI_PET_NAME 16
#define TAMI_CATCHUP_CAP_SEC (12 * 3600)
#define TAMI_BLOB_VERSION 6
#define TAMI_FAMILY_PASSING 0xFF
#define TAMI_FAMILY_ELITE 0x80
#define TAMI_FAMILY_COUNT 14

typedef struct {
    uint32_t s;
} TamiRng;

void tami_rng_seed(TamiRng *r, uint32_t seed);
uint32_t tami_rng_u32(TamiRng *r);
uint32_t tami_rng_below(TamiRng *r, uint32_t n);

typedef enum {
    TAMI_PEOPLE_HUMAN = 0,
    TAMI_PEOPLE_ORC,
    TAMI_PEOPLE_ELF,
    TAMI_PEOPLE_UNDEAD,
    TAMI_PEOPLE_COUNT
} TamiPeople;

typedef enum {
    TAMI_CALL_WARRIOR = 0,
    TAMI_CALL_RANGER,
    TAMI_CALL_MAGE,
    TAMI_CALL_ROGUE,
    TAMI_CALL_COUNT
} TamiCalling;

typedef enum {
    TAMI_FIELD_GREENROAD = 0,
    TAMI_FIELD_IRONPIT,
    TAMI_FIELD_MOONWOOD,
    TAMI_FIELD_BARROW,
    TAMI_FIELD_ASHFEN,
    TAMI_FIELD_COUNT
} TamiField;

typedef enum {
    TAMI_TASK_KILL = 0,
    TAMI_TASK_ROAD_MARKET,
    TAMI_TASK_SELL,
    TAMI_TASK_BUY,
    TAMI_TASK_ROAD_FIELDS,
    TAMI_TASK_CAMP,
    TAMI_TASK_DOWNED,
    TAMI_TASK_DUNGEON,
    TAMI_TASK_RESCUE,
    TAMI_TASK_RAID
} TamiTaskKind;

typedef enum {
    TAMI_SLOT_WEAPON = 0,
    TAMI_SLOT_SHIELD,
    TAMI_SLOT_HELM,
    TAMI_SLOT_HAUBERK,
    TAMI_SLOT_BRASSAIRTS,
    TAMI_SLOT_VAMBRACES,
    TAMI_SLOT_GAUNTLETS,
    TAMI_SLOT_GAMBESON,
    TAMI_SLOT_CUISSES,
    TAMI_SLOT_GREAVES,
    TAMI_SLOT_SOLLERETS,
    TAMI_SLOT_COUNT
} TamiSlot;

typedef enum {
    TAMI_QUEST_EXTERMINATE = 0,
    TAMI_QUEST_DELIVER,
    TAMI_QUEST_SEEK,
    TAMI_QUEST_PLACATE,
    TAMI_QUEST_RESCUE,
    TAMI_QUEST_CLEAR,
    TAMI_QUEST_ESCORT,
    TAMI_QUEST_RELIC
} TamiQuestType;

typedef enum {
    TAMI_RARITY_COMMON = 0,
    TAMI_RARITY_UNCOMMON,
    TAMI_RARITY_RARE,
    TAMI_RARITY_LEGENDARY,
    TAMI_RARITY_COUNT
} TamiRarity;

typedef enum {
    TAMI_PET_TOAD = 0,
    TAMI_PET_RAT,
    TAMI_PET_MOTH,
    TAMI_PET_CROW,
    TAMI_PET_KIND_COUNT
} TamiPetKind;

typedef struct {
    uint8_t kind;
    uint8_t hunger;
    uint8_t mood;
    uint8_t pad;
    uint32_t starve_sec;
    char name[TAMI_PET_NAME];
} TamiPet;

/* Paper-doll look codes. 0 = empty / junk. Kept on the item so the figure
 * changes when gear is equipped or stripped. */
enum {
    TAMI_LOOK_NONE = 0,
    TAMI_LOOK_STICK = 1,
    TAMI_LOOK_SHIV,
    TAMI_LOOK_HATCHET,
    TAMI_LOOK_SPEAR,
    TAMI_LOOK_BLADE,
    TAMI_LOOK_POLE,
    TAMI_LOOK_PICK,
    TAMI_LOOK_CLOTH = 10,
    TAMI_LOOK_LEATHER,
    TAMI_LOOK_MAIL,
    TAMI_LOOK_HAUBERK,
    TAMI_LOOK_PLATE,
    TAMI_LOOK_CAP = 20,
    TAMI_LOOK_COIF,
    TAMI_LOOK_HELM,
    TAMI_LOOK_WRAPS = 25,
    TAMI_LOOK_GLOVES,
    TAMI_LOOK_GAUNTLETS,
    TAMI_LOOK_SANDAL = 28,
    TAMI_LOOK_BOOTS,
    TAMI_LOOK_GREAVES,
    TAMI_LOOK_BEAD = 30,
    TAMI_LOOK_SIGNET,
    TAMI_LOOK_IDOL,
    TAMI_LOOK_CHARM,
    TAMI_LOOK_SHIELD = 40,
    TAMI_LOOK_SHIELD_TOWER,
    TAMI_LOOK_VAMBRACE,
    TAMI_LOOK_CUISSE,
    TAMI_LOOK_SOLLERET,
    TAMI_LOOK_GAMBESON,
    TAMI_LOOK_BRASSAIRT
};

typedef struct {
    char name[TAMI_NAME_MAX];
    uint8_t score;
    uint8_t slot;      /* TAMI_SLOT_* or 0xFF junk */
    uint8_t of_suffix; /* 1 = "of …" sell bonus */
    uint8_t look;      /* TAMI_LOOK_* paper doll */
    uint8_t qty;       /* stack size, 0 means 1 */
    uint8_t rarity;    /* TamiRarity */
} TamiItem;

typedef struct {
    char name[24];
    uint8_t rank;
} TamiSpell;

typedef struct {
    TamiTaskKind kind;
    uint32_t duration_sec;
    uint32_t elapsed_sec;
    uint8_t family;
    char label[64];
} TamiTask;

typedef struct {
    uint16_t version;
    TamiPeople people;
    TamiCalling calling;
    uint8_t str, con, dex, intel, wis, cha;
    uint16_t hp_max, mp_max;
    uint8_t level;
    uint32_t xp, xp_need;
    uint32_t gold;
    uint8_t hunger, rest, morale, wounds;
    TamiField field;
    int64_t last_tick_unix;
    TamiTask task;
    TamiItem equip[TAMI_SLOT_COUNT];
    TamiItem inv[TAMI_INV_MAX];
    uint8_t inv_count;
    TamiSpell spells[TAMI_SPELL_MAX];
    uint8_t spell_count;
    TamiQuestType quest_type;
    uint8_t quest_need, quest_have, quest_family;
    char quest_label[64];
    char quest_log[TAMI_QUEST_LOG][64];
    uint8_t quest_log_n;
    uint8_t plot_act;      /* 0 = Prologue … 5 = Act V */
    uint8_t plot_progress; /* 0..100 toward next act */
    char banner[64];
    uint16_t banner_left;
    uint32_t kills;
    uint32_t starved_sec;
    uint8_t faded;
    TamiItem heirloom;
    TamiPet pets[TAMI_PET_MAX];
    uint8_t pet_count;
    uint8_t pet_active; /* index, or 0xFF none */
    uint16_t rares_found;
    uint16_t legendaries_found;
    uint16_t elites_slain;
    uint16_t delves;
    uint16_t rescues;
    uint16_t camps_stormed;
    char people_name[16];
    char calling_name[16];
    char given_name[24];
} TamiAdventurer;

typedef struct {
    uint32_t applied_sec;
    uint32_t discarded_sec;
    uint32_t kills;
    uint32_t gold_gained;
    uint32_t items_looted;
    uint32_t items_sold;
    uint32_t quests_done;
    uint32_t levels_gained;
    uint8_t camped;
    uint8_t downed;
    uint8_t faded;
    uint8_t rolled_back;
    uint8_t pets_found;
    uint8_t pets_lost;
    uint16_t rares_looted;
    uint16_t legendaries_looted;
    uint16_t elites_killed;
    uint8_t best_loot_rarity;
} TamiReport;

const char *tami_people_name(TamiPeople p);
const char *tami_calling_name(TamiCalling c);
const char *tami_field_name(TamiField f);
const char *tami_mob_name(uint8_t family);
const char *tami_mob_plural(uint8_t family);
int tami_kill_family(const TamiAdventurer *a); /* -1 passing, else 0..FAMILY_COUNT-1 */
int tami_kill_is_elite(const TamiAdventurer *a);
int tami_task_is_fight(const TamiAdventurer *a);
const char *tami_slot_name(TamiSlot s);
const char *tami_task_kind_name(TamiTaskKind k);

int tami_field_band_lo(TamiField f);
int tami_field_band_hi(TamiField f);
int tami_encumbrance(const TamiAdventurer *a);
int tami_encumbrance_max(const TamiAdventurer *a);
uint32_t tami_xp_need(uint8_t level);
int tami_xp_pct(const TamiAdventurer *a);

void tami_hatch(TamiAdventurer *a, TamiPeople p, TamiCalling c, TamiRng *rng, int64_t now);
int tami_whelp(TamiAdventurer *a, TamiRng *rng, int64_t now); /* 0 ok, -1 not faded */
void tami_catchup(TamiAdventurer *a, TamiRng *rng, int64_t now, TamiReport *out);
void tami_feed(TamiAdventurer *a);
void tami_pep(TamiAdventurer *a);
void tami_bandage(TamiAdventurer *a);
void tami_force_camp(TamiAdventurer *a);
int tami_set_field(TamiAdventurer *a, TamiField f); /* 0 ok, -1 faded */

const char *tami_pet_kind_name(TamiPetKind k);
const char *tami_pet_trait(TamiPetKind k);
const TamiPet *tami_pet_heel(const TamiAdventurer *a);
const TamiPet *tami_pet_beside(const TamiAdventurer *a); /* active pet, even if hungry */
int tami_pet_add(TamiAdventurer *a, TamiRng *rng, TamiPetKind k); /* 0 ok, -1 full/faded */
void tami_pet_feed(TamiAdventurer *a, uint8_t i);
void tami_pet_play(TamiAdventurer *a, uint8_t i);
int tami_pet_heel_set(TamiAdventurer *a, uint8_t i); /* 0 ok */
int tami_pet_release(TamiAdventurer *a, uint8_t i);  /* 0 ok */

void tami_format_status(const TamiAdventurer *a, char *buf, size_t n);
void tami_format_story(const TamiAdventurer *a, char *buf, size_t n);
void tami_format_recap(const TamiAdventurer *a, char *buf, size_t n);
void tami_format_chronicle(const TamiAdventurer *a, char *buf, size_t n);
void tami_format_log(const TamiAdventurer *a, char *buf, size_t n);
void tami_task_bar(const TamiAdventurer *a, char *buf, size_t n);
int tami_task_pct(const TamiAdventurer *a);
int tami_item_on(const TamiItem *it);
const char *tami_rarity_name(uint8_t r);
const char *tami_rarity_mark(uint8_t r);
void tami_rarity_rgb(uint8_t r, uint8_t *R, uint8_t *G, uint8_t *B);
uint8_t tami_worn_rarity(const TamiAdventurer *a);
const TamiItem *tami_best_gleam(const TamiAdventurer *a);
void tami_learn_spell(TamiAdventurer *a, const char *name);
const char *tami_plot_name(uint8_t act);
const char *tami_plot_epitaph(uint8_t act);
int tami_blob_sane(const TamiAdventurer *a);
int tami_save(const TamiAdventurer *a, const char *path);
int tami_load(TamiAdventurer *a, const char *path);
void tami_add_item(TamiAdventurer *a, const TamiItem *it);
void tami_give_relic(TamiAdventurer *a, TamiRng *rng);
int tami_hour(int64_t unix_ts);
int tami_is_night(int64_t unix_ts);
void tami_format_card(const TamiAdventurer *a, const TamiReport *r, char *buf, size_t n);

#endif
