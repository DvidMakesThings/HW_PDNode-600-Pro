/**
 * @file tasks/PDCardTask.c
 * @brief USB-C PD Card monitoring task — PLACEHOLDER IMPLEMENTATION.
 *
 * The actual PDCard SPI protocol is not yet implemented. This task:
 *  - Initialises the 8 SPI CS pins (idle high)
 *  - Reads VBUS + PMIC voltages via the CD74HC4067 analog mux + ADC
 *  - Exposes telemetry via PDCard_GetTelemetry()
 *  - Runs a 500 ms polling loop
 *
 * SPI PDCard communication is left as TODO stubs (search FUTURE:).
 *
 * @project PDNode-600 Pro
 * @version 1.0.0 (placeholder)
 */

#include "../CONFIG.h"
#include "PDCardTask.h"
#include "HealthTask.h"

#define PDCARD_TAG      "[PDCARD]"
#define PDCARD_STACK    2048
#define PDCARD_POLL_MS  500

/* SPI CS pins for 8 PDCard slots */
static const uint8_t k_cs_pins[PDCARD_NUM_PORTS] = {
    SPI_CS_PORT0, SPI_CS_PORT1, SPI_CS_PORT2, SPI_CS_PORT3,
    SPI_CS_PORT4, SPI_CS_PORT5, SPI_CS_PORT6, SPI_CS_PORT7
};

/* Analog mux channel mappings (CD74HC4067) */
static const uint8_t k_vbus_mux_ch[PDCARD_NUM_PORTS] = {
    VBUS_VMON_PORT0, VBUS_VMON_PORT1, VBUS_VMON_PORT2, VBUS_VMON_PORT3,
    VBUS_VMON_PORT4, VBUS_VMON_PORT5, VBUS_VMON_PORT6, VBUS_VMON_PORT7
};

static const uint8_t k_pmic_mux_ch[PDCARD_NUM_PORTS] = {
    PMIC_VMON_PORT0, PMIC_VMON_PORT1, PMIC_VMON_PORT2, PMIC_VMON_PORT3,
    PMIC_VMON_PORT4, PMIC_VMON_PORT5, PMIC_VMON_PORT6, PMIC_VMON_PORT7
};

/* ADC reference and scaling */
#define ADC_VREF        3.0f
#define ADC_MAX         4095.0f
/* Resistor divider on VBUS: 100k / 10k → factor 11 */
#define VBUS_DIVIDER    11.0f
/* Resistor divider on PMIC: 1:1 (3V3 rail) */
#define PMIC_DIVIDER    1.0f

static pdcard_telemetry_t s_telem[PDCARD_NUM_PORTS];
static SemaphoreHandle_t  s_mutex  = NULL;
static volatile bool      s_ready  = false;

/* -------------------------------------------------------------------------- */
/*  MUX helpers                                                               */
/* -------------------------------------------------------------------------- */

static void mux_select(uint8_t ch) {
    gpio_put(PIN_MUX_S0, (ch >> 0) & 1);
    gpio_put(PIN_MUX_S1, (ch >> 1) & 1);
    gpio_put(PIN_MUX_S2, (ch >> 2) & 1);
    gpio_put(PIN_MUX_S3, (ch >> 3) & 1);
    sleep_us(10); /* settle */
}

static float adc_read_mux_voltage(uint8_t mux_ch, float divider) {
    mux_select(mux_ch);
    adc_select_input(5); /* ADC5 = PIN_MUX_OUTPUT GPIO45 */
    (void)adc_read();    /* discard first sample */
    uint32_t acc = 0;
    for (int i = 0; i < 8; i++) acc += adc_read();
    float v_adc = (float)(acc / 8) * (ADC_VREF / ADC_MAX);
    return v_adc * divider;
}

/* -------------------------------------------------------------------------- */
/*  SPI PDCard stub                                                           */
/* -------------------------------------------------------------------------- */

/* FUTURE: Send command to PDCard over SPI and read response.
 * Protocol TBD once PDCard hardware is finalised.
 * Placeholder returns false (no device). */
static bool pdcard_spi_ping(uint8_t port) {
    (void)port;
    /* FUTURE: gpio_put(k_cs_pins[port], 0);
     *         spi_write_read_blocking(spi0, cmd, resp, len);
     *         gpio_put(k_cs_pins[port], 1); */
    return false;
}

/* -------------------------------------------------------------------------- */
/*  Task                                                                      */
/* -------------------------------------------------------------------------- */

static void PDCardTask_Function(void *arg) {
    (void)arg;
    INFO_PRINT("%s Task started (placeholder)\r\n", PDCARD_TAG);

    /* Init SPI CS pins — all idle-high */
    spi_init(spi0, 1000 * 1000); /* 1 MHz default */
    spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(SPI_MISO, GPIO_FUNC_SPI);
    gpio_set_function(SPI_CLK,  GPIO_FUNC_SPI);
    gpio_set_function(SPI_MOSI, GPIO_FUNC_SPI);

    for (int p = 0; p < PDCARD_NUM_PORTS; p++) {
        gpio_init(k_cs_pins[p]);
        gpio_set_dir(k_cs_pins[p], GPIO_OUT);
        gpio_put(k_cs_pins[p], 1);
    }

    /* Init MUX select pins */
    gpio_init(PIN_MUX_S0); gpio_set_dir(PIN_MUX_S0, GPIO_OUT);
    gpio_init(PIN_MUX_S1); gpio_set_dir(PIN_MUX_S1, GPIO_OUT);
    gpio_init(PIN_MUX_S2); gpio_set_dir(PIN_MUX_S2, GPIO_OUT);
    gpio_init(PIN_MUX_S3); gpio_set_dir(PIN_MUX_S3, GPIO_OUT);

    /* ADC for mux output */
    adc_gpio_init(PIN_MUX_OUTPUT);

    /* Init telemetry to safe defaults */
    for (int p = 0; p < PDCARD_NUM_PORTS; p++) {
        memset(&s_telem[p], 0, sizeof(s_telem[p]));
        strncpy(s_telem[p].port_state, "Disconnected", sizeof(s_telem[p].port_state));
        strncpy(s_telem[p].contract,   "None",         sizeof(s_telem[p].contract));
    }

    s_ready = true;

    for (;;) {
        Health_Heartbeat(HEALTH_ID_PDCARD);

        for (int p = 0; p < PDCARD_NUM_PORTS; p++) {
            /* Read voltages from analog mux */
            float vbus = adc_read_mux_voltage(k_vbus_mux_ch[p], VBUS_DIVIDER);
            float pmic = adc_read_mux_voltage(k_pmic_mux_ch[p], PMIC_DIVIDER);

            /* Ping PDCard via SPI */
            bool card_present = pdcard_spi_ping(p); /* always false for now */

            /* FUTURE: Read port status registers from PDCard SPI.
             *         Parse PD state, contract, fault flags, etc. */

            pdcard_telemetry_t t;
            t.connected  = card_present;
            t.pd_active  = false;
            t.vbus_v     = vbus;
            t.pmic_v     = pmic;
            t.current_a  = 0.0f; /* FUTURE: read from PDCard current register */
            t.power_w    = t.vbus_v * t.current_a;
            t.uptime_s   = 0;
            strncpy(t.port_state, card_present ? "Ready" : "Disconnected",
                    sizeof(t.port_state));
            strncpy(t.contract, "None", sizeof(t.contract));

            if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(10))) {
                s_telem[p] = t;
                xSemaphoreGive(s_mutex);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(PDCARD_POLL_MS));
    }
}

/* -------------------------------------------------------------------------- */
/*  Public API                                                                */
/* -------------------------------------------------------------------------- */

BaseType_t PDCardTask_Init(bool enable) {
    if (!enable) return pdPASS;
    s_mutex = xSemaphoreCreateMutex();
    if (!s_mutex) return pdFAIL;
    return xTaskCreate(PDCardTask_Function, "PDCard", PDCARD_STACK,
                       NULL, PDCARDTASK_PRIORITY, NULL);
}

bool PDCard_IsReady(void) { return s_ready; }

bool PDCard_GetTelemetry(uint8_t port, pdcard_telemetry_t *out) {
    if (port >= PDCARD_NUM_PORTS || !out || !s_ready) return false;
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(10))) {
        *out = s_telem[port];
        xSemaphoreGive(s_mutex);
        return true;
    }
    return false;
}
