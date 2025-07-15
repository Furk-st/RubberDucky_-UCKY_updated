/*
 * TinyUSB Configuration for Rubber Ducky + Mass Storage Device
 * File: include/tusb_config.h
 */

#pragma once

#define CFG_TUSB_RHPORT0_MODE   (OPT_MODE_DEVICE)

// CDC (Serial) Configuration
#define CFG_TUD_CDC             (1)
#define CFG_TUD_CDC_RX_BUFSIZE  (256)
#define CFG_TUD_CDC_TX_BUFSIZE  (256)

// MSC (Mass Storage) Configuration
#define CFG_TUD_MSC             (1)
#define CFG_TUD_MSC_EP_BUFSIZE  (4096)

// HID (Human Interface Device) Configuration - YE IMPORTANT HAI!
#define CFG_TUD_HID             (1)
#define CFG_TUD_HID_EP_BUFSIZE  (16)

// Vendor Configuration
#if !PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_MS_OS_20_DESCRIPTOR
#define CFG_TUD_VENDOR            (0)
#else
#define CFG_TUD_VENDOR            (1)
#define CFG_TUD_VENDOR_RX_BUFSIZE  (256)
#define CFG_TUD_VENDOR_TX_BUFSIZE  (256)
#endif

// RHPort number used for device
#ifndef BOARD_TUD_RHPORT
#define BOARD_TUD_RHPORT      0
#endif
