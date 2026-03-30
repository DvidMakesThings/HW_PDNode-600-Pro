#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

/* Port 0: USB full-speed device */
#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)

/* Device stack */
#define CFG_TUD_ENABLED 1
#define CFG_TUD_ENDPOINT0_SIZE 64

/* CDC only */
#define CFG_TUD_CDC 1
#define CFG_TUD_CDC_RX_BUFSIZE 512
#define CFG_TUD_CDC_TX_BUFSIZE 2048

/* Disable unused classes */
#define CFG_TUD_HID 0
#define CFG_TUD_MSC 0
#define CFG_TUD_MIDI 0
#define CFG_TUD_VENDOR 0
#define CFG_TUD_DFU_RUNTIME 0
#define CFG_TUD_NCM 0
#define CFG_TUD_AUDIO 0
#define CFG_TUD_VIDEO 0

#endif /* TUSB_CONFIG_H */
