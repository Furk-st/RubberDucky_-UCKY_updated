#include <bsp/board.h>
#include <pico/stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <tusb.h>
#include "ff.h"
#include "f_util.h"
#include "hw_config.h"
#include "sd_card.h"

#define KEY_MOD_LSHIFT 0x02

static char *payload = NULL;
static size_t payload_len = 0;

static uint8_t ascii_to_key(char c, uint8_t *modifier) {
    *modifier = 0;
    if (c >= 'a' && c <= 'z') return (c - 'a') + 0x04;
    if (c >= 'A' && c <= 'Z') { *modifier = KEY_MOD_LSHIFT; return (c - 'A') + 0x04; }
    if (c >= '1' && c <= '9') return (c - '1') + 0x1e;
    if (c == '0') return 0x27;
    if (c == ' ') return 0x2c;
    if (c == '\n') return 0x28;
    return 0; // unsupported
}

    if (!tud_hid_ready()) return;
static void send_char(char c) {
    uint8_t mod; uint8_t key = ascii_to_key(c, &mod);
    uint8_t mod;
    uint8_t key = ascii_to_key(c, &mod);
    if (!tud_hid_ready()) return;
    uint8_t report[6] = {0};
    if (key) {
        report[0] = key;
        tud_hid_keyboard_report(0, mod, report);
        sleep_ms(5);
        tud_hid_keyboard_report(0, 0, NULL);
        sleep_ms(5);
    }
}

static void execute_payload(void) {
    if (!payload) return;
    for (size_t i = 0; i < payload_len; ++i) {
        send_char(payload[i]);
    }
}

static void load_payload(void) {
    sd_init_driver();
    sd_card_t *sd = sd_get_by_num(0);
    const char *drive = sd_get_drive_prefix(sd);
    FRESULT fr = f_mount(&sd->state.fatfs, drive, 1);
    if (fr != FR_OK) return;
    FIL f; UINT br;
