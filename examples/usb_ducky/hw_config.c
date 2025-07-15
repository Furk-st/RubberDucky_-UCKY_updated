#include "hw_config.h"

// 1. Define the SPI interface
static spi_t spi0_config = {
    .hw_inst = spi0,
    .miso_gpio = 4,
    .mosi_gpio = 3,
    .sck_gpio = 2,
    .baud_rate = 12500000,          // 12.5 MHz
    .spi_mode = 0,                  // SPI mode 0 (CPOL=0, CPHA=0)
    .no_miso_gpio_pull_up = false,
    .set_drive_strength = true,
    .mosi_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
    .sck_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
    .use_static_dma_channels = false,
};

// 2. Define the SD-SPI interface
static sd_spi_if_t spi_if = {
    .spi = &spi0_config,
    .ss_gpio = 5,
    .set_drive_strength = true,
    .ss_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
};

// 3. Define the SD card configuration
static sd_card_t sd_card = {
    .type = SD_IF_SPI,
    .spi_if_p = &spi_if,
    .use_card_detect = false,
};

// 4. Required driver interface functions
size_t sd_get_num(void) {
    return 1;
}

sd_card_t *sd_get_by_num(size_t num) {
    return num == 0 ? &sd_card : NULL;
}
