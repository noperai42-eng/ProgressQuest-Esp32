#include "ui.h"

#include "art.h"
#include "persist.h"
#include "tami/anim.h"

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "lvgl.h"

static const char *TAG = "tami_ui";

enum {
    VIEW_HOME = 0,
    VIEW_CARE,
    VIEW_BAG,
    VIEW_LOG,
    VIEW_SHEET,
    VIEW_PETS,
    VIEW_FIELDS,
    VIEW_HATCH,
    VIEW_ROSTER
};

static lv_obj_t *s_disc;
static lv_obj_t *s_field_img;
static lv_obj_t *s_portal_ring;
static lv_obj_t *s_fig;
static lv_obj_t *s_wep;
static lv_obj_t *s_helm;
static lv_obj_t *s_shield;
static lv_obj_t *s_enemy;
static lv_obj_t *s_pet;
static lv_obj_t *s_pet_tag;
static lv_obj_t *s_pet_name;
static lv_obj_t *s_fire;
static lv_obj_t *s_blob;
static lv_obj_t *s_name_chip;
static lv_obj_t *s_name;
static lv_obj_t *s_meta_chip;
static lv_obj_t *s_meta;
static lv_obj_t *s_xp_arc;
static lv_obj_t *s_xp_bar;
static lv_obj_t *s_task_chip;
static lv_obj_t *s_task;
static lv_obj_t *s_bar;
static lv_obj_t *s_nav[5];
static lv_obj_t *s_burger;
static lv_obj_t *s_menu;
static lv_obj_t *s_menu_title;
static lv_obj_t *s_menu_body;
static lv_obj_t *s_sheet;
static lv_obj_t *s_sheet_xp_num;
static lv_obj_t *s_sheet_xp;
static lv_obj_t *s_sheet_hp;
static lv_obj_t *s_sheet_mp;
static lv_obj_t *s_sheet_stat[6];
static lv_obj_t *s_sheet_plot;
static lv_obj_t *s_sheet_plot_bar;
static lv_obj_t *s_sheet_quest;
static lv_obj_t *s_sheet_spells;
static lv_obj_t *s_gear;
static lv_obj_t *s_sheet_slot[TAMI_SLOT_COUNT];
static lv_obj_t *s_sheet_gear[TAMI_SLOT_COUNT];
static lv_obj_t *s_sheet_tab[2];
static int s_sheet_page;
static lv_obj_t *s_log;
static lv_obj_t *s_log_story;
static lv_obj_t *s_log_done;
static lv_obj_t *s_care_btn[4];
static lv_obj_t *s_field_btn[TAMI_FIELD_COUNT];
static lv_obj_t *s_pet_btn[4];
static lv_obj_t *s_hatch_p[4];
static lv_obj_t *s_hatch_c[4];
static lv_obj_t *s_hatch_go;
static lv_obj_t *s_hatch_back;
static lv_obj_t *s_roster_btn[TAMI_ROSTER_SLOTS];
static lv_obj_t *s_wake;
static lv_obj_t *s_toast;
static lv_obj_t *s_fx;
static lv_obj_t *s_mote[3];
static lv_obj_t *s_cut_lab;
static TamiAdventurer *s_adv;
static TamiRng *s_rng;
static int64_t *s_now;
static int s_hatched;
static int s_hud_on;
static int s_create_slot;
static int s_pick_people = TAMI_PEOPLE_ORC;
static int s_pick_call = TAMI_CALL_WARRIOR;
static float s_anim_t;
static TamiCut s_cut;
static float s_cut_left;
static int s_view = VIEW_HOME;
static int s_toast_left;
static int s_pet_sel;
static int64_t s_gone_until_us;
static lv_timer_t *s_anim_tm;
static char s_toast_txt[72];

static void hide_motes(void);

static const char *k_nav[5] = {"camp", "bag", "log", "sheet", "pets"};
static const char *k_people[4] = {"human", "orc", "elf", "undead"};
static const char *k_call[4] = {"warrior", "ranger", "mage", "rogue"};
static const char *k_care[4] = {"feed", "pep", "bandage", "camp"};
static const char *k_pet_act[4] = {"feed", "play", "heel", "gone"};

static lv_color_t people_color(TamiPeople p) {
    switch (p) {
    case TAMI_PEOPLE_ORC:
        return lv_color_make(72, 128, 64);
    case TAMI_PEOPLE_ELF:
        return lv_color_make(196, 210, 170);
    case TAMI_PEOPLE_UNDEAD:
        return lv_color_make(150, 168, 158);
    default:
        return lv_color_make(196, 154, 110);
    }
}

static lv_color_t field_color(TamiField f) {
    switch (f) {
    case TAMI_FIELD_IRONPIT:
        return lv_color_make(58, 36, 28);
    case TAMI_FIELD_MOONWOOD:
        return lv_color_make(22, 44, 52);
    case TAMI_FIELD_BARROW:
        return lv_color_make(36, 28, 52);
    case TAMI_FIELD_ASHFEN:
        return lv_color_make(40, 40, 38);
    default:
        return lv_color_make(32, 52, 36);
    }
}

static void style_btn(lv_obj_t *b, int hot) {
    lv_obj_set_style_bg_color(b, hot ? lv_color_make(92, 64, 36) : lv_color_make(18, 14, 16), 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(b, 12, 0);
    lv_obj_set_style_border_color(b, hot ? lv_color_make(255, 214, 130) : lv_color_make(196, 168, 104), 0);
    lv_obj_set_style_border_width(b, 2, 0);
    lv_obj_set_style_pad_all(b, 6, 0);
}

static lv_obj_t *make_chip(lv_obj_t *parent) {
    lv_obj_t *c = lv_obj_create(parent);
    lv_obj_set_style_bg_color(c, lv_color_make(8, 6, 10), 0);
    lv_obj_set_style_bg_opa(c, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(c, 12, 0);
    lv_obj_set_style_border_width(c, 0, 0);
    lv_obj_set_style_pad_hor(c, 14, 0);
    lv_obj_set_style_pad_ver(c, 6, 0);
    lv_obj_set_scrollable(c, false);
    lv_obj_set_clickable(c, false);
    lv_obj_set_height(c, LV_SIZE_CONTENT);
    lv_obj_set_width(c, LV_SIZE_CONTENT);
    return c;
}

static lv_obj_t *make_btn(lv_obj_t *parent, const char *txt, lv_event_cb_t cb, void *ud) {
    lv_obj_t *b = lv_button_create(parent);
    style_btn(b, 0);
    lv_obj_set_clickable(b, true);
    lv_obj_set_scrollable(b, false);
    lv_obj_set_ext_click_area(b, 22);
    lv_obj_t *lab = lv_label_create(b);
    lv_label_set_text(lab, txt);
    lv_obj_set_style_text_color(lab, lv_color_make(255, 244, 220), 0);
    lv_obj_set_style_text_font(lab, &lv_font_montserrat_16, 0);
    lv_obj_set_clickable(lab, false);
    lv_obj_center(lab);
    /* PRESSED: round-clip + finger slide often eats LV_EVENT_CLICKED. */
    lv_obj_add_event_cb(b, cb, LV_EVENT_PRESSED, ud);
    return b;
}

static void toast(const char *s) {
    snprintf(s_toast_txt, sizeof(s_toast_txt), "%s", s);
    s_toast_left = 3;
    if (s_toast) {
        lv_label_set_text(s_toast, s_toast_txt);
        lv_obj_set_hidden(s_toast, false);
    }
}

static void set_hidden(lv_obj_t *o, int hide) {
    if (o) {
        lv_obj_set_hidden(o, hide ? true : false);
    }
}

static void apply_nav_hud(void);

static void fill_bag(char *buf, size_t n, const TamiAdventurer *a) {
    size_t used = 0;
    int w = snprintf(buf, n, "gold %u   pack %d/%d\nloot on the way to market\n", (unsigned)a->gold,
                     tami_encumbrance(a), tami_encumbrance_max(a));
    if (w > 0) {
        used = (size_t)w;
    }
    if (a->inv_count == 0) {
        snprintf(buf + used, n - used, "pack empty");
        return;
    }
    int show = a->inv_count < 10 ? (int)a->inv_count : 10;
    for (int i = 0; i < show && used + 40 < n; i++) {
        int idx = (int)a->inv_count - 1 - i;
        uint8_t q = a->inv[idx].qty ? a->inv[idx].qty : 1;
        if (q > 1) {
            w = snprintf(buf + used, n - used, "%s%s x%u\n", tami_rarity_mark(a->inv[idx].rarity),
                         a->inv[idx].name, q);
        } else {
            w = snprintf(buf + used, n - used, "%s%s\n", tami_rarity_mark(a->inv[idx].rarity),
                         a->inv[idx].name);
        }
        if (w > 0) {
            used += (size_t)w;
        }
    }
    if (a->inv_count > 10 && used + 16 < n) {
        snprintf(buf + used, n - used, "... %u more", (unsigned)a->inv_count - 10);
    }
}

static void set_text_if(lv_obj_t *lab, const char *s) {
    if (!lab || !s) {
        return;
    }
    const char *cur = lv_label_get_text(lab);
    if (cur && strcmp(cur, s) == 0) {
        return;
    }
    lv_label_set_text(lab, s);
}

static void set_child_text(lv_obj_t *obj, const char *s) {
    lv_obj_t *lab = obj ? lv_obj_get_child(obj, 0) : NULL;
    if (lab) {
        set_text_if(lab, s);
    }
}

static void update_sheet(const TamiAdventurer *a) {
    char line[96];
    snprintf(line, sizeof(line), "%s  lv %u", a->given_name, (unsigned)a->level);
    set_text_if(s_menu_title, line);

    snprintf(line, sizeof(line), "%u/%u", (unsigned)a->xp, (unsigned)a->xp_need);
    set_text_if(s_sheet_xp_num, line);
    lv_bar_set_value(s_sheet_xp, tami_xp_pct(a), LV_ANIM_OFF);

    snprintf(line, sizeof(line), "HP   %u", a->hp_max);
    set_child_text(s_sheet_hp, line);
    snprintf(line, sizeof(line), "MP   %u", a->mp_max);
    set_child_text(s_sheet_mp, line);

    const char *labs[6] = {"STR", "CON", "DEX", "INT", "WIS", "CHA"};
    unsigned vals[6] = {a->str, a->con, a->dex, a->intel, a->wis, a->cha};
    for (int i = 0; i < 6; i++) {
        snprintf(line, sizeof(line), "%s   %u", labs[i], vals[i]);
        set_text_if(s_sheet_stat[i], line);
    }

    snprintf(line, sizeof(line), "%s   %u%%", tami_plot_name(a->plot_act), a->plot_progress);
    set_text_if(s_sheet_plot, line);
    lv_bar_set_value(s_sheet_plot_bar, (int32_t)a->plot_progress, LV_ANIM_OFF);
    if (a->quest_label[0]) {
        snprintf(line, sizeof(line), "%s  %u/%u", a->quest_label, (unsigned)a->quest_have,
                 (unsigned)a->quest_need);
        set_text_if(s_sheet_quest, line);
    } else {
        set_text_if(s_sheet_quest, "");
    }

    line[0] = 0;
    size_t used = 0;
    uint8_t n = a->spell_count < 6 ? a->spell_count : 6;
    for (uint8_t i = 0; i < n && used + 24 < sizeof(line); i++) {
        int w;
        if (a->spells[i].rank > 1) {
            w = snprintf(line + used, sizeof(line) - used, "%s %u  ", a->spells[i].name,
                         a->spells[i].rank);
        } else {
            w = snprintf(line + used, sizeof(line) - used, "%s  ", a->spells[i].name);
        }
        if (w > 0) {
            used += (size_t)w;
        }
    }
    set_text_if(s_sheet_spells, line[0] ? line : "(no spells yet)");
    for (int s = 0; s < TAMI_SLOT_COUNT; s++) {
        set_text_if(s_sheet_slot[s], tami_slot_name((TamiSlot)s));
        if (tami_item_on(&a->equip[s])) {
            snprintf(line, sizeof(line), "%s%s", tami_rarity_mark(a->equip[s].rarity),
                     a->equip[s].name);
            set_text_if(s_sheet_gear[s], line);
            uint8_t rr, rg, rb;
            tami_rarity_rgb(a->equip[s].rarity, &rr, &rg, &rb);
            lv_obj_set_style_text_color(s_sheet_gear[s], lv_color_make(rr, rg, rb), 0);
        } else {
            set_text_if(s_sheet_gear[s], "-");
            lv_obj_set_style_text_color(s_sheet_gear[s], lv_color_make(140, 130, 120), 0);
        }
    }
}

static void show_sheet_page(int page) {
    s_sheet_page = page ? 1 : 0;
    int on_sheet = (s_view == VIEW_SHEET);
    set_hidden(s_sheet_tab[0], !on_sheet);
    set_hidden(s_sheet_tab[1], !on_sheet);
    set_hidden(s_sheet, !on_sheet || s_sheet_page != 0);
    set_hidden(s_gear, !on_sheet || s_sheet_page != 1);
    if (s_sheet_tab[0]) {
        style_btn(s_sheet_tab[0], on_sheet && s_sheet_page == 0);
    }
    if (s_sheet_tab[1]) {
        style_btn(s_sheet_tab[1], on_sheet && s_sheet_page == 1);
    }
    if (on_sheet) {
        if (s_sheet_tab[0]) {
            lv_obj_move_foreground(s_sheet_tab[0]);
        }
        if (s_sheet_tab[1]) {
            lv_obj_move_foreground(s_sheet_tab[1]);
        }
    }
}

static void update_log(const TamiAdventurer *a) {
    set_text_if(s_menu_title, "story");
    if (s_log_story) {
        static char story[480];
        tami_format_chronicle(a, story, sizeof(story));
        set_text_if(s_log_story, story);
    }
    if (s_log_done) {
        static char done[640];
        tami_format_log(a, done, sizeof(done));
        set_text_if(s_log_done, done);
    }
}

static void fill_care(char *buf, size_t n, const TamiAdventurer *a) {
    const char *h = a->people == TAMI_PEOPLE_UNDEAD ? "ichor" : "hunger";
    snprintf(buf, n, "they camp when they must\n%s %u   rest %u\nmorale %u   wounds %u", h,
             a->hunger, a->rest, a->morale, a->wounds);
}

static void show_view(int view) {
    s_view = view;
    if (s_hatched) {
        apply_nav_hud();
    }
    int open = view != VIEW_HOME;
    if (open) {
        hide_motes();
        if (s_fx) {
            lv_obj_set_hidden(s_fx, true);
        }
        if (s_cut_lab) {
            lv_obj_set_hidden(s_cut_lab, true);
        }
    }
    if (s_anim_tm) {
        if (open) {
            lv_timer_pause(s_anim_tm);
        } else {
            lv_timer_resume(s_anim_tm);
        }
    }
    set_hidden(s_menu, !open);
    set_hidden(s_task_chip, open);
    set_hidden(s_bar, open);
    set_hidden(s_xp_bar, open);
    set_hidden(s_log, view != VIEW_LOG);
    set_hidden(s_menu_body, view == VIEW_SHEET || view == VIEW_LOG || view == VIEW_FIELDS);
    show_sheet_page(s_sheet_page);
    int pet_open = (view == VIEW_PETS && s_adv && s_adv->pet_count > 0);
    for (int i = 0; i < 4; i++) {
        set_hidden(s_care_btn[i], view != VIEW_CARE);
        set_hidden(s_pet_btn[i], !pet_open);
    }
    for (int i = 0; i < 5; i++) {
        style_btn(s_nav[i], view == i + 1);
    }
    for (int i = 0; i < TAMI_FIELD_COUNT; i++) {
        set_hidden(s_field_btn[i], view != VIEW_FIELDS);
    }
    if (!s_adv || !open) {
        return;
    }
    char body[320];
    body[0] = 0;
    if (view == VIEW_CARE) {
        lv_label_set_text(s_menu_title, "camp");
        fill_care(body, sizeof(body), s_adv);
    } else if (view == VIEW_BAG) {
        lv_label_set_text(s_menu_title, "bag");
        fill_bag(body, sizeof(body), s_adv);
    } else if (view == VIEW_FIELDS) {
        lv_label_set_text(s_menu_title, "fields");
        for (int i = 0; i < TAMI_FIELD_COUNT; i++) {
            char lab[40];
            const char *here = s_adv->field == (TamiField)i ? " *" : "";
            snprintf(lab, sizeof(lab), "%s  %d-%d%s", tami_field_name((TamiField)i),
                     tami_field_band_lo((TamiField)i), tami_field_band_hi((TamiField)i), here);
            lv_obj_t *labobj = lv_obj_get_child(s_field_btn[i], 0);
            if (labobj) {
                lv_label_set_text(labobj, lab);
            }
        }
    } else if (view == VIEW_LOG) {
        update_log(s_adv);
        lv_label_set_text(s_menu_body, "");
    } else if (view == VIEW_SHEET) {
        update_sheet(s_adv);
        lv_label_set_text(s_menu_body, "");
    } else if (view == VIEW_PETS) {
        lv_label_set_text(s_menu_title, "pets");
        if (s_adv->pet_count == 0) {
            snprintf(body, sizeof(body), "none yet\nthey find you on the road");
        } else {
            if (s_pet_sel >= s_adv->pet_count) {
                s_pet_sel = 0;
            }
            size_t used = 0;
            for (uint8_t i = 0; i < s_adv->pet_count && used + 48 < sizeof(body); i++) {
                const TamiPet *p = &s_adv->pets[i];
                int w = snprintf(body + used, sizeof(body) - used, "%s%s %s h%u m%u%s\n", p->name,
                                 i == s_adv->pet_active ? "*" : "",
                                 tami_pet_kind_name((TamiPetKind)p->kind), p->hunger, p->mood,
                                 (int)i == s_pet_sel ? " <" : "");
                if (w > 0) {
                    used += (size_t)w;
                }
            }
            snprintf(body + used, sizeof(body) - used, "%s",
                     tami_pet_trait((TamiPetKind)s_adv->pets[s_pet_sel].kind));
        }
    }
    lv_label_set_text(s_menu_body, body);
    for (int i = 0; i < 5; i++) {
        if (s_nav[i]) {
            lv_obj_move_foreground(s_nav[i]);
        }
    }
}

static void on_wake(lv_event_t *e) {
    (void)e;
    if (!s_hatched || !s_adv || !s_rng || !s_now || !s_adv->faded) {
        return;
    }
    if (tami_whelp(s_adv, s_rng, *s_now) != 0) {
        return;
    }
    (void)tami_persist_save(s_adv, s_rng);
    char line[48];
    snprintf(line, sizeof(line), "%s wakes", s_adv->given_name);
    toast(line);
    tami_ui_refresh(s_adv);
}

static void on_pep(lv_event_t *e) {
    lv_obj_t *t = lv_event_get_target(e);
    if (t != s_blob && t != s_fig && t != s_wake) {
        return;
    }
    if (!s_hatched || s_view != VIEW_HOME) {
        return;
    }
    if (s_adv && s_adv->faded) {
        on_wake(e);
        return;
    }
    if (s_adv) {
        tami_pep(s_adv);
        toast("pep");
    }
}

static void on_field_tap(lv_event_t *e) {
    if (lv_event_get_target(e) != s_disc) {
        return;
    }
    if (!s_hatched || s_view == VIEW_HOME || s_view == VIEW_ROSTER || s_view == VIEW_HATCH) {
        return;
    }
    s_hud_on = 0;
    show_view(VIEW_HOME);
}

static void on_burger(lv_event_t *e) {
    (void)e;
    if (!s_hatched) {
        return;
    }
    s_hud_on = !s_hud_on;
    apply_nav_hud();
}

static void on_nav(lv_event_t *e) {
    int which = (int)(intptr_t)lv_event_get_user_data(e);
    ESP_LOGI(TAG, "nav %d (was %d)", which, s_view);
    s_hud_on = 0;
    if (s_view == which) {
        show_view(VIEW_HOME);
    } else {
        show_view(which);
    }
}

static void on_sheet_tab(lv_event_t *e) {
    int page = (int)(intptr_t)lv_event_get_user_data(e);
    show_sheet_page(page);
}

static void on_care(lv_event_t *e) {
    int which = (int)(intptr_t)lv_event_get_user_data(e);
    if (!s_adv) {
        return;
    }
    if (s_adv->faded) {
        toast("faded — wake them");
        show_view(VIEW_HOME);
        return;
    }
    if (which == 0) {
        tami_feed(s_adv);
        toast(s_adv->people == TAMI_PEOPLE_UNDEAD ? "ichor up" : "fed");
    } else if (which == 1) {
        tami_pep(s_adv);
        toast("pep");
    } else if (which == 2) {
        tami_bandage(s_adv);
        toast("bandaged");
    } else {
        tami_force_camp(s_adv);
        toast("camped");
        show_view(VIEW_HOME);
        return;
    }
    show_view(VIEW_CARE);
}

static void on_pet_act(lv_event_t *e) {
    int which = (int)(intptr_t)lv_event_get_user_data(e);
    if (!s_adv || s_adv->pet_count == 0) {
        return;
    }
    uint8_t s = (uint8_t)s_pet_sel;
    if (s >= s_adv->pet_count) {
        s = 0;
    }
    if (which == 0) {
        s_gone_until_us = 0;
        tami_pet_feed(s_adv, s);
        toast("fed pet");
    } else if (which == 1) {
        s_gone_until_us = 0;
        tami_pet_play(s_adv, s);
        toast("played");
    } else if (which == 2) {
        s_gone_until_us = 0;
        tami_pet_heel_set(s_adv, s);
        toast("at heel");
    } else {
        int64_t now = esp_timer_get_time();
        if (s_gone_until_us == 0 || now > s_gone_until_us) {
            s_gone_until_us = now + 3000000; /* 3s to confirm */
            char warn[64];
            snprintf(warn, sizeof(warn), "gone again to banish %s", s_adv->pets[s].name);
            toast(warn);
            show_view(VIEW_PETS);
            return;
        }
        char line[48];
        snprintf(line, sizeof(line), "let %s go", s_adv->pets[s].name);
        tami_pet_release(s_adv, s);
        toast(line);
        s_gone_until_us = 0;
        s_pet_sel = 0;
    }
    show_view(VIEW_PETS);
}

static int look_tier(uint8_t look) {
    switch (look) {
    case TAMI_LOOK_PLATE:
    case TAMI_LOOK_HELM:
    case TAMI_LOOK_GAUNTLETS:
    case TAMI_LOOK_GREAVES:
    case TAMI_LOOK_SOLLERET:
    case TAMI_LOOK_BRASSAIRT:
        return 3;
    case TAMI_LOOK_MAIL:
    case TAMI_LOOK_HAUBERK:
    case TAMI_LOOK_COIF:
    case TAMI_LOOK_VAMBRACE:
    case TAMI_LOOK_CUISSE:
        return 2;
    case TAMI_LOOK_LEATHER:
    case TAMI_LOOK_BOOTS:
    case TAMI_LOOK_GLOVES:
    case TAMI_LOOK_CAP:
        return 1;
    default:
        return 0;
    }
}

static int armor_tier(const TamiAdventurer *a) {
    int t = 0;
    for (int s = 0; s < TAMI_SLOT_COUNT; s++) {
        if (s == TAMI_SLOT_WEAPON || s == TAMI_SLOT_SHIELD) {
            continue;
        }
        if (tami_item_on(&a->equip[s])) {
            int lt = look_tier(a->equip[s].look);
            if (lt > t) {
                t = lt;
            }
        }
    }
    return t;
}

static void set_img(lv_obj_t *o, const lv_image_dsc_t *src, int show);

static void hide_loadout(void) {
    set_img(s_wep, NULL, 0);
    set_img(s_helm, NULL, 0);
    set_img(s_shield, NULL, 0);
}

static void apply_loadout(const TamiAdventurer *adv) {
    if (!adv || adv->faded) {
        hide_loadout();
        return;
    }
    const TamiItem *wep = &adv->equip[TAMI_SLOT_WEAPON];
    const TamiItem *sh = &adv->equip[TAMI_SLOT_SHIELD];
    const TamiItem *head = &adv->equip[TAMI_SLOT_HELM];
    set_img(s_wep, tami_item_on(wep) ? tami_art_weapon(wep->look) : NULL, tami_item_on(wep));
    set_img(s_helm, tami_item_on(head) ? tami_art_helm(head->look) : NULL, tami_item_on(head));
    set_img(s_shield, tami_item_on(sh) ? tami_art_shield(sh->look) : NULL, tami_item_on(sh));
}

static void set_img(lv_obj_t *o, const lv_image_dsc_t *src, int show) {
    if (!o) {
        return;
    }
    if (src && show) {
        lv_image_set_src(o, src);
        lv_obj_set_hidden(o, false);
    } else {
        lv_obj_set_hidden(o, true);
    }
}

static void apply_art(const TamiAdventurer *adv) {
    set_img(s_field_img, tami_art_field(adv->field), 1);
    set_img(s_portal_ring, tami_art_portal_ring(), 1);
    int tier = armor_tier(adv);
    const lv_image_dsc_t *fig = tami_art_figure(adv->people, tier);
    set_img(s_fig, fig, fig != NULL);
    apply_loadout(adv);
    if (s_blob) {
        lv_obj_set_hidden(s_blob, fig != NULL);
        if (!fig) {
            lv_obj_set_style_bg_color(s_blob, people_color(adv->people), 0);
        }
    }
    {
        const lv_image_dsc_t *mob = tami_art_enemy();
        int fam = tami_kill_family(adv);
        if (fam >= 0) {
            const lv_image_dsc_t *typed = tami_art_mob((uint8_t)fam);
            if (typed) {
                mob = typed;
            }
        }
        set_img(s_enemy, mob, tami_task_is_fight(adv) && !adv->faded);
    }
    set_img(s_fire, tami_art_campfire(), adv->task.kind == TAMI_TASK_CAMP && !adv->faded);
    const TamiPet *side = tami_pet_beside(adv);
    set_img(s_pet, side ? tami_art_pet((TamiPetKind)side->kind) : NULL, side != NULL);
    if (s_pet && side) {
        lv_obj_set_style_opa(s_pet, tami_pet_heel(adv) ? LV_OPA_COVER : LV_OPA_60, 0);
    }
    if (s_pet_tag) {
        lv_obj_set_hidden(s_pet_tag, side == NULL);
    }
    if (s_pet_name && side) {
        lv_label_set_text(s_pet_name, side->name);
    }
}

static void hide_motes(void) {
    for (int i = 0; i < 3; i++) {
        if (s_mote[i]) {
            lv_obj_set_hidden(s_mote[i], true);
        }
    }
}

static void tami_ui_animate(void) {
    if (!s_adv) {
        return;
    }
    /* Never lv_image_set_rotation / set_scale on RGB565A8 inside the round
     * clip_corner disc — that stalls LVGL (struck-down lockup). Pose with
     * align + opa only. */
    if (!s_hatched) {
        s_anim_t += 0.08f;
        lv_obj_align(s_fig, LV_ALIGN_CENTER, 0, -40 + (int)(sinf(s_anim_t * 2.2f) * 3.0f));
        hide_loadout();
        hide_motes();
        if (s_fx) {
            lv_obj_set_hidden(s_fx, true);
        }
        return;
    }
    s_anim_t += 0.08f;
    if (s_view != VIEW_HOME) {
        return;
    }
    if (s_cut_left > 0) {
        s_cut_left -= 0.08f;
        if (s_cut_left < 0) {
            s_cut_left = 0;
            s_cut = TAMI_CUT_NONE;
        }
    }
    TamiAnimPose p;
    tami_anim_pose(s_adv, s_anim_t, &p);
    lv_obj_align(s_fig, LV_ALIGN_CENTER, -36 + p.fig_dx, 4 + p.fig_dy);
    lv_obj_set_style_opa(s_fig, (lv_opa_t)p.alpha, 0);
    {
        int fx = -36 + p.fig_dx;
        int fy = 4 + p.fig_dy;
        uint8_t sl = s_adv->equip[TAMI_SLOT_SHIELD].look;
        uint8_t wl = s_adv->equip[TAMI_SLOT_WEAPON].look;
        uint8_t hl = s_adv->equip[TAMI_SLOT_HELM].look;
        int sdx = sl == TAMI_LOOK_SHIELD_TOWER ? -44 : -40;
        int sdy = 11;
        int wdx = 32, wdy = 22;
        if (wl == TAMI_LOOK_HATCHET || wl == TAMI_LOOK_PICK) {
            wdx = 36;
            wdy = 25;
        } else if (wl == TAMI_LOOK_SPEAR || wl == TAMI_LOOK_POLE) {
            wdx = 34;
            wdy = 21;
        } else if (wl == TAMI_LOOK_BLADE || wl == TAMI_LOOK_SHIV) {
            wdx = 34;
            wdy = 28;
        }
        int hdy = -53;
        if (hl == TAMI_LOOK_CAP) {
            hdy = -63;
        } else if (hl == TAMI_LOOK_COIF) {
            hdy = -44;
        }
        lv_obj_align(s_shield, LV_ALIGN_CENTER, fx + sdx, fy + sdy);
        lv_obj_align(s_wep, LV_ALIGN_CENTER, fx + wdx, fy + wdy);
        lv_obj_align(s_helm, LV_ALIGN_CENTER, fx, fy + hdy);
        lv_obj_align(s_pet, LV_ALIGN_CENTER, fx - 92 + p.pet_dx, fy - 36 + p.pet_dy);
        if (s_pet_tag) {
            lv_obj_align(s_pet_tag, LV_ALIGN_CENTER, fx - 92 + p.pet_dx, fy - 2 + p.pet_dy);
        }
    }
    lv_obj_align(s_enemy, LV_ALIGN_CENTER, 110 + p.enemy_dx, 20 + p.enemy_dy);
    lv_obj_set_style_opa(s_enemy, (lv_opa_t)p.enemy_alpha, 0);
    lv_obj_align(s_fire, LV_ALIGN_CENTER, -110, 64);
    {
        int fopa = 200 + p.fire_h_add * 4;
        if (fopa < 140) {
            fopa = 140;
        }
        if (fopa > 255) {
            fopa = 255;
        }
        lv_obj_set_style_opa(s_fire, (lv_opa_t)fopa, 0);
    }

    if (s_fx) {
        lv_obj_set_hidden(s_fx, true);
    }
    if (s_cut != TAMI_CUT_NONE && s_cut_left > 0 && s_view == VIEW_HOME && s_cut_lab) {
        float u = 1.0f - s_cut_left / 0.85f;
        int opa = u < 0.2f ? (int)(255.0f * u / 0.2f)
                           : (int)(255.0f * (1.0f - (u - 0.2f) / 0.8f));
        if (opa < 0) {
            opa = 0;
        }
        if (opa > 255) {
            opa = 255;
        }
        const char *cap = tami_cut_caption(s_cut);
        const char *cur = lv_label_get_text(s_cut_lab);
        if (!cur || strcmp(cur, cap) != 0) {
            lv_label_set_text(s_cut_lab, cap);
        }
        lv_obj_set_hidden(s_cut_lab, false);
        lv_obj_set_style_opa(s_cut_lab, (lv_opa_t)opa, 0);
    } else if (s_cut_lab) {
        lv_obj_set_hidden(s_cut_lab, true);
    }

    uint8_t gleam = tami_worn_rarity(s_adv);
    int show_mote = s_view == VIEW_HOME && gleam >= TAMI_RARITY_RARE && !s_adv->faded;
    lv_color_t mc = gleam >= TAMI_RARITY_LEGENDARY ? lv_color_make(255, 210, 90)
                                                   : lv_color_make(130, 180, 255);
    int n = gleam >= TAMI_RARITY_LEGENDARY ? 3 : 2;
    for (int i = 0; i < 3; i++) {
        if (!s_mote[i]) {
            continue;
        }
        int on = show_mote && i < n;
        lv_obj_set_hidden(s_mote[i], !on);
        if (!on) {
            continue;
        }
        lv_obj_set_style_bg_color(s_mote[i], mc, 0);
        float ph = s_anim_t * 2.4f + (float)i * 1.7f;
        int mx = -36 + p.fig_dx + (int)(cosf(ph) * 34.0f);
        int my = 4 + p.fig_dy - 40 + (int)(sinf(ph * 1.35f) * 22.0f);
        lv_obj_align(s_mote[i], LV_ALIGN_CENTER, mx, my);
        int opa = 90 + (int)(sinf(ph * 1.8f) * 80.0f);
        if (opa < 40) {
            opa = 40;
        }
        lv_obj_set_style_opa(s_mote[i], (lv_opa_t)opa, 0);
    }
}

static void on_anim_timer(lv_timer_t *tm) {
    (void)tm;
    tami_ui_animate();
}

void tami_ui_side_key(int dir) {
    if (!s_hatched) {
        return;
    }
    if (s_view == VIEW_PETS && s_adv && s_adv->pet_count > 0) {
        int n = (int)s_adv->pet_count;
        s_pet_sel = (s_pet_sel + (dir < 0 ? 1 : n - 1)) % n;
        show_view(VIEW_PETS);
        return;
    }
    lv_obj_t *sc = NULL;
    if (s_view == VIEW_SHEET) {
        sc = (s_sheet_page == 1) ? s_gear : s_sheet;
    } else if (s_view == VIEW_LOG) {
        sc = s_log;
    }
    if (!sc) {
        return;
    }
    /* Instant scroll — animated scroll fights the home redraw and feels laggy. */
    lv_coord_t dy = (dir > 0) ? 72 : -72;
    lv_obj_scroll_by(sc, 0, dy, LV_ANIM_OFF);
}

int tami_ui_menu_open(void) {
    return s_hatched && s_view != VIEW_HOME;
}

void tami_ui_on_report(const TamiReport *r) {
    TamiCut c = tami_cut_from_report(r);
    if (c != TAMI_CUT_NONE) {
        s_cut = c;
        s_cut_left = 0.85f;
    }
}

static void on_meta(lv_event_t *e) {
    (void)e;
    if (!s_hatched) {
        return;
    }
    if (s_view == VIEW_FIELDS) {
        show_view(VIEW_HOME);
    } else {
        show_view(VIEW_FIELDS);
    }
}

static void on_field(lv_event_t *e) {
    int which = (int)(intptr_t)lv_event_get_user_data(e);
    if (!s_adv || which < 0 || which >= TAMI_FIELD_COUNT) {
        return;
    }
    if (tami_set_field(s_adv, (TamiField)which) == 0) {
        char line[48];
        snprintf(line, sizeof(line), "sent to %s", tami_field_name((TamiField)which));
        toast(line);
        show_view(VIEW_HOME);
    }
}

static void hatch_preview(void) {
    const lv_image_dsc_t *fig = tami_art_figure((TamiPeople)s_pick_people, 0);
    set_img(s_fig, fig, fig != NULL);
    set_img(s_field_img, tami_art_portal_void(), 1);
    set_img(s_enemy, NULL, 0);
    set_img(s_pet, NULL, 0);
    set_hidden(s_pet_tag, 1);
    set_img(s_fire, NULL, 0);
    hide_loadout();
    if (s_name) {
        lv_label_set_text(s_name, "who wakes?");
    }
    if (s_meta) {
        char line[48];
        snprintf(line, sizeof(line), "%s  %s", k_people[s_pick_people], k_call[s_pick_call]);
        lv_label_set_text(s_meta, line);
    }
    for (int i = 0; i < 4; i++) {
        style_btn(s_hatch_p[i], s_pick_people == i);
        style_btn(s_hatch_c[i], s_pick_call == i);
    }
}

static void apply_nav_hud(void) {
    int meta = s_hatched && s_view != VIEW_ROSTER && s_view != VIEW_HATCH;
    int faded = s_adv && s_adv->faded;
    int wake = meta && s_view == VIEW_HOME && faded;
    int nav = meta && !faded && s_hud_on;
    for (int i = 0; i < 5; i++) {
        set_hidden(s_nav[i], !nav);
        if (nav && s_nav[i]) {
            lv_obj_move_foreground(s_nav[i]);
        }
    }
    set_hidden(s_burger, !(meta && !faded));
    if (s_burger) {
        set_child_text(s_burger, s_hud_on ? "hide" : "menu");
        style_btn(s_burger, s_hud_on);
        if (meta && !faded) {
            lv_obj_move_foreground(s_burger);
        }
    }
    set_hidden(s_wake, !wake);
    if (wake && s_wake) {
        lv_obj_move_foreground(s_wake);
    }
}

static void set_play_chrome(int on) {
    set_hidden(s_task_chip, !on);
    set_hidden(s_bar, !on);
    set_hidden(s_xp_bar, !on);
    set_hidden(s_xp_arc, !on);
    if (!on) {
        set_hidden(s_menu, 1);
        set_hidden(s_wake, 1);
        set_hidden(s_burger, 1);
        for (int i = 0; i < 5; i++) {
            set_hidden(s_nav[i], 1);
        }
    } else {
        apply_nav_hud();
    }
}

static void set_hatch_ui(int on) {
    for (int i = 0; i < 4; i++) {
        set_hidden(s_hatch_p[i], !on);
        set_hidden(s_hatch_c[i], !on);
    }
    set_hidden(s_hatch_go, !on);
    int back = 0;
    if (on) {
        const TamiRoster *ros = tami_persist_roster();
        back = ros && tami_roster_used(ros) > 0;
    }
    set_hidden(s_hatch_back, !back);
    if (on) {
        set_play_chrome(0);
        for (int i = 0; i < TAMI_ROSTER_SLOTS; i++) {
            set_hidden(s_roster_btn[i], 1);
        }
        set_hidden(s_portal_ring, 1);
    }
}

static void roster_preview(void) {
    const TamiRoster *ros = tami_persist_roster();
    set_img(s_fig, NULL, 0);
    set_img(s_field_img, tami_art_portal_void(), 1);
    set_img(s_enemy, NULL, 0);
    set_img(s_pet, NULL, 0);
    set_hidden(s_pet_tag, 1);
    set_img(s_fire, NULL, 0);
    hide_loadout();
    if (s_name) {
        lv_label_set_text(s_name, "who walks?");
    }
    if (s_meta) {
        lv_label_set_text(s_meta, "pick a soul, or hatch new");
    }
    for (int i = 0; i < TAMI_ROSTER_SLOTS; i++) {
        if (!s_roster_btn[i]) {
            continue;
        }
        char lab[64];
        tami_roster_label(ros, i, lab, sizeof(lab));
        lv_obj_t *txt = lv_obj_get_child(s_roster_btn[i], 0);
        if (txt) {
            lv_label_set_text(txt, lab);
        }
        int hot = ros && ros->active == (uint8_t)i && ros->card[i].used;
        style_btn(s_roster_btn[i], hot);
        set_hidden(s_roster_btn[i], 0);
    }
}

static void set_roster_ui(int on) {
    for (int i = 0; i < TAMI_ROSTER_SLOTS; i++) {
        set_hidden(s_roster_btn[i], !on);
    }
    if (on) {
        set_hatch_ui(0);
        set_play_chrome(0);
        set_hidden(s_hatch_back, 1);
        set_hidden(s_portal_ring, 1);
        roster_preview();
    }
}

void tami_ui_do_hatch(TamiPeople p, TamiCalling c) {
    if (!s_adv || !s_rng) {
        return;
    }
    if (s_hatched && s_adv && tami_blob_sane(s_adv)) {
        (void)tami_persist_save(s_adv, s_rng);
    }
    const TamiRoster *ros = tami_persist_roster();
    int slot = s_create_slot;
    if (slot < 0 || slot >= TAMI_ROSTER_SLOTS || (ros && ros->card[slot].used)) {
        slot = ros ? tami_roster_first_empty(ros) : 0;
    }
    if (slot < 0) {
        toast("roster full");
        return;
    }
    s_pick_people = (int)p;
    s_pick_call = (int)c;
    int64_t now = s_now ? *s_now : 1000000;
    tami_hatch(s_adv, p, c, s_rng, now);
    if (s_now) {
        *s_now = now;
    }
    (void)tami_persist_create(slot, s_adv, s_rng);
    s_create_slot = slot;
    s_hatched = 1;
    s_view = VIEW_HOME;
    set_hatch_ui(0);
    set_roster_ui(0);
    set_play_chrome(1);
    apply_art(s_adv);
    tami_ui_refresh(s_adv);
    ESP_LOGI(TAG, "hatched %s slot %d", s_adv->given_name, slot);
}

int tami_ui_hatched(void) {
    return s_hatched;
}

void tami_ui_resume(void) {
    if (!s_adv || !tami_blob_sane(s_adv)) {
        return;
    }
    s_hatched = 1;
    s_view = VIEW_HOME;
    set_hatch_ui(0);
    set_roster_ui(0);
    set_play_chrome(1);
    apply_art(s_adv);
    tami_ui_refresh(s_adv);
    ESP_LOGI(TAG, "resume %s the %s %s lv %u", s_adv->given_name, s_adv->people_name,
             s_adv->calling_name, (unsigned)s_adv->level);
}

void tami_ui_show_roster(void) {
    if (s_hatched && s_adv && tami_blob_sane(s_adv)) {
        (void)tami_persist_save(s_adv, s_rng);
    }
    s_hatched = 0;
    s_view = VIEW_ROSTER;
    hide_loadout();
    hide_motes();
    set_hatch_ui(0);
    set_roster_ui(1);
}

void tami_ui_unhatch(void) {
    const TamiRoster *ros = tami_persist_roster();
    if (ros && tami_roster_used(ros) > 0) {
        tami_ui_show_roster();
        return;
    }
    s_hatched = 0;
    s_view = VIEW_HATCH;
    s_create_slot = 0;
    if (s_adv) {
        memset(s_adv, 0, sizeof(*s_adv));
    }
    hide_loadout();
    set_roster_ui(0);
    set_hatch_ui(1);
    hatch_preview();
}

static void on_hatch_people(lv_event_t *e) {
    s_pick_people = (int)(intptr_t)lv_event_get_user_data(e);
    hatch_preview();
}

static void on_hatch_call(lv_event_t *e) {
    s_pick_call = (int)(intptr_t)lv_event_get_user_data(e);
    hatch_preview();
}

static void on_hatch_go(lv_event_t *e) {
    (void)e;
    tami_ui_do_hatch((TamiPeople)s_pick_people, (TamiCalling)s_pick_call);
}

static void on_hatch_back(lv_event_t *e) {
    (void)e;
    tami_ui_show_roster();
}

static void on_roster_slot(lv_event_t *e) {
    int slot = (int)(intptr_t)lv_event_get_user_data(e);
    const TamiRoster *ros = tami_persist_roster();
    if (!ros || slot < 0 || slot >= TAMI_ROSTER_SLOTS) {
        return;
    }
    if (!ros->card[slot].used) {
        s_create_slot = slot;
        s_view = VIEW_HATCH;
        set_roster_ui(0);
        set_hatch_ui(1);
        hatch_preview();
        return;
    }
    if (!s_adv || !s_rng) {
        return;
    }
    if (tami_persist_select(slot, s_adv, s_rng) != 0) {
        toast("cannot wake");
        return;
    }
    if (s_now) {
        *s_now = s_adv->last_tick_unix;
        if (*s_now < 1000000) {
            *s_now = 1000000;
        }
    }
    tami_ui_resume();
}

static void on_name(lv_event_t *e) {
    (void)e;
    if (!s_hatched) {
        return;
    }
    tami_ui_show_roster();
}

void tami_ui_init(TamiAdventurer *adv, TamiRng *rng, int64_t *now) {
    s_adv = adv;
    s_rng = rng;
    s_now = now;
    s_hatched = 0;
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    s_disc = lv_obj_create(scr);
    lv_obj_set_size(s_disc, 466, 466);
    lv_obj_center(s_disc);
    lv_obj_set_style_radius(s_disc, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_clip_corner(s_disc, true, 0);
    lv_obj_set_style_border_width(s_disc, 0, 0);
    lv_obj_set_style_pad_all(s_disc, 0, 0);
    lv_obj_set_style_bg_color(s_disc, field_color(adv->field), 0);
    lv_obj_set_scrollable(s_disc, false);
    lv_obj_set_clickable(s_disc, true);
    lv_obj_add_event_cb(s_disc, on_field_tap, LV_EVENT_PRESSED, NULL);

    s_field_img = lv_image_create(s_disc);
    lv_obj_center(s_field_img);
    lv_obj_set_clickable(s_field_img, false);

    s_portal_ring = lv_image_create(s_disc);
    lv_obj_center(s_portal_ring);
    lv_obj_set_clickable(s_portal_ring, false);
    if (tami_art_portal_ring()) {
        lv_image_set_src(s_portal_ring, tami_art_portal_ring());
    }

    s_xp_arc = lv_arc_create(s_disc);
    lv_obj_set_size(s_xp_arc, 450, 450);
    lv_obj_center(s_xp_arc);
    lv_arc_set_rotation(s_xp_arc, 270);
    lv_arc_set_bg_angles(s_xp_arc, 0, 360);
    lv_arc_set_range(s_xp_arc, 0, 100);
    lv_arc_set_value(s_xp_arc, 0);
    lv_obj_set_clickable(s_xp_arc, false);
    lv_obj_remove_style(s_xp_arc, NULL, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(s_xp_arc, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(s_xp_arc, 0, 0);
    lv_obj_set_style_arc_width(s_xp_arc, 6, LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_xp_arc, lv_color_make(40, 36, 28), LV_PART_MAIN);
    lv_obj_set_style_arc_opa(s_xp_arc, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_xp_arc, 6, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_xp_arc, lv_color_make(220, 180, 70), LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(s_xp_arc, LV_OPA_COVER, LV_PART_INDICATOR);

    s_fig = lv_image_create(s_disc);
    lv_obj_align(s_fig, LV_ALIGN_CENTER, 0, -40);
    lv_obj_set_clickable(s_fig, true);
    lv_obj_add_event_cb(s_fig, on_pep, LV_EVENT_PRESSED, NULL);
    s_shield = lv_image_create(s_disc);
    lv_obj_set_clickable(s_shield, false);
    lv_obj_set_hidden(s_shield, true);
    s_wep = lv_image_create(s_disc);
    lv_obj_set_clickable(s_wep, false);
    lv_obj_set_hidden(s_wep, true);
    s_helm = lv_image_create(s_disc);
    lv_obj_set_clickable(s_helm, false);
    lv_obj_set_hidden(s_helm, true);
    if (s_portal_ring) {
        lv_obj_move_foreground(s_portal_ring);
    }

    s_enemy = lv_image_create(s_disc);
    lv_obj_align(s_enemy, LV_ALIGN_CENTER, 110, 20);
    lv_obj_set_clickable(s_enemy, false);
    lv_obj_set_hidden(s_enemy, true);

    s_pet = lv_image_create(s_disc);
    lv_obj_align(s_pet, LV_ALIGN_CENTER, -128, -32);
    lv_obj_set_clickable(s_pet, false);
    lv_obj_set_hidden(s_pet, true);
    s_pet_tag = make_chip(s_disc);
    lv_obj_set_hidden(s_pet_tag, true);
    s_pet_name = lv_label_create(s_pet_tag);
    lv_obj_set_style_text_color(s_pet_name, lv_color_make(255, 244, 220), 0);
    lv_obj_set_style_text_font(s_pet_name, &lv_font_montserrat_14, 0);
    lv_obj_center(s_pet_name);

    s_fire = lv_image_create(s_disc);
    lv_obj_align(s_fire, LV_ALIGN_CENTER, -110, 64);
    lv_obj_set_clickable(s_fire, false);
    lv_obj_set_hidden(s_fire, true);

    s_name_chip = make_chip(s_disc);
    lv_obj_align(s_name_chip, LV_ALIGN_TOP_MID, 0, 22);
    lv_obj_set_clickable(s_name_chip, true);
    lv_obj_set_ext_click_area(s_name_chip, 16);
    lv_obj_add_event_cb(s_name_chip, on_name, LV_EVENT_PRESSED, NULL);
    s_name = lv_label_create(s_name_chip);
    lv_obj_set_style_text_color(s_name, lv_color_make(255, 244, 220), 0);
    lv_obj_set_style_text_font(s_name, &lv_font_montserrat_20, 0);
    lv_obj_center(s_name);

    s_meta_chip = make_chip(s_disc);
    lv_obj_align(s_meta_chip, LV_ALIGN_TOP_MID, 0, 58);
    lv_obj_set_clickable(s_meta_chip, true);
    lv_obj_set_ext_click_area(s_meta_chip, 16);
    lv_obj_add_event_cb(s_meta_chip, on_meta, LV_EVENT_PRESSED, NULL);
    s_meta = lv_label_create(s_meta_chip);
    lv_obj_set_style_text_color(s_meta, lv_color_make(230, 214, 170), 0);
    lv_obj_set_style_text_font(s_meta, &lv_font_montserrat_16, 0);
    lv_obj_set_clickable(s_meta, false);
    lv_obj_center(s_meta);

    s_xp_bar = lv_bar_create(s_disc);
    lv_obj_set_size(s_xp_bar, 240, 12);
    lv_bar_set_range(s_xp_bar, 0, 100);
    lv_obj_align(s_xp_bar, LV_ALIGN_TOP_MID, 0, 88);
    lv_obj_set_style_bg_color(s_xp_bar, lv_color_make(8, 6, 10), 0);
    lv_obj_set_style_bg_opa(s_xp_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_xp_bar, 6, 0);
    lv_obj_set_style_border_color(s_xp_bar, lv_color_make(196, 168, 104), 0);
    lv_obj_set_style_border_width(s_xp_bar, 1, 0);
    lv_obj_set_style_bg_color(s_xp_bar, lv_color_make(220, 180, 70), LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_xp_bar, 5, LV_PART_INDICATOR);

    s_blob = lv_obj_create(s_disc);
    lv_obj_set_size(s_blob, 90, 110);
    lv_obj_set_style_radius(s_blob, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(s_blob, 0, 0);
    lv_obj_set_style_bg_color(s_blob, people_color(adv->people), 0);
    lv_obj_align(s_blob, LV_ALIGN_CENTER, -36, 8);
    lv_obj_set_clickable(s_blob, true);
    lv_obj_add_event_cb(s_blob, on_pep, LV_EVENT_PRESSED, NULL);
    lv_obj_set_hidden(s_blob, true);

    s_task_chip = make_chip(s_disc);
    lv_obj_set_width(s_task_chip, 300);
    lv_obj_align(s_task_chip, LV_ALIGN_TOP_MID, 0, 268);
    s_task = lv_label_create(s_task_chip);
    lv_obj_set_style_text_color(s_task, lv_color_make(255, 244, 220), 0);
    lv_obj_set_style_text_font(s_task, &lv_font_montserrat_14, 0);
    lv_obj_set_width(s_task, 276);
    lv_label_set_long_mode(s_task, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(s_task, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(s_task);

    s_bar = lv_bar_create(s_disc);
    lv_obj_set_size(s_bar, 290, 18);
    lv_bar_set_range(s_bar, 0, 100);
    lv_obj_align(s_bar, LV_ALIGN_TOP_MID, 0, 300);

    s_toast = lv_label_create(s_disc);
    lv_obj_set_style_text_color(s_toast, lv_color_make(255, 220, 140), 0);
    lv_obj_set_style_text_font(s_toast, &lv_font_montserrat_14, 0);
    lv_obj_set_style_bg_color(s_toast, lv_color_make(8, 6, 10), 0);
    lv_obj_set_style_bg_opa(s_toast, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_toast, 10, 0);
    lv_obj_set_style_pad_hor(s_toast, 12, 0);
    lv_obj_set_style_pad_ver(s_toast, 5, 0);
    lv_obj_align(s_toast, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_hidden(s_toast, true);

    s_fx = lv_image_create(s_disc);
    lv_obj_center(s_fx);
    lv_obj_set_clickable(s_fx, false);
    lv_obj_set_hidden(s_fx, true);

    for (int i = 0; i < 3; i++) {
        s_mote[i] = lv_obj_create(s_disc);
        lv_obj_set_size(s_mote[i], 8, 8);
        lv_obj_set_style_radius(s_mote[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(s_mote[i], lv_color_make(255, 210, 90), 0);
        lv_obj_set_style_bg_opa(s_mote[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(s_mote[i], 0, 0);
        lv_obj_set_clickable(s_mote[i], false);
        lv_obj_set_hidden(s_mote[i], true);
    }

    s_cut_lab = lv_label_create(s_disc);
    lv_obj_set_style_text_color(s_cut_lab, lv_color_make(255, 230, 140), 0);
    lv_obj_set_style_text_font(s_cut_lab, &lv_font_montserrat_16, 0);
    lv_obj_set_style_bg_color(s_cut_lab, lv_color_make(8, 6, 10), 0);
    lv_obj_set_style_bg_opa(s_cut_lab, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_cut_lab, 10, 0);
    lv_obj_set_style_pad_hor(s_cut_lab, 12, 0);
    lv_obj_set_style_pad_ver(s_cut_lab, 5, 0);
    lv_obj_align(s_cut_lab, LV_ALIGN_TOP_MID, 0, 88);
    lv_obj_set_hidden(s_cut_lab, true);

    s_menu = lv_obj_create(s_disc);
    lv_obj_set_size(s_menu, 330, 236);
    lv_obj_align(s_menu, LV_ALIGN_TOP_MID, 0, 78);
    lv_obj_set_style_radius(s_menu, 16, 0);
    lv_obj_set_style_bg_color(s_menu, lv_color_make(10, 8, 12), 0);
    lv_obj_set_style_bg_opa(s_menu, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(s_menu, lv_color_make(196, 168, 104), 0);
    lv_obj_set_style_border_width(s_menu, 2, 0);
    lv_obj_set_style_pad_all(s_menu, 10, 0);
    lv_obj_set_style_clip_corner(s_menu, true, 0);
    lv_obj_remove_flag(s_menu, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_hidden(s_menu, true);
    lv_obj_set_scrollable(s_menu, false);

    s_menu_title = lv_label_create(s_menu);
    lv_obj_set_style_text_color(s_menu_title, lv_color_make(255, 244, 220), 0);
    lv_obj_set_style_text_font(s_menu_title, &lv_font_montserrat_16, 0);
    lv_obj_align(s_menu_title, LV_ALIGN_TOP_MID, 0, 0);

    s_menu_body = lv_label_create(s_menu);
    lv_obj_set_style_text_color(s_menu_body, lv_color_make(230, 220, 196), 0);
    lv_obj_set_style_text_font(s_menu_body, &lv_font_montserrat_14, 0);
    lv_obj_set_width(s_menu_body, 300);
    lv_label_set_long_mode(s_menu_body, LV_LABEL_LONG_WRAP);
    lv_obj_align(s_menu_body, LV_ALIGN_TOP_MID, 0, 28);

    s_sheet_tab[0] = make_btn(s_menu, "stats", on_sheet_tab, (void *)(intptr_t)0);
    lv_obj_set_size(s_sheet_tab[0], 140, 30);
    lv_obj_align(s_sheet_tab[0], LV_ALIGN_TOP_MID, -76, 22);
    lv_obj_set_hidden(s_sheet_tab[0], true);
    s_sheet_tab[1] = make_btn(s_menu, "worn", on_sheet_tab, (void *)(intptr_t)1);
    lv_obj_set_size(s_sheet_tab[1], 140, 30);
    lv_obj_align(s_sheet_tab[1], LV_ALIGN_TOP_MID, 76, 22);
    lv_obj_set_hidden(s_sheet_tab[1], true);

    s_sheet = lv_obj_create(s_menu);
    lv_obj_set_size(s_sheet, 310, 164);
    lv_obj_align(s_sheet, LV_ALIGN_TOP_MID, 0, 56);
    lv_obj_set_style_bg_opa(s_sheet, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_sheet, 0, 0);
    lv_obj_set_style_pad_all(s_sheet, 0, 0);
    lv_obj_set_scrollable(s_sheet, true);
    lv_obj_set_scroll_dir(s_sheet, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(s_sheet, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_anim_duration(s_sheet, 0, LV_PART_MAIN);
    lv_obj_remove_flag(s_sheet, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_clickable(s_sheet, true);
    lv_obj_set_hidden(s_sheet, true);

    lv_obj_t *xp_lab = lv_label_create(s_sheet);
    lv_label_set_text(xp_lab, "xp");
    lv_obj_set_style_text_color(xp_lab, lv_color_make(168, 148, 118), 0);
    lv_obj_set_style_text_font(xp_lab, &lv_font_montserrat_14, 0);
    lv_obj_align(xp_lab, LV_ALIGN_TOP_LEFT, 0, 2);

    s_sheet_xp = lv_bar_create(s_sheet);
    lv_obj_set_size(s_sheet_xp, 168, 10);
    lv_bar_set_range(s_sheet_xp, 0, 100);
    lv_obj_align(s_sheet_xp, LV_ALIGN_TOP_LEFT, 28, 6);
    lv_obj_set_style_bg_color(s_sheet_xp, lv_color_make(36, 28, 30), 0);
    lv_obj_set_style_bg_opa(s_sheet_xp, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_sheet_xp, 4, 0);
    lv_obj_set_style_bg_color(s_sheet_xp, lv_color_make(220, 180, 70), LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_sheet_xp, 3, LV_PART_INDICATOR);

    s_sheet_xp_num = lv_label_create(s_sheet);
    lv_obj_set_style_text_color(s_sheet_xp_num, lv_color_make(230, 214, 170), 0);
    lv_obj_set_style_text_font(s_sheet_xp_num, &lv_font_montserrat_14, 0);
    lv_obj_align(s_sheet_xp_num, LV_ALIGN_TOP_RIGHT, 0, 2);

    for (int i = 0; i < 2; i++) {
        lv_obj_t *v = lv_obj_create(s_sheet);
        lv_obj_set_size(v, 148, 28);
        lv_obj_align(v, LV_ALIGN_TOP_LEFT, i ? 158 : 0, 24);
        lv_obj_set_style_bg_color(v, lv_color_make(18, 14, 16), 0);
        lv_obj_set_style_bg_opa(v, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(v, 10, 0);
        lv_obj_set_style_border_color(v, lv_color_make(196, 168, 104), 0);
        lv_obj_set_style_border_width(v, 1, 0);
        lv_obj_set_scrollable(v, false);
        lv_obj_set_clickable(v, false);
        lv_obj_t *vl = lv_label_create(v);
        lv_obj_set_style_text_font(vl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(vl, lv_color_make(255, 244, 220), 0);
        lv_obj_center(vl);
        if (i == 0) {
            s_sheet_hp = v;
        } else {
            s_sheet_mp = v;
        }
    }

    for (int i = 0; i < 6; i++) {
        s_sheet_stat[i] = lv_label_create(s_sheet);
        lv_obj_set_style_text_color(s_sheet_stat[i], lv_color_make(255, 244, 220), 0);
        lv_obj_set_style_text_font(s_sheet_stat[i], &lv_font_montserrat_16, 0);
        lv_obj_align(s_sheet_stat[i], LV_ALIGN_TOP_LEFT, (i < 3) ? 8 : 168, 60 + (i % 3) * 22);
    }

    s_sheet_plot = lv_label_create(s_sheet);
    lv_obj_set_style_text_color(s_sheet_plot, lv_color_make(180, 176, 210), 0);
    lv_obj_set_style_text_font(s_sheet_plot, &lv_font_montserrat_14, 0);
    lv_obj_align(s_sheet_plot, LV_ALIGN_TOP_LEFT, 0, 130);

    s_sheet_plot_bar = lv_bar_create(s_sheet);
    lv_obj_set_size(s_sheet_plot_bar, 120, 8);
    lv_bar_set_range(s_sheet_plot_bar, 0, 100);
    lv_obj_align(s_sheet_plot_bar, LV_ALIGN_TOP_RIGHT, 0, 134);
    lv_obj_set_style_bg_color(s_sheet_plot_bar, lv_color_make(36, 28, 30), 0);
    lv_obj_set_style_bg_opa(s_sheet_plot_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(s_sheet_plot_bar, lv_color_make(140, 130, 190), LV_PART_INDICATOR);

    s_sheet_quest = lv_label_create(s_sheet);
    lv_obj_set_style_text_color(s_sheet_quest, lv_color_make(170, 190, 150), 0);
    lv_obj_set_style_text_font(s_sheet_quest, &lv_font_montserrat_14, 0);
    lv_obj_set_width(s_sheet_quest, 300);
    lv_label_set_long_mode(s_sheet_quest, LV_LABEL_LONG_DOT);
    lv_obj_align(s_sheet_quest, LV_ALIGN_TOP_LEFT, 0, 150);

    s_sheet_spells = lv_label_create(s_sheet);
    lv_obj_set_style_text_color(s_sheet_spells, lv_color_make(190, 176, 210), 0);
    lv_obj_set_style_text_font(s_sheet_spells, &lv_font_montserrat_14, 0);
    lv_obj_set_width(s_sheet_spells, 300);
    lv_label_set_long_mode(s_sheet_spells, LV_LABEL_LONG_WRAP);
    lv_obj_align(s_sheet_spells, LV_ALIGN_TOP_LEFT, 0, 172);

    s_gear = lv_obj_create(s_menu);
    lv_obj_set_size(s_gear, 310, 164);
    lv_obj_align(s_gear, LV_ALIGN_TOP_MID, 0, 56);
    lv_obj_set_style_bg_opa(s_gear, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_gear, 0, 0);
    lv_obj_set_style_pad_all(s_gear, 0, 0);
    lv_obj_set_scrollable(s_gear, true);
    lv_obj_set_scroll_dir(s_gear, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(s_gear, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_anim_duration(s_gear, 0, LV_PART_MAIN);
    lv_obj_remove_flag(s_gear, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_clickable(s_gear, true);
    lv_obj_set_hidden(s_gear, true);
    for (int s = 0; s < TAMI_SLOT_COUNT; s++) {
        int y = 2 + s * 18;
        s_sheet_slot[s] = lv_label_create(s_gear);
        lv_obj_set_style_text_color(s_sheet_slot[s], lv_color_make(160, 150, 130), 0);
        lv_obj_set_style_text_font(s_sheet_slot[s], &lv_font_montserrat_14, 0);
        lv_obj_set_size(s_sheet_slot[s], 96, 16);
        lv_label_set_long_mode(s_sheet_slot[s], LV_LABEL_LONG_CLIP);
        lv_obj_align(s_sheet_slot[s], LV_ALIGN_TOP_LEFT, 0, y);

        s_sheet_gear[s] = lv_label_create(s_gear);
        lv_obj_set_style_text_color(s_sheet_gear[s], lv_color_make(230, 220, 196), 0);
        lv_obj_set_style_text_font(s_sheet_gear[s], &lv_font_montserrat_14, 0);
        lv_obj_set_size(s_sheet_gear[s], 204, 16);
        lv_label_set_long_mode(s_sheet_gear[s], LV_LABEL_LONG_DOT);
        lv_obj_align(s_sheet_gear[s], LV_ALIGN_TOP_LEFT, 100, y);
    }

    s_log = lv_obj_create(s_menu);
    lv_obj_set_size(s_log, 310, 196);
    lv_obj_align(s_log, LV_ALIGN_TOP_MID, 0, 24);
    lv_obj_set_style_bg_opa(s_log, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_log, 0, 0);
    lv_obj_set_style_pad_all(s_log, 0, 0);
    lv_obj_set_scrollable(s_log, true);
    lv_obj_set_scroll_dir(s_log, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(s_log, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_anim_duration(s_log, 0, LV_PART_MAIN);
    lv_obj_remove_flag(s_log, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_clickable(s_log, true);
    lv_obj_set_hidden(s_log, true);
    lv_obj_set_flex_flow(s_log, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(s_log, 12, 0);

    s_log_story = lv_label_create(s_log);
    lv_obj_set_style_text_color(s_log_story, lv_color_make(230, 220, 196), 0);
    lv_obj_set_style_text_font(s_log_story, &lv_font_montserrat_14, 0);
    lv_obj_set_width(s_log_story, 300);
    lv_label_set_long_mode(s_log_story, LV_LABEL_LONG_WRAP);

    s_log_done = lv_label_create(s_log);
    lv_obj_set_style_text_color(s_log_done, lv_color_make(200, 190, 160), 0);
    lv_obj_set_style_text_font(s_log_done, &lv_font_montserrat_14, 0);
    lv_obj_set_width(s_log_done, 300);
    lv_label_set_long_mode(s_log_done, LV_LABEL_LONG_WRAP);

    for (int i = 0; i < 4; i++) {
        s_care_btn[i] = make_btn(s_menu, k_care[i], on_care, (void *)(intptr_t)i);
        lv_obj_set_size(s_care_btn[i], 148, 44);
        lv_obj_align(s_care_btn[i], LV_ALIGN_BOTTOM_MID, (i % 2) ? 78 : -78, (i / 2) ? -20 : -72);
        lv_obj_set_hidden(s_care_btn[i], true);
    }
    for (int i = 0; i < TAMI_FIELD_COUNT; i++) {
        s_field_btn[i] = make_btn(s_menu, tami_field_name((TamiField)i), on_field, (void *)(intptr_t)i);
        lv_obj_set_size(s_field_btn[i], 294, 36);
        lv_obj_align(s_field_btn[i], LV_ALIGN_TOP_MID, 0, 36 + i * 38);
        lv_obj_set_hidden(s_field_btn[i], true);
    }
    for (int i = 0; i < 4; i++) {
        s_pet_btn[i] = make_btn(s_menu, k_pet_act[i], on_pet_act, (void *)(intptr_t)i);
        lv_obj_set_size(s_pet_btn[i], 148, 44);
        lv_obj_align(s_pet_btn[i], LV_ALIGN_BOTTOM_MID, (i % 2) ? 78 : -78, (i / 2) ? -20 : -72);
        lv_obj_set_hidden(s_pet_btn[i], true);
    }

    /* Two rows of fat buttons, on the screen (not the clipped disc). */
    static const int nav_x[5] = {-108, 0, 108, -54, 54};
    static const int nav_y[5] = {292, 292, 292, 348, 348};
    for (int i = 0; i < 5; i++) {
        s_nav[i] = make_btn(scr, k_nav[i], on_nav, (void *)(intptr_t)(i + 1));
        lv_obj_set_size(s_nav[i], 100, 48);
        lv_obj_align(s_nav[i], LV_ALIGN_TOP_MID, nav_x[i], nav_y[i]);
        lv_obj_set_hidden(s_nav[i], true);
    }
    s_burger = make_btn(scr, "menu", on_burger, NULL);
    lv_obj_set_size(s_burger, 96, 40);
    lv_obj_align(s_burger, LV_ALIGN_TOP_MID, 0, 404);
    lv_obj_set_hidden(s_burger, true);

    static const int hp_x[4] = {-59, 59, -59, 59};
    static const int hp_y[4] = {210, 210, 252, 252};
    for (int i = 0; i < 4; i++) {
        s_hatch_p[i] = make_btn(scr, k_people[i], on_hatch_people, (void *)(intptr_t)i);
        lv_obj_set_size(s_hatch_p[i], 110, 40);
        lv_obj_align(s_hatch_p[i], LV_ALIGN_TOP_MID, hp_x[i], hp_y[i]);
    }
    static const int hc_x[4] = {-59, 59, -59, 59};
    static const int hc_y[4] = {298, 298, 340, 340};
    for (int i = 0; i < 4; i++) {
        s_hatch_c[i] = make_btn(scr, k_call[i], on_hatch_call, (void *)(intptr_t)i);
        lv_obj_set_size(s_hatch_c[i], 110, 40);
        lv_obj_align(s_hatch_c[i], LV_ALIGN_TOP_MID, hc_x[i], hc_y[i]);
    }
    s_hatch_go = make_btn(scr, "hatch", on_hatch_go, NULL);
    lv_obj_set_size(s_hatch_go, 168, 42);
    lv_obj_align(s_hatch_go, LV_ALIGN_TOP_MID, 0, 386);
    s_wake = make_btn(scr, "wake", on_wake, NULL);
    lv_obj_set_size(s_wake, 220, 52);
    lv_obj_align(s_wake, LV_ALIGN_TOP_MID, 0, 330);
    lv_obj_set_hidden(s_wake, true);
    s_hatch_back = make_btn(scr, "list", on_hatch_back, NULL);
    lv_obj_set_size(s_hatch_back, 100, 36);
    lv_obj_align(s_hatch_back, LV_ALIGN_TOP_MID, 0, 168);
    lv_obj_set_hidden(s_hatch_back, true);

    for (int i = 0; i < TAMI_ROSTER_SLOTS; i++) {
        s_roster_btn[i] = make_btn(scr, "+  new", on_roster_slot, (void *)(intptr_t)i);
        lv_obj_set_size(s_roster_btn[i], 280, 52);
        lv_obj_align(s_roster_btn[i], LV_ALIGN_TOP_MID, 0, 92 + i * 58);
        lv_obj_set_hidden(s_roster_btn[i], true);
    }

    s_anim_tm = lv_timer_create(on_anim_timer, 80, NULL);
    set_hatch_ui(1);
    hatch_preview();
}

void tami_ui_refresh(const TamiAdventurer *adv) {
    if (s_view == VIEW_ROSTER) {
        roster_preview();
        return;
    }
    if (!s_hatched) {
        hatch_preview();
        return;
    }
    if (s_toast_left > 0) {
        s_toast_left--;
        if (s_toast_left <= 0) {
            lv_obj_set_hidden(s_toast, true);
        }
    }
    char line[96];
    snprintf(line, sizeof(line), "lv %u  %d%%  %s", adv->level, tami_xp_pct(adv),
             tami_field_name(adv->field));
    set_text_if(s_meta, line);

    /* Menus skip fight-pose art so scroll stays snappy, but sheet/log/bag
     * still rewrite in place as kills land. */
    if (s_view == VIEW_SHEET) {
        update_sheet(adv);
        return;
    }
    if (s_view == VIEW_LOG) {
        update_log(adv);
        return;
    }
    if (s_view == VIEW_BAG) {
        char body[320];
        fill_bag(body, sizeof(body), adv);
        set_text_if(s_menu_body, body);
        return;
    }
    if (s_view == VIEW_CARE) {
        char body[320];
        fill_care(body, sizeof(body), adv);
        set_text_if(s_menu_body, body);
        return;
    }
    if (s_view != VIEW_HOME) {
        return;
    }
    lv_obj_set_style_bg_color(s_disc, field_color(adv->field), 0);
    lv_obj_set_style_bg_color(s_blob, people_color(adv->people), 0);

    snprintf(line, sizeof(line), "%s  %s %s", adv->given_name, adv->people_name, adv->calling_name);
    set_text_if(s_name, line);

    lv_label_set_text(s_task, adv->banner_left && adv->banner[0] ? adv->banner : adv->task.label);
    lv_bar_set_value(s_bar, tami_task_pct(adv), LV_ANIM_OFF);
    lv_bar_set_value(s_xp_bar, tami_xp_pct(adv), LV_ANIM_OFF);
    lv_arc_set_value(s_xp_arc, tami_xp_pct(adv));
    apply_art(adv);
    apply_nav_hud();
}
