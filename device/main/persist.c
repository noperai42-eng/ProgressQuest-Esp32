#include "persist.h"

#include "esp_log.h"
#include "esp_spiffs.h"
#include "nvs.h"

#include <string.h>

static const char *TAG = "tami_nv";
static const char *NS = "tami";
static TamiRoster s_ros;

static int nvs_import(TamiAdventurer *a, TamiRng *rng) {
    nvs_handle_t h;
    if (nvs_open(NS, NVS_READONLY, &h) != ESP_OK) {
        return -1;
    }
    TamiAdventurer tmp;
    size_t sz = sizeof(tmp);
    esp_err_t err = nvs_get_blob(h, "adv", &tmp, &sz);
    uint32_t rs = 0;
    (void)nvs_get_u32(h, "rng", &rs);
    nvs_close(h);
    if (err != ESP_OK || sz != sizeof(tmp) || !tami_blob_sane(&tmp)) {
        return -1;
    }
    *a = tmp;
    if (rng && rs != 0) {
        rng->s = rs;
    }
    return 0;
}

static void nvs_drop_legacy(void) {
    nvs_handle_t h;
    if (nvs_open(NS, NVS_READWRITE, &h) != ESP_OK) {
        return;
    }
    (void)nvs_erase_key(h, "adv");
    (void)nvs_erase_key(h, "rng");
    (void)nvs_commit(h);
    nvs_close(h);
}

int tami_persist_init(void) {
    tami_roster_set_dir("/tami");
    tami_roster_clear(&s_ros);
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/tami",
        .partition_label = "storage",
        .max_files = 8,
        .format_if_mount_failed = true,
    };
    esp_err_t err = esp_vfs_spiffs_register(&conf);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "spiffs %s", esp_err_to_name(err));
        return -1;
    }
    (void)tami_roster_load(&s_ros);
    if (tami_roster_used(&s_ros) == 0) {
        TamiAdventurer legacy;
        TamiRng rng;
        memset(&legacy, 0, sizeof(legacy));
        memset(&rng, 0, sizeof(rng));
        if (nvs_import(&legacy, &rng) == 0) {
            if (tami_roster_write_slot(&s_ros, 0, &legacy, &rng) == 0) {
                nvs_drop_legacy();
                ESP_LOGI(TAG, "imported %s into slot 0", legacy.given_name);
            }
        }
    }
    ESP_LOGI(TAG, "roster %d souls, active %u", tami_roster_used(&s_ros), (unsigned)s_ros.active);
    return 0;
}

const TamiRoster *tami_persist_roster(void) {
    return &s_ros;
}

int tami_persist_active(void) {
    return s_ros.active == 0xFF ? -1 : (int)s_ros.active;
}

int tami_persist_load(TamiAdventurer *a, TamiRng *rng) {
    int slot = tami_persist_active();
    if (slot < 0) {
        return -1;
    }
    return tami_persist_select(slot, a, rng);
}

int tami_persist_select(int slot, TamiAdventurer *a, TamiRng *rng) {
    if (tami_roster_read_slot(slot, a, rng) != 0) {
        return -1;
    }
    s_ros.active = (uint8_t)slot;
    (void)tami_roster_save_meta(&s_ros);
    ESP_LOGI(TAG, "wake %s slot %d", a->given_name, slot);
    return 0;
}

int tami_persist_create(int slot, const TamiAdventurer *a, const TamiRng *rng) {
    return tami_roster_write_slot(&s_ros, slot, a, rng);
}

int tami_persist_save(const TamiAdventurer *a, const TamiRng *rng) {
    int slot = tami_persist_active();
    if (slot < 0) {
        slot = tami_roster_first_empty(&s_ros);
    }
    if (slot < 0) {
        slot = 0;
    }
    return tami_roster_write_slot(&s_ros, slot, a, rng);
}

int tami_persist_wipe(void) {
    int slot = tami_persist_active();
    if (slot < 0) {
        return 0;
    }
    return tami_persist_erase_slot(slot);
}

int tami_persist_erase_slot(int slot) {
    int rc = tami_roster_erase_slot(&s_ros, slot);
    ESP_LOGI(TAG, "erased slot %d, active %u", slot, (unsigned)s_ros.active);
    return rc;
}
