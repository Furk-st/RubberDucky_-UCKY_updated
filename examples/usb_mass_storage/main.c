/*
 * Rubber Ducky + Mass Storage Device Implementation
 * File: main.c
 */

#include <bsp/board.h>
#include <pico/stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <tusb.h>
#include <ctype.h>

// FatFS includes for reading scripts from SD card
#include "ff.h"
#include "diskio.h"

//--------------------------------------------------------------------+
// Configuration
//--------------------------------------------------------------------+
#define SCRIPT_FILE_NAME "payload.txt"
#define MAX_SCRIPT_LINE_LENGTH 256
#define TRIGGER_BUTTON_PIN 15
#define STATUS_LED_PIN PICO_DEFAULT_LED_PIN

//--------------------------------------------------------------------+
// USB HID Keycodes
//--------------------------------------------------------------------+
enum {
    HID_KEY_A = 0x04, HID_KEY_B = 0x05, HID_KEY_C = 0x06, HID_KEY_D = 0x07,
    HID_KEY_E = 0x08, HID_KEY_F = 0x09, HID_KEY_G = 0x0A, HID_KEY_H = 0x0B,
    HID_KEY_I = 0x0C, HID_KEY_J = 0x0D, HID_KEY_K = 0x0E, HID_KEY_L = 0x0F,
    HID_KEY_M = 0x10, HID_KEY_N = 0x11, HID_KEY_O = 0x12, HID_KEY_P = 0x13,
    HID_KEY_Q = 0x14, HID_KEY_R = 0x15, HID_KEY_S = 0x16, HID_KEY_T = 0x17,
    HID_KEY_U = 0x18, HID_KEY_V = 0x19, HID_KEY_W = 0x1A, HID_KEY_X = 0x1B,
    HID_KEY_Y = 0x1C, HID_KEY_Z = 0x1D,
    HID_KEY_1 = 0x1E, HID_KEY_2 = 0x1F, HID_KEY_3 = 0x20, HID_KEY_4 = 0x21,
    HID_KEY_5 = 0x22, HID_KEY_6 = 0x23, HID_KEY_7 = 0x24, HID_KEY_8 = 0x25,
    HID_KEY_9 = 0x26, HID_KEY_0 = 0x27,
    HID_KEY_ENTER = 0x28, HID_KEY_ESCAPE = 0x29, HID_KEY_BACKSPACE = 0x2A,
    HID_KEY_TAB = 0x2B, HID_KEY_SPACE = 0x2C, HID_KEY_MINUS = 0x2D,
    HID_KEY_EQUAL = 0x2E, HID_KEY_BRACKET_LEFT = 0x2F, HID_KEY_BRACKET_RIGHT = 0x30,
    HID_KEY_BACKSLASH = 0x31, HID_KEY_SEMICOLON = 0x33, HID_KEY_APOSTROPHE = 0x34,
    HID_KEY_GRAVE = 0x35, HID_KEY_COMMA = 0x36, HID_KEY_PERIOD = 0x37,
    HID_KEY_SLASH = 0x38, HID_KEY_CAPS_LOCK = 0x39,
    HID_KEY_F1 = 0x3A, HID_KEY_F2 = 0x3B, HID_KEY_F3 = 0x3C, HID_KEY_F4 = 0x3D,
    HID_KEY_F5 = 0x3E, HID_KEY_F6 = 0x3F, HID_KEY_F7 = 0x40, HID_KEY_F8 = 0x41,
    HID_KEY_F9 = 0x42, HID_KEY_F10 = 0x43, HID_KEY_F11 = 0x44, HID_KEY_F12 = 0x45,
    HID_KEY_INSERT = 0x49, HID_KEY_HOME = 0x4A, HID_KEY_PAGE_UP = 0x4B,
    HID_KEY_DELETE = 0x4C, HID_KEY_END = 0x4D, HID_KEY_PAGE_DOWN = 0x4E,
    HID_KEY_ARROW_RIGHT = 0x4F, HID_KEY_ARROW_LEFT = 0x50,
    HID_KEY_ARROW_DOWN = 0x51, HID_KEY_ARROW_UP = 0x52,
};

// Modifier key masks
enum {
    KEYBOARD_MODIFIER_LEFTCTRL = 0x01,
    KEYBOARD_MODIFIER_LEFTSHIFT = 0x02,
    KEYBOARD_MODIFIER_LEFTALT = 0x04,
    KEYBOARD_MODIFIER_LEFTGUI = 0x08,
    KEYBOARD_MODIFIER_RIGHTCTRL = 0x10,
    KEYBOARD_MODIFIER_RIGHTSHIFT = 0x20,
    KEYBOARD_MODIFIER_RIGHTALT = 0x40,
    KEYBOARD_MODIFIER_RIGHTGUI = 0x80,
};

//--------------------------------------------------------------------+
// Global Variables
//--------------------------------------------------------------------+
static bool script_running = false;
static uint32_t last_button_press = 0;

//--------------------------------------------------------------------+
// Helper Functions
//--------------------------------------------------------------------+

void blink_led(uint32_t times, uint32_t delay_ms) {
    for(uint32_t i = 0; i < times; i++) {
        gpio_put(STATUS_LED_PIN, 1);
        sleep_ms(delay_ms);
        gpio_put(STATUS_LED_PIN, 0);
        sleep_ms(delay_ms);
    }
}

// Convert character to HID keycode
uint8_t char_to_hid_keycode(char c) {
    if (c >= 'a' && c <= 'z') return HID_KEY_A + (c - 'a');
    if (c >= 'A' && c <= 'Z') return HID_KEY_A + (c - 'A');
    if (c >= '1' && c <= '9') return HID_KEY_1 + (c - '1');
    
    switch (c) {
        case '0': return HID_KEY_0;
        case ' ': return HID_KEY_SPACE;
        case '\n': return HID_KEY_ENTER;
        case '\t': return HID_KEY_TAB;
        case '-': return HID_KEY_MINUS;
        case '=': return HID_KEY_EQUAL;
        case '[': return HID_KEY_BRACKET_LEFT;
        case ']': return HID_KEY_BRACKET_RIGHT;
        case '\\': return HID_KEY_BACKSLASH;
        case ';': return HID_KEY_SEMICOLON;
        case '\'': return HID_KEY_APOSTROPHE;
        case '`': return HID_KEY_GRAVE;
        case ',': return HID_KEY_COMMA;
        case '.': return HID_KEY_PERIOD;
        case '/': return HID_KEY_SLASH;
        default: return 0;
    }
}

// Check if character needs shift modifier
bool char_needs_shift(char c) {
    return (c >= 'A' && c <= 'Z') || 
           strchr("!@#$%^&*()_+{}|:\"<>?", c) != NULL;
}

// Send a single key press
void send_key(uint8_t modifier, uint8_t keycode) {
    if (!tud_hid_ready()) return;
    
    uint8_t keycode_array[6] = {0};
    keycode_array[0] = keycode;
    
    // Send key press
    tud_hid_keyboard_report(1, modifier, keycode_array);
    sleep_ms(10);
    
    // Send key release  
    tud_hid_keyboard_report(1, 0, NULL);
    sleep_ms(10);
}

// Type a string character by character
void type_string(const char* str) {
    printf("Typing: %s\n", str);
    
    for (int i = 0; str[i] != '\0'; i++) {
        char c = str[i];
        uint8_t keycode = char_to_hid_keycode(c);
        uint8_t modifier = char_needs_shift(c) ? KEYBOARD_MODIFIER_LEFTSHIFT : 0;
        
        if (keycode != 0) {
            send_key(modifier, keycode);
        }
        sleep_ms(20);
    }
}

// Execute special commands (DELAY, GUI, etc.)
bool execute_ducky_command(const char* line) {
    char command[32];
    char param[224];
    
    if (sscanf(line, "%31s %223[^\n]", command, param) < 1) {
        return false;
    }
    
    // Convert command to uppercase
    for (int i = 0; command[i]; i++) {
        command[i] = toupper(command[i]);
    }
    
    printf("Executing: %s %s\n", command, param);
    
    if (strcmp(command, "DELAY") == 0) {
        int delay_ms = atoi(param);
        if (delay_ms > 0) {
            printf("Delaying %d ms\n", delay_ms);
            sleep_ms(delay_ms);
        }
        return true;
    }
    else if (strcmp(command, "STRING") == 0) {
        type_string(param);
        return true;
    }
    else if (strcmp(command, "GUI") == 0 || strcmp(command, "WINDOWS") == 0) {
        if (strlen(param) > 0) {
            uint8_t keycode = char_to_hid_keycode(tolower(param[0]));
            if (keycode != 0) {
                send_key(KEYBOARD_MODIFIER_LEFTGUI, keycode);
            }
        } else {
            send_key(KEYBOARD_MODIFIER_LEFTGUI, 0);
        }
        return true;
    }
    else if (strcmp(command, "CTRL") == 0 || strcmp(command, "CONTROL") == 0) {
        if (strlen(param) > 0) {
            uint8_t keycode = char_to_hid_keycode(tolower(param[0]));
            if (keycode != 0) {
                send_key(KEYBOARD_MODIFIER_LEFTCTRL, keycode);
            }
        }
        return true;
    }
    else if (strcmp(command, "ALT") == 0) {
        if (strlen(param) > 0) {
            uint8_t keycode = char_to_hid_keycode(tolower(param[0]));
            if (keycode != 0) {
                send_key(KEYBOARD_MODIFIER_LEFTALT, keycode);
            }
        }
        return true;
    }
    else if (strcmp(command, "SHIFT") == 0) {
        if (strlen(param) > 0) {
            uint8_t keycode = char_to_hid_keycode(tolower(param[0]));
            if (keycode != 0) {
                send_key(KEYBOARD_MODIFIER_LEFTSHIFT, keycode);
            }
        }
        return true;
    }
    else if (strcmp(command, "ENTER") == 0) {
        send_key(0, HID_KEY_ENTER);
        return true;
    }
    else if (strcmp(command, "TAB") == 0) {
        send_key(0, HID_KEY_TAB);
        return true;
    }
    else if (strcmp(command, "ESCAPE") == 0) {
        send_key(0, HID_KEY_ESCAPE);
        return true;
    }
    else if (strcmp(command, "SPACE") == 0) {
        send_key(0, HID_KEY_SPACE);
        return true;
    }
    else if (strcmp(command, "BACKSPACE") == 0) {
        send_key(0, HID_KEY_BACKSPACE);
        return true;
    }
    else if (strcmp(command, "DELETE") == 0) {
        send_key(0, HID_KEY_DELETE);
        return true;
    }
    else if (strcmp(command, "HOME") == 0) {
        send_key(0, HID_KEY_HOME);
        return true;
    }
    else if (strcmp(command, "END") == 0) {
        send_key(0, HID_KEY_END);
        return true;
    }
    else if (strcmp(command, "UP") == 0 || strcmp(command, "UPARROW") == 0) {
        send_key(0, HID_KEY_ARROW_UP);
        return true;
    }
    else if (strcmp(command, "DOWN") == 0 || strcmp(command, "DOWNARROW") == 0) {
        send_key(0, HID_KEY_ARROW_DOWN);
        return true;
    }
    else if (strcmp(command, "LEFT") == 0 || strcmp(command, "LEFTARROW") == 0) {
        send_key(0, HID_KEY_ARROW_LEFT);
        return true;
    }
    else if (strcmp(command, "RIGHT") == 0 || strcmp(command, "RIGHTARROW") == 0) {
        send_key(0, HID_KEY_ARROW_RIGHT);
        return true;
    }
    
    return false; // Unknown command
}

// Read and execute script from SD card
bool execute_script_from_sd(void) {
    FATFS fs;
    FIL file;
    FRESULT fr;
    char line_buffer[MAX_SCRIPT_LINE_LENGTH];
    
    printf("Reading script from SD card...\n");
    
    // Mount SD card
    fr = f_mount(&fs, "", 1);
    if (fr != FR_OK) {
        printf("Failed to mount SD card: %d\n", fr);
        blink_led(3, 200);
        return false;
    }
    
    // Open script file
    fr = f_open(&file, SCRIPT_FILE_NAME, FA_READ);
    if (fr != FR_OK) {
        printf("Failed to open '%s': %d\n", SCRIPT_FILE_NAME, fr);
        f_unmount("");
        blink_led(4, 200);
        return false;
    }
    
    printf("Script file opened. Executing...\n");
    blink_led(2, 100);
    
    script_running = true;
    
    // Execute script line by line
    while (script_running && f_gets(line_buffer, sizeof(line_buffer), &file)) {
        // Remove newline
        line_buffer[strcspn(line_buffer, "\r\n")] = 0;
        
        // Skip empty lines and comments
        if (strlen(line_buffer) == 0 || line_buffer[0] == '#' || line_buffer[0] == '/') {
            continue;
        }
        
        // Execute command
        if (!execute_ducky_command(line_buffer)) {
            printf("Unknown command: %s\n", line_buffer);
        }
        
        tud_task(); // Keep USB alive
        sleep_ms(50);
    }
    
    // Cleanup
    f_close(&file);
    f_unmount("");
    script_running = false;
    blink_led(1, 500);
    
    printf("Script execution completed.\n");
    return true;
}

//--------------------------------------------------------------------+
// USB HID Callbacks
//--------------------------------------------------------------------+

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    (void) instance; (void) report_id; (void) report_type; (void) buffer; (void) reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    (void) instance; (void) report_id; (void) report_type; (void) buffer; (void) bufsize;
}

//--------------------------------------------------------------------+
// Main Function
//--------------------------------------------------------------------+

int main(void) {
    // Initialize board
    board_init();
    
    printf("\033[2J\033[H"); // Clear screen
    printf("Rubber Ducky + Mass Storage Device\n");
    printf("==================================\n");
    
    // Initialize GPIO
    gpio_init(STATUS_LED_PIN);
    gpio_set_dir(STATUS_LED_PIN, GPIO_OUT);
    gpio_put(STATUS_LED_PIN, 0);
    
    gpio_init(TRIGGER_BUTTON_PIN);
    gpio_set_dir(TRIGGER_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(TRIGGER_BUTTON_PIN);
    
    // Initialize TinyUSB
    tud_init(BOARD_TUD_RHPORT);
    stdio_init_all();
    
    printf("Device initialized. Waiting for USB connection...\n");
    
    // Main loop
    while (true) {
        tud_task();
        
        // Check button press
        if (!gpio_get(TRIGGER_BUTTON_PIN)) {
            uint32_t current_time = to_ms_since_boot(get_absolute_time());
            
            if (current_time - last_button_press > 1000) { // 1 second debounce
                last_button_press = current_time;
                
                if (!script_running && tud_mounted()) {
                    printf("Button pressed! Executing script...\n");
                    execute_script_from_sd();
                } else if (script_running) {
                    printf("Stopping script...\n");
                    script_running = false;
                } else {
                    printf("USB not connected.\n");
                }
            }
        }
        
        // Status LED blink when connected
        if (tud_mounted()) {
            static uint32_t led_timer = 0;
            uint32_t current_time = to_ms_since_boot(get_absolute_time());
            
            if (current_time - led_timer > 1000) {
                led_timer = current_time;
                gpio_put(STATUS_LED_PIN, !gpio_get(STATUS_LED_PIN));
            }
        } else {
            gpio_put(STATUS_LED_PIN, 0);
        }
        
        sleep_ms(1);
    }
    
    return 0;
}
