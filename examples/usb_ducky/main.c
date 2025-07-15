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

// Minimal ASCII to HID keycode translation used for the sample payload.
// Only a small subset of characters are supported. Unsupported
// characters return 0 which will be ignored by send_char().
static uint8_t ascii_to_key(char c, uint8_t *modifier) {
    *modifier = 0;
    if (c >= 'a' && c <= 'z') return (c - 'a') + 0x04;
    if (c >= 'A' && c <= 'Z') { *modifier = KEY_MOD_LSHIFT; return (c - 'A') + 0x04; }
    if (c >= '1' && c <= '9') return (c - '1') + 0x1e;
    if (c == '0') return 0x27;
    if (c == ' ') return 0x2c;
    if (c == '\n') return 0x28;
    return 0; // unsupported

    switch (c) {
        case ' ':  return 0x2c;               // SPACE
        case '\n': return 0x28;               // ENTER
        case '.':  return 0x37;               // PERIOD
        case ',':  return 0x36;               // COMMA
        case '!':  *modifier = KEY_MOD_LSHIFT; return 0x1e; // 1 with shift
        case '?':  *modifier = KEY_MOD_LSHIFT; return 0x38; // '/' with shift
        default:   return 0;                  // unsupported
    }
}

static void send_char(char c) {
    uint8_t mod = 0;
    uint8_t key;
    uint8_t report[6] = {0};

    if (!tud_hid_ready()) return;

    key = ascii_to_key(c, &mod);

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
