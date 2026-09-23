# Touch this file when adding embed/*.img so IDF re-GLOBs.
# Packed families: gnoll boar wight moth pike crow mire_eel ash_rat
#                   wolf spider ogre brigand wyrm hag
# Gear overlays: wep_stick hatchet spear blade helm_cap coif iron shield_round tower
file(GLOB TAMI_ART_EMBED "${CMAKE_CURRENT_LIST_DIR}/embed/*.img")

idf_component_register(
    SRCS "main.c" "ui.c" "art.c" "persist.c"
    INCLUDE_DIRS "."
    REQUIRES tami_sim nvs_flash spiffs esp_timer driver esp_driver_usb_serial_jtag
             waveshare__esp32_s3_touch_amoled_1_75 espressif__esp_io_expander
    EMBED_FILES ${TAMI_ART_EMBED}
)
