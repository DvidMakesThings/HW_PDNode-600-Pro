#include "tusb.h"
#include <string.h>

/* -------------------------------------------------------------------------- */
/*  Endpoint assignments                                                       */
/* -------------------------------------------------------------------------- */
#define EP_CDC_NOTIF 0x81u
#define EP_CDC_OUT 0x02u
#define EP_CDC_IN 0x82u

enum { ITF_NUM_CDC = 0, ITF_NUM_CDC_DATA, ITF_NUM_TOTAL };

/* -------------------------------------------------------------------------- */
/*  Device descriptor                                                          */
/* -------------------------------------------------------------------------- */
static const tusb_desc_device_t s_desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200u,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = 0x2E8Au,
    .idProduct = 0x000Au,
    .bcdDevice = 0x0100u,
    .iManufacturer = 1u,
    .iProduct = 2u,
    .iSerialNumber = 3u,
    .bNumConfigurations = 1u,
};

uint8_t const *tud_descriptor_device_cb(void) { return (uint8_t const *)&s_desc_device; }

/* -------------------------------------------------------------------------- */
/*  Configuration descriptor                                                   */
/* -------------------------------------------------------------------------- */
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN)

static const uint8_t s_desc_config[] = {
    TUD_CONFIG_DESCRIPTOR(1u, ITF_NUM_TOTAL, 0u, CONFIG_TOTAL_LEN, 0x00u, 100u),
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4u, EP_CDC_NOTIF, 8u, EP_CDC_OUT, EP_CDC_IN, 64u),
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return s_desc_config;
}

/* -------------------------------------------------------------------------- */
/*  String descriptors                                                         */
/* -------------------------------------------------------------------------- */
static const char *const s_string_desc[] = {
    (const char[]){0x09u, 0x04u}, "PDNode", "PDNode FreeRTOS CDC", "000001", "PDNode CDC Port",
};

static uint16_t s_desc_str[32];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;

    uint8_t chr_count;

    if (index == 0u) {
        memcpy(&s_desc_str[1], s_string_desc[0], 2u);
        chr_count = 1u;
    } else {
        if (index >= (uint8_t)(sizeof(s_string_desc) / sizeof(s_string_desc[0]))) {
            return NULL;
        }

        const char *str = s_string_desc[index];
        chr_count = (uint8_t)strlen(str);

        if (chr_count > 31u) {
            chr_count = 31u;
        }

        for (uint8_t i = 0u; i < chr_count; i++) {
            s_desc_str[1u + i] = (uint16_t)str[i];
        }
    }

    s_desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8u) | (2u * chr_count + 2u));
    return s_desc_str;
}