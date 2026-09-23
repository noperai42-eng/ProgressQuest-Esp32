#include "tami/roster.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static char g_dir[192] = "roster";

void tami_roster_set_dir(const char *dir) {
    if (!dir || !dir[0]) {
        return;
    }
    snprintf(g_dir, sizeof(g_dir), "%s", dir);
}

static int ensure_dir(void) {
    if (mkdir(g_dir, 0755) == 0 || errno == EEXIST) {
        return 0;
    }
    return 0; /* mount point already exists on device */
}

static void slot_path(int slot, char *buf, size_t n) {
    snprintf(buf, n, "%s/s%d.tami", g_dir, slot);
}

static void meta_path(char *buf, size_t n) {
    snprintf(buf, n, "%s/meta.bin", g_dir);
}

void tami_roster_clear(TamiRoster *r) {
    if (!r) {
        return;
    }
    memset(r, 0, sizeof(*r));
    r->version = TAMI_ROSTER_VERSION;
    r->active = 0xFF;
}

int tami_roster_used(const TamiRoster *r) {
    if (!r) {
        return 0;
    }
    int n = 0;
    for (int i = 0; i < TAMI_ROSTER_SLOTS; i++) {
        if (r->card[i].used) {
            n++;
        }
    }
    return n;
}

int tami_roster_first_empty(const TamiRoster *r) {
    if (!r) {
        return -1;
    }
    for (int i = 0; i < TAMI_ROSTER_SLOTS; i++) {
        if (!r->card[i].used) {
            return i;
        }
    }
    return -1;
}

void tami_roster_label(const TamiRoster *r, int slot, char *buf, size_t n) {
    if (!buf || n == 0) {
        return;
    }
    buf[0] = 0;
    if (!r || slot < 0 || slot >= TAMI_ROSTER_SLOTS) {
        return;
    }
    const TamiRosterCard *c = &r->card[slot];
    if (!c->used) {
        snprintf(buf, n, "+  new");
        return;
    }
    if (c->faded) {
        snprintf(buf, n, "%s   faded  %s %s", c->given_name,
                 tami_people_name((TamiPeople)c->people), tami_calling_name((TamiCalling)c->calling));
        return;
    }
    snprintf(buf, n, "%s   lv %u  %s %s", c->given_name, (unsigned)c->level,
             tami_people_name((TamiPeople)c->people), tami_calling_name((TamiCalling)c->calling));
}

static void fill_card(TamiRosterCard *c, const TamiAdventurer *a) {
    memset(c, 0, sizeof(*c));
    c->used = 1;
    c->people = (uint8_t)a->people;
    c->calling = (uint8_t)a->calling;
    c->level = a->level;
    c->faded = a->faded;
    snprintf(c->given_name, sizeof(c->given_name), "%s", a->given_name);
}

int tami_roster_load(TamiRoster *r) {
    if (!r) {
        return -1;
    }
    tami_roster_clear(r);
    char path[256];
    meta_path(path, sizeof(path));
    FILE *f = fopen(path, "rb");
    if (!f) {
        return 0;
    }
    TamiRoster tmp;
    size_t n = fread(&tmp, sizeof(tmp), 1, f);
    fclose(f);
    if (n != 1 || tmp.version != TAMI_ROSTER_VERSION) {
        return 0;
    }
    *r = tmp;
    return 0;
}

int tami_roster_save_meta(const TamiRoster *r) {
    if (!r) {
        return -1;
    }
    ensure_dir();
    char path[256];
    meta_path(path, sizeof(path));
    FILE *f = fopen(path, "wb");
    if (!f) {
        return -1;
    }
    size_t n = fwrite(r, sizeof(*r), 1, f);
    fclose(f);
    return n == 1 ? 0 : -1;
}

int tami_roster_write_slot(TamiRoster *r, int slot, const TamiAdventurer *a, const TamiRng *rng) {
    if (!r || !a || !tami_blob_sane(a) || slot < 0 || slot >= TAMI_ROSTER_SLOTS) {
        return -1;
    }
    ensure_dir();
    char path[256];
    slot_path(slot, path, sizeof(path));
    FILE *f = fopen(path, "wb");
    if (!f) {
        return -1;
    }
    uint32_t rs = rng ? rng->s : 0;
    size_t n = fwrite(a, sizeof(*a), 1, f);
    n += fwrite(&rs, sizeof(rs), 1, f);
    fclose(f);
    if (n != 2) {
        return -1;
    }
    fill_card(&r->card[slot], a);
    r->version = TAMI_ROSTER_VERSION;
    r->active = (uint8_t)slot;
    return tami_roster_save_meta(r);
}

int tami_roster_read_slot(int slot, TamiAdventurer *a, TamiRng *rng) {
    if (!a || slot < 0 || slot >= TAMI_ROSTER_SLOTS) {
        return -1;
    }
    char path[256];
    slot_path(slot, path, sizeof(path));
    FILE *f = fopen(path, "rb");
    if (!f) {
        return -1;
    }
    TamiAdventurer tmp;
    uint32_t rs = 0;
    size_t n = fread(&tmp, sizeof(tmp), 1, f);
    (void)fread(&rs, sizeof(rs), 1, f);
    fclose(f);
    if (n != 1 || !tami_blob_sane(&tmp)) {
        return -1;
    }
    *a = tmp;
    if (rng && rs != 0) {
        rng->s = rs;
    }
    return 0;
}

int tami_roster_erase_slot(TamiRoster *r, int slot) {
    if (!r || slot < 0 || slot >= TAMI_ROSTER_SLOTS) {
        return -1;
    }
    char path[256];
    slot_path(slot, path, sizeof(path));
    (void)remove(path);
    memset(&r->card[slot], 0, sizeof(r->card[slot]));
    if (r->active == (uint8_t)slot) {
        r->active = 0xFF;
        for (int i = 0; i < TAMI_ROSTER_SLOTS; i++) {
            if (r->card[i].used) {
                r->active = (uint8_t)i;
                break;
            }
        }
    }
    return tami_roster_save_meta(r);
}
