/*
 * USB Descriptors for Rubber Ducky + Mass Storage Device
 * File: usb_descriptors.c
 */

#include "tusb.h"

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+

#define _PID_MAP(itf, n)  ( (CFG_TUD_##itf) << (n) )
#define USB_PID           (0x4000 | _PID_MAP(CDC, 0) | _PID_MAP(MSC, 1) | _PID_MAP(HID, 2) | \
                           _PID_MAP(MIDI, 3) | _PID_MAP(VENDOR, 4) )

#define USB_VID   0xCafe
#define USB_BCD   0x0200

tusb_desc_device_t const desc_device =
{
  .bLength            = sizeof(tusb_desc_device_t),
  .bDescriptorType    = TUSB_DESC_DEVICE,
  .bcdUSB             = USB_BCD,
  .bDeviceClass       = TUSB_CLASS_MISC,
  .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
  .bDeviceProtocol    = MISC_PROTOCOL_IAD,
  .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
  .idVendor           = USB_VID,
  .idProduct          = USB_PID,
  .bcdDevice          = 0x0100,
  .iManufacturer      = 0x01,
  .iProduct           = 0x02,
  .iSerialNumber      = 0x03,
  .bNumConfigurations = 0x01
};

uint8_t const * tud_descriptor_device_cb(void)
{
  return (uint8_t const *) &desc_device;
}

//--------------------------------------------------------------------+
// HID Report Descriptor - Keyboard
//--------------------------------------------------------------------+

uint8_t const desc_hid_report[] =
{
  TUD_HID_REPORT_DESC_KEYBOARD( HID_REPORT_ID(1) )
};

uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance)
{
  (void) instance;
  return desc_hid_report;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

enum
{
  ITF_NUM_CDC = 0,
  ITF_NUM_CDC_DATA,
  ITF_NUM_MSC,
  ITF_NUM_HID,        // HID Interface
  ITF_NUM_TOTAL
};

// Endpoint assignments
#define EPNUM_CDC_NOTIF   0x81
#define EPNUM_CDC_OUT     0x02
#define EPNUM_CDC_IN      0x82
#define EPNUM_MSC_OUT     0x03
#define EPNUM_MSC_IN      0x83
#define EPNUM_HID         0x84  // HID Endpoint

#define CONFIG_TOTAL_LEN    (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_MSC_DESC_LEN + TUD_HID_DESC_LEN)

// Full speed configuration
uint8_t const desc_fs_configuration[] =
{
  // Config number, interface count, string index, total length, attribute, power in mA
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

  // CDC Interface
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),

  // MSC Interface  
  TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 5, EPNUM_MSC_OUT, EPNUM_MSC_IN, 64),

  // HID Interface
  TUD_HID_DESCRIPTOR(ITF_NUM_HID, 6, HID_ITF_PROTOCOL_KEYBOARD, sizeof(desc_hid_report), EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 5)
};

#if TUD_OPT_HIGH_SPEED
// High speed configuration
uint8_t const desc_hs_configuration[] =
{
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 512),
  TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 5, EPNUM_MSC_OUT, EPNUM_MSC_IN, 512),
  TUD_HID_DESCRIPTOR(ITF_NUM_HID, 6, HID_ITF_PROTOCOL_KEYBOARD, sizeof(desc_hid_report), EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 5)
};

uint8_t desc_other_speed_config[CONFIG_TOTAL_LEN];

tusb_desc_device_qualifier_t const desc_device_qualifier =
{
  .bLength            = sizeof(tusb_desc_device_qualifier_t),
  .bDescriptorType    = TUSB_DESC_DEVICE_QUALIFIER,
  .bcdUSB             = USB_BCD,
  .bDeviceClass       = TUSB_CLASS_MISC,
  .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
  .bDeviceProtocol    = MISC_PROTOCOL_IAD,
  .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
  .bNumConfigurations = 0x01,
  .bReserved          = 0x00
};

uint8_t const* tud_descriptor_device_qualifier_cb(void)
{
  return (uint8_t const*) &desc_device_qualifier;
}

uint8_t const* tud_descriptor_other_speed_configuration_cb(uint8_t index)
{
  (void) index;
  memcpy(desc_other_speed_config,
         (tud_speed_get() == TUSB_SPEED_HIGH) ? desc_fs_configuration : desc_hs_configuration,
         CONFIG_TOTAL_LEN);
  desc_other_speed_config[1] = TUSB_DESC_OTHER_SPEED_CONFIG;
  return desc_other_speed_config;
}
#endif

uint8_t const * tud_descriptor_configuration_cb(uint8_t index)
{
  (void) index;
#if TUD_OPT_HIGH_SPEED
  return (tud_speed_get() == TUSB_SPEED_HIGH) ? desc_hs_configuration : desc_fs_configuration;
#else
  return desc_fs_configuration;
#endif
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

char const* string_desc_arr [] =
{
  (const char[]) { 0x09, 0x04 },
  "TinyUSB",
  "Rubber Ducky + Storage",
  "123456789012",
  "TinyUSB CDC",
  "TinyUSB MSC",
  "TinyUSB HID",
};

static uint16_t _desc_str[32];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
  (void) langid;
  uint8_t chr_count;

  if ( index == 0)
  {
    memcpy(&_desc_str[1], string_desc_arr[0], 2);
    chr_count = 1;
  }
  else
  {
    if ( !(index < sizeof(string_desc_arr)/sizeof(string_desc_arr[0])) ) return NULL;

    const char* str = string_desc_arr[index];
    chr_count = (uint8_t) strlen(str);
    if ( chr_count > 31 ) chr_count = 31;

    for(uint8_t i=0; i<chr_count; i++)
    {
      _desc_str[1+i] = str[i];
    }
  }

  _desc_str[0] = (uint16_t) ((TUSB_DESC_STRING << 8 ) | (2*chr_count + 2));
  return _desc_str;
}
