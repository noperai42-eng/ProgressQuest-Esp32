#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "driver/usb_serial_jtag.h"
#include "driver/gpio.h"
#include "esp_io_expander.h"

#include "bsp/esp-bsp.h"
#include "lvgl.h"

#include "tami/sim.h"
#include "persist.h"
#include "ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static const char *TAG = "tami";

enum {
    CMD_RATE = 1,
    CMD_AUTO,
    CMD_TICK,
    CMD_HOURS,
    CMD_FEED,
    CMD_PEP,
    CMD_CAMP,
    CMD_BANDAGE,
    CMD_STATUS,
    CMD_FIELD,
    CMD_HATCH,
    CMD_STORY,
    CMD_LOG,
    CMD_WIPE,
    CMD_ROSTER,
    CMD_WAKE,
    CMD_WHELP,
    CMD_HELP
};

typedef struct {
    int kind;
    int n;
    char arg[24];
} tami_cmd_t;

static TamiAdventurer s_adv;
static TamiRng s_rng;
static int64_t s_sim_now = 1000000;
static QueueHandle_t s_cmds;
static int s_rate_user = -1; /* -1 = auto: 20× on USB, 1× unplugged */
static esp_io_expander_handle_t s_ioexp;
static int s_boot_down;
static int s_pwr_down;

static void strip(char *s) {
    size_t n = strlen(s);
    while (n && (s[n - 1] == '\n' || s[n - 1] == '\r' || s[n - 1] == ' ')) {
        s[--n] = 0;
    }
}

static int cmd_kind(const char *w) {
    if (!strcmp(w, "rate")) {
        return CMD_RATE;
    }
    if (!strcmp(w, "auto")) {
        return CMD_AUTO;
    }
    if (!strcmp(w, "tick")) {
        return CMD_TICK;
    }
    if (!strcmp(w, "hours") || !strcmp(w, "hour")) {
        return CMD_HOURS;
    }
    if (!strcmp(w, "feed")) {
        return CMD_FEED;
    }
    if (!strcmp(w, "pep")) {
        return CMD_PEP;
    }
    if (!strcmp(w, "camp")) {
        return CMD_CAMP;
    }
    if (!strcmp(w, "bandage")) {
        return CMD_BANDAGE;
    }
    if (!strcmp(w, "status") || !strcmp(w, "sheet") || !strcmp(w, "?")) {
        return CMD_STATUS;
    }
    if (!strcmp(w, "field") || !strcmp(w, "map")) {
        return CMD_FIELD;
    }
    if (!strcmp(w, "help")) {
        return CMD_HELP;
    }
    if (!strcmp(w, "hatch")) {
        return CMD_HATCH;
    }
    if (!strcmp(w, "story") || !strcmp(w, "recap")) {
        return CMD_STORY;
    }
    if (!strcmp(w, "log") || !strcmp(w, "quests")) {
        return CMD_LOG;
    }
    if (!strcmp(w, "wipe") || !strcmp(w, "forget")) {
        return CMD_WIPE;
    }
    if (!strcmp(w, "roster") || !strcmp(w, "chars") || !strcmp(w, "list")) {
        return CMD_ROSTER;
    }
    if (!strcmp(w, "wake") || !strcmp(w, "play")) {
        return CMD_WAKE;
    }
    if (!strcmp(w, "whelp")) {
        return CMD_WHELP;
    }
    return 0;
}

static void print_help(void) {
    printf("tami usb: rate N | auto | tick N | hours N | status | story | log | feed | pep | camp | "
           "bandage | field NAME | hatch PEOPLE CALLING | roster | wake N | whelp | wipe\n");
}

static void print_status(int rate) {
    if (!tami_ui_hatched()) {
        printf("awaiting hatch  |  %dx\n", rate);
        return;
    }
    char line[192];
    tami_format_status(&s_adv, line, sizeof(line));
    printf("%s  |  %dx\n", line, rate);
}

static void console_task(void *arg) {
    (void)arg;
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    vTaskDelay(pdMS_TO_TICKS(400));
    print_help();
    char line[96];
    for (;;) {
        if (!fgets(line, sizeof(line), stdin)) {
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }
        strip(line);
        if (!line[0]) {
            continue;
        }
        tami_cmd_t c;
        memset(&c, 0, sizeof(c));
        char word[24];
        char rest[48];
        rest[0] = 0;
        int n = sscanf(line, "%23s %47[^\n]", word, rest);
        if (n < 1) {
            continue;
        }
        c.kind = cmd_kind(word);
        if (c.kind == 0) {
            printf("unknown: %s\n", word);
            print_help();
            continue;
        }
        if (c.kind == CMD_HELP) {
            print_help();
            continue;
        }
        if (n >= 2) {
            c.n = atoi(rest);
            snprintf(c.arg, sizeof(c.arg), "%s", rest);
        }
        if (xQueueSend(s_cmds, &c, 0) != pdTRUE) {
            printf("busy\n");
        }
    }
}

static int current_rate(void) {
    if (s_rate_user >= 0) {
        return s_rate_user;
    }
    return usb_serial_jtag_is_connected() ? 20 : 1;
}

static TamiReport s_last_rep;
static int s_have_rep;

static void persist_now(void) {
    if (!tami_ui_hatched()) {
        return;
    }
    (void)tami_persist_save(&s_adv, &s_rng);
}

static void apply_sim(int sec) {
    if (sec < 1) {
        return;
    }
    if (sec > 12 * 3600) {
        sec = 12 * 3600;
    }
    s_sim_now += sec;
    tami_catchup(&s_adv, &s_rng, s_sim_now, &s_last_rep);
    s_have_rep = 1;
}

static void flush_ui(void) {
    if (bsp_display_lock(40) != ESP_OK) {
        return;
    }
    tami_ui_refresh(&s_adv);
    if (s_have_rep) {
        tami_ui_on_report(&s_last_rep);
        s_have_rep = 0;
    }
    bsp_display_unlock();
}

static void handle_cmd(const tami_cmd_t *c, int rate) {
    switch (c->kind) {
    case CMD_RATE:
        if (c->n < 1) {
            printf("rate %d (%s)\n", rate, s_rate_user < 0 ? "auto" : "set");
            break;
        }
        if (c->n > 600) {
            s_rate_user = 600;
        } else {
            s_rate_user = c->n;
        }
        printf("rate %d\n", s_rate_user);
        break;
    case CMD_AUTO:
        s_rate_user = -1;
        printf("rate auto -> %d\n", current_rate());
        break;
    case CMD_TICK:
        apply_sim(c->n > 0 ? c->n : 1);
        flush_ui();
        print_status(current_rate());
        break;
    case CMD_HOURS:
        apply_sim((c->n > 0 ? c->n : 1) * 3600);
        flush_ui();
        print_status(current_rate());
        break;
    case CMD_FEED:
        tami_feed(&s_adv);
        apply_sim(1);
        flush_ui();
        printf("fed\n");
        print_status(current_rate());
        break;
    case CMD_PEP:
        tami_pep(&s_adv);
        apply_sim(1);
        flush_ui();
        printf("pep\n");
        print_status(current_rate());
        break;
    case CMD_CAMP:
        tami_force_camp(&s_adv);
        apply_sim(1);
        flush_ui();
        printf("camp\n");
        print_status(current_rate());
        break;
    case CMD_BANDAGE:
        tami_bandage(&s_adv);
        apply_sim(1);
        flush_ui();
        printf("bandaged\n");
        print_status(current_rate());
        break;
    case CMD_STATUS:
        print_status(rate);
        break;
    case CMD_STORY: {
        if (!tami_ui_hatched()) {
            printf("awaiting hatch\n");
            break;
        }
        static char story[512];
        static char recap[400];
        tami_format_story(&s_adv, story, sizeof(story));
        tami_format_recap(&s_adv, recap, sizeof(recap));
        printf("%s\nRECAP %s\n", story, recap);
        break;
    }
    case CMD_LOG: {
        if (!tami_ui_hatched()) {
            printf("awaiting hatch\n");
            break;
        }
        static char recap[400];
        static char log[640];
        tami_format_recap(&s_adv, recap, sizeof(recap));
        tami_format_log(&s_adv, log, sizeof(log));
        printf("RECAP %s\n%s", recap, log);
        break;
    }
    case CMD_HATCH: {
        char pn[24] = {0};
        char cn[24] = {0};
        sscanf(c->arg, "%23s %23s", pn, cn);
        TamiPeople pe = TAMI_PEOPLE_ORC;
        TamiCalling ca = TAMI_CALL_WARRIOR;
        if (!strcmp(pn, "human")) {
            pe = TAMI_PEOPLE_HUMAN;
        } else if (!strcmp(pn, "elf")) {
            pe = TAMI_PEOPLE_ELF;
        } else if (!strcmp(pn, "undead")) {
            pe = TAMI_PEOPLE_UNDEAD;
        }
        if (!strcmp(cn, "ranger")) {
            ca = TAMI_CALL_RANGER;
        } else if (!strcmp(cn, "mage")) {
            ca = TAMI_CALL_MAGE;
        } else if (!strcmp(cn, "rogue")) {
            ca = TAMI_CALL_ROGUE;
        }
        tami_ui_do_hatch(pe, ca);
        flush_ui();
        print_status(current_rate());
        break;
    }
    case CMD_FIELD: {
        int found = 0;
        for (int i = 0; i < TAMI_FIELD_COUNT; i++) {
            if (strcasecmp(c->arg, tami_field_name((TamiField)i)) == 0) {
                if (tami_set_field(&s_adv, (TamiField)i) == 0) {
                    printf("field %s\n", tami_field_name((TamiField)i));
                } else {
                    printf("cannot travel\n");
                }
                found = 1;
                break;
            }
        }
        if (!found) {
            printf("fields: Greenroad Ironpit Moonwood Barrow Ashfen\n");
        }
        apply_sim(1);
        flush_ui();
        print_status(current_rate());
        break;
    }
    case CMD_WIPE:
        tami_persist_wipe();
        tami_ui_unhatch();
        flush_ui();
        printf("wiped\n");
        return;
    case CMD_ROSTER: {
        const TamiRoster *ros = tami_persist_roster();
        printf("roster %d/4\n", ros ? tami_roster_used(ros) : 0);
        if (ros) {
            for (int i = 0; i < TAMI_ROSTER_SLOTS; i++) {
                char lab[64];
                tami_roster_label(ros, i, lab, sizeof(lab));
                printf("  %d%s %s\n", i, ros->active == (uint8_t)i ? "*" : " ", lab);
            }
        }
        return;
    }
    case CMD_WAKE: {
        int slot = c->n;
        if (tami_ui_hatched()) {
            persist_now();
        }
        if (tami_persist_select(slot, &s_adv, &s_rng) != 0) {
            printf("cannot wake %d\n", slot);
            return;
        }
        s_sim_now = s_adv.last_tick_unix;
        if (s_sim_now < 1000000) {
            s_sim_now = 1000000;
        }
        if (bsp_display_lock(40) == ESP_OK) {
            tami_ui_resume();
            bsp_display_unlock();
        }
        print_status(current_rate());
        return;
    }
    case CMD_WHELP:
        if (tami_whelp(&s_adv, &s_rng, s_sim_now) != 0) {
            printf("not faded\n");
            return;
        }
        persist_now();
        if (bsp_display_lock(40) == ESP_OK) {
            tami_ui_resume();
            bsp_display_unlock();
        }
        printf("whelp %s\n", s_adv.given_name);
        print_status(current_rate());
        return;
    default:
        break;
    }
    if (c->kind != CMD_RATE && c->kind != CMD_AUTO && c->kind != CMD_STATUS &&
        c->kind != CMD_STORY && c->kind != CMD_LOG && c->kind != CMD_HELP) {
        persist_now();
    }
}

void app_main(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    tami_rng_seed(&s_rng, (uint32_t)esp_random());
    memset(&s_adv, 0, sizeof(s_adv));
    (void)tami_persist_init();
    int loaded = tami_persist_load(&s_adv, &s_rng) == 0;
    if (loaded) {
        s_sim_now = s_adv.last_tick_unix;
        if (s_sim_now < 1000000) {
            s_sim_now = 1000000;
        }
    }

    lv_display_t *disp = bsp_display_start();
    if (!disp) {
        ESP_LOGE(TAG, "display start failed");
        return;
    }
    bsp_display_lock(-1);
    tami_ui_init(&s_adv, &s_rng, &s_sim_now);
    if (loaded) {
        tami_ui_resume();
    } else if (tami_persist_roster() && tami_roster_used(tami_persist_roster()) > 0) {
        tami_ui_show_roster();
    }
    bsp_display_unlock();

    /* Side-case keys: BOOT = GPIO0 (pressed low), PWR = TCA9554 P4 (pressed high). */
    gpio_config_t boot = {
        .pin_bit_mask = 1ULL << 0,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&boot);
    s_ioexp = bsp_io_expander_init();
    if (s_ioexp) {
        esp_io_expander_set_dir(s_ioexp, IO_EXPANDER_PIN_NUM_4, IO_EXPANDER_INPUT);
    }

    s_cmds = xQueueCreate(8, sizeof(tami_cmd_t));
    xTaskCreate(console_task, "tami_usb", 4096, NULL, 2, NULL);

    int64_t last_status = 0;
    int64_t last_ui = 0;
    int64_t last_save = 0;
    int s_was_hatched = loaded;
    int64_t last_sim = esp_timer_get_time();
    while (1) {
        tami_cmd_t c;
        while (xQueueReceive(s_cmds, &c, 0) == pdTRUE) {
            handle_cmd(&c, current_rate());
        }

        int rate = current_rate();
        if (rate < 1) {
            rate = 1;
        }

        {
            int boot = gpio_get_level(GPIO_NUM_0) == 0;
            int pwr = 0;
            if (s_ioexp) {
                uint32_t lvl = 0;
                if (esp_io_expander_get_level(s_ioexp, IO_EXPANDER_PIN_NUM_4, &lvl) == ESP_OK) {
                    pwr = (lvl & IO_EXPANDER_PIN_NUM_4) != 0;
                }
            }
            int hit = 0;
            if (pwr && !s_pwr_down) {
                hit = 1; /* PWR = up */
            } else if (boot && !s_boot_down) {
                hit = -1; /* BOOT = down */
            }
            s_pwr_down = pwr;
            s_boot_down = boot;
            if (hit && bsp_display_lock(40) == ESP_OK) {
                tami_ui_side_key(hit);
                bsp_display_unlock();
            }
        }

        int64_t now = esp_timer_get_time();
        if (tami_ui_hatched()) {
            if (!s_was_hatched) {
                persist_now();
                last_save = now;
                s_was_hatched = 1;
            }
            int64_t due_us = 1000000 / rate;
            if (due_us < 2000) {
                due_us = 2000;
            }
            int64_t elapsed = now - last_sim;
            if (elapsed >= due_us) {
                int step = (int)(elapsed / due_us);
                if (step < 1) {
                    step = 1;
                }
                if (step > 120) {
                    step = 120;
                }
                apply_sim(step);
                last_sim += (int64_t)step * due_us;
                if (now - last_sim > due_us * 4) {
                    last_sim = now;
                }
            }
        } else {
            s_was_hatched = 0;
            last_sim = now;
        }

        int ui_every_us = (rate <= 1) ? 1000000 : 200000;
        if (tami_ui_menu_open()) {
            ui_every_us = 400000; /* menus: rewrite sheet/log in place, no fight pose */
        }
        if (now - last_ui >= ui_every_us) {
            flush_ui();
            last_ui = now;
        }
        if (tami_ui_hatched() && now - last_save >= 15 * 1000000LL) {
            persist_now();
            last_save = now;
        }
        if (rate > 1 && now - last_status > 2000000) {
            print_status(rate);
            last_status = now;
        }
        /* Keys at 25ms even at 1× so PWR/BOOT scroll isn't waiting on the sim tick. */
        vTaskDelay(pdMS_TO_TICKS(25));
    }
}
