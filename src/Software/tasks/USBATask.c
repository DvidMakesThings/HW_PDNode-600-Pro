/**
 * @file tasks/USBATask.c
 * @brief USB-A port monitoring — PAC1720 current sensing + MCP23017 GPIO control.
 *
 * Polls PAC1720 dual-channel current monitors for current readings and reads
 * MCP23017 @ 0x27 for enable/fault GPIO state every 500 ms.
 *
 * @project PDNode-600 Pro
 * @version 1.0.0
 */

#include "../CONFIG.h"
#include "USBATask.h"
#include "HealthTask.h"
#include "../drivers/MCP23017_driver.h"
#include "../drivers/PAC1720_driver.h"
#include "../drivers/i2c_bus.h"

#define USBA_TAG        "[USBA]"
#define USBA_STACK      1024
#define USBA_POLL_MS    500
#define USBA_VBUS_V     5.0f   /* USB-A is always 5 V */

static usba_telemetry_t  s_telem[USBA_NUM_PORTS];
static SemaphoreHandle_t s_mutex     = NULL;
static volatile bool     s_ready     = false;
static mcp23017_t       *s_mcp_usba  = NULL;

/* Enable ON-time tracking */
static uint32_t s_enable_ts[USBA_NUM_PORTS];

/* Zero-current offsets captured at boot with all ports disabled */
static float s_current_offset[USBA_NUM_PORTS];

/* -------------------------------------------------------------------------- */

static void poll_mcp23017(void) {
    if (!s_mcp_usba) return;

    uint8_t porta = 0;
    mcp_read_reg(s_mcp_usba, MCP23017_GPIOA, &porta);

    uint32_t now_s = to_ms_since_boot(get_absolute_time()) / 1000;

    for (int p = 0; p < USBA_NUM_PORTS; p++) {
        bool fault = !((porta >> (p * 2))     & 1); /* GPA(p*2):   FAULTn, active-low */
        bool en    =   (porta >> (p * 2 + 1)) & 1;  /* GPA(p*2+1): ILIM_EN */

        if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(10))) {
            if (en && !s_telem[p].enabled) {
                s_enable_ts[p] = now_s; /* port just enabled */
            }
            s_telem[p].enabled  = en;
            s_telem[p].fault    = fault;
            s_telem[p].uptime_s = en ? (now_s - s_enable_ts[p]) : 0u;
            xSemaphoreGive(s_mutex);
        }
    }
}

static void poll_pac1720(void) {
    /* PAC1720 #1 (0x4C): ports 0 (ch1) and 1 (ch2) */
    float cur0 = 0.0f, cur1 = 0.0f, cur2 = 0.0f, cur3 = 0.0f;
    PAC1720_ReadCurrent(PAC1720_1_I2C_ADDR, 1, &cur0);
    PAC1720_ReadCurrent(PAC1720_1_I2C_ADDR, 2, &cur1);
    /* PAC1720 #2 (0x4D): ports 2 (ch1) and 3 (ch2) */
    PAC1720_ReadCurrent(PAC1720_2_I2C_ADDR, 1, &cur2);
    PAC1720_ReadCurrent(PAC1720_2_I2C_ADDR, 2, &cur3);

    float currents[USBA_NUM_PORTS] = {cur0, cur1, cur2, cur3};

    for (int p = 0; p < USBA_NUM_PORTS; p++) {
        float cal = currents[p] - s_current_offset[p];
        if (cal < 0.0f) cal = 0.0f;
        if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(10))) {
            s_telem[p].current_a = s_telem[p].enabled ? cal : 0.0f;
            s_telem[p].voltage_v = s_telem[p].enabled ? USBA_VBUS_V : 0.0f;
            s_telem[p].power_w   = s_telem[p].voltage_v * s_telem[p].current_a;
            xSemaphoreGive(s_mutex);
        }
    }
}

/* -------------------------------------------------------------------------- */

static void USBATask_Function(void *arg) {
    (void)arg;
    INFO_PRINT("%s Task started\r\n", USBA_TAG);

    /* Init MCP23017 for USB-A enable/fault */
    s_mcp_usba = mcp_register(MCP23017_I2C_INSTANCE, MCP23017_USBA, -1);
    if (s_mcp_usba) {
        mcp_init(s_mcp_usba);
        /* Port A: even bits (0,2,4,6) = FAULTn inputs; odd bits (1,3,5,7) = ILIM_EN outputs */
        mcp_write_reg(s_mcp_usba, MCP23017_IODIRA, 0x55); /* 0b01010101 */
        mcp_write_reg(s_mcp_usba, MCP23017_IODIRB, 0xFF); /* Port B unused */
        mcp_write_reg(s_mcp_usba, MCP23017_GPPUA,  0x55); /* pull-ups on FAULTn pins */
        mcp_write_reg(s_mcp_usba, MCP23017_OLATA,  0x00); /* all ports OFF */
        INFO_PRINT("%s MCP23017 @ 0x%02X initialised\r\n",
                   USBA_TAG, MCP23017_USBA);
    } else {
        WARNING_PRINT("%s MCP23017 @ 0x%02X not found\r\n",
                      USBA_TAG, MCP23017_USBA);
    }

    /* Init PAC1720 current monitors */
    PAC1720_Init(PAC1720_1_I2C_ADDR);
    PAC1720_Init(PAC1720_2_I2C_ADDR);

    /* Capture zero-current offsets while all ports are disabled.
     * Wait 150 ms (≥9 conversion cycles at 64 Hz) for the ADC to fully settle,
     * then average 8 samples spaced one conversion period apart (~16 ms each). */
#define OFFSET_SAMPLES  8
#define OFFSET_SETTLE_MS 150
#define OFFSET_INTERVAL_MS 16
    vTaskDelay(pdMS_TO_TICKS(OFFSET_SETTLE_MS));
    {
        float acc[USBA_NUM_PORTS] = {0.0f, 0.0f, 0.0f, 0.0f};
        for (int s = 0; s < OFFSET_SAMPLES; s++) {
            float c[USBA_NUM_PORTS] = {0};
            PAC1720_ReadCurrent(PAC1720_1_I2C_ADDR, 1, &c[0]);
            PAC1720_ReadCurrent(PAC1720_1_I2C_ADDR, 2, &c[1]);
            PAC1720_ReadCurrent(PAC1720_2_I2C_ADDR, 1, &c[2]);
            PAC1720_ReadCurrent(PAC1720_2_I2C_ADDR, 2, &c[3]);
            for (int p = 0; p < USBA_NUM_PORTS; p++) acc[p] += c[p];
            vTaskDelay(pdMS_TO_TICKS(OFFSET_INTERVAL_MS));
        }
        for (int p = 0; p < USBA_NUM_PORTS; p++)
            s_current_offset[p] = acc[p] / (float)OFFSET_SAMPLES;
        INFO_PRINT("%s Zero offsets (avg %d): %.3f %.3f %.3f %.3f A\r\n",
                   USBA_TAG, OFFSET_SAMPLES,
                   s_current_offset[0], s_current_offset[1],
                   s_current_offset[2], s_current_offset[3]);
    }
#undef OFFSET_SAMPLES
#undef OFFSET_SETTLE_MS
#undef OFFSET_INTERVAL_MS

    /* Initialise telemetry */
    for (int p = 0; p < USBA_NUM_PORTS; p++) {
        memset(&s_telem[p], 0, sizeof(s_telem[p]));
        s_enable_ts[p] = 0;
    }

    s_ready = true;
    INFO_PRINT("%s Ready\r\n", USBA_TAG);

    for (;;) {
        Health_Heartbeat(HEALTH_ID_USBA);
        poll_mcp23017();
        poll_pac1720();
        vTaskDelay(pdMS_TO_TICKS(USBA_POLL_MS));
    }
}

/* -------------------------------------------------------------------------- */
/*  Public API                                                                */
/* -------------------------------------------------------------------------- */

BaseType_t USBATask_Init(bool enable) {
    if (!enable) return pdPASS;
    s_mutex = xSemaphoreCreateMutex();
    if (!s_mutex) return pdFAIL;
    return xTaskCreate(USBATask_Function, "USBAMon", USBA_STACK,
                       NULL, USBATASK_PRIORITY, NULL);
}

bool USBA_IsReady(void) { return s_ready; }

bool USBA_GetTelemetry(uint8_t port, usba_telemetry_t *out) {
    if (port >= USBA_NUM_PORTS || !out || !s_ready) return false;
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(10))) {
        *out = s_telem[port];
        xSemaphoreGive(s_mutex);
        return true;
    }
    return false;
}

bool USBA_SetEnable(uint8_t port, bool on) {
    if (port >= USBA_NUM_PORTS || !s_mcp_usba) return false;
    return mcp_write_pin(s_mcp_usba, port * 2 + 1, on ? 1 : 0); /* ILIM_EN = GPA(p*2+1) */
}
