/**
 * @file tasks/InitTask.c
 * @brief PDNode-600 Pro system bring-up sequencer.
 *
 * Phases:
 *  Phase 1 — Hardware init (GPIO, I2C, SPI, ADC, 5V buck)
 *  Phase 2 — Peripheral probe (EEPROM, MCP23017s, W5500)
 *  Phase 3 — Subsystem bring-up (Logger → Storage → Net → PDCard → USBA → Heartbeat)
 *  Phase 4 — Health task registration and start
 *  Phase 5 — Self-delete
 *
 * @project PDNode-600 Pro
 * @version 1.0.0
 */

#include "InitTask.h"
#include "../CONFIG.h"
#include "../drivers/CAT24C256_driver.h"
#include "../drivers/MCP23017_driver.h"
#include "../drivers/TCA9548_driver.h"
#include "../drivers/ethernet_driver.h"
#include "../drivers/i2c_bus.h"
#include "ConsoleTask.h"
#include "HealthTask.h"
#include "HeartbeatTask.h"
#include "LoggerTask.h"
#include "NetTask.h"
#include "PDCardTask.h"
#include "StorageTask.h"
#include "USBATask.h"

#define INIT_TAG "[INIT]"
#define INIT_STACK 2048
#define INIT_TIMEOUT_MS 8000
#define INIT_POLL_MS 10

/* -------------------------------------------------------------------------- */
/*  Phase 1: Hardware Init                                                    */
/* -------------------------------------------------------------------------- */

static void init_gpio(void) {
    /* 5V switching PSU enable — pull high immediately */
    gpio_init(PIN_5V_BUCK_EN);
    gpio_set_dir(PIN_5V_BUCK_EN, GPIO_OUT);
    gpio_put(PIN_5V_BUCK_EN, 1);

    /* Power-good input */
    gpio_init(PGOOD_SYSPMIC);
    gpio_set_dir(PGOOD_SYSPMIC, GPIO_IN);
    gpio_set_pulls(PGOOD_SYSPMIC, true, false); /* No pull-up on board, but be safe */

    /* MCP23017 reset — active low; release after delay */
    gpio_init(PIN_MCP23017_RST);
    gpio_set_dir(PIN_MCP23017_RST, GPIO_OUT);
    gpio_put(PIN_MCP23017_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(5));
    gpio_put(PIN_MCP23017_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    /* PD card IRQ inputs */
    gpio_init(PD_IRQ_EVENT_A);
    gpio_set_dir(PD_IRQ_EVENT_A, GPIO_IN);
    gpio_pull_up(PD_IRQ_EVENT_A);

    gpio_init(PD_IRQ_EVENT_B);
    gpio_set_dir(PD_IRQ_EVENT_B, GPIO_IN);
    gpio_pull_up(PD_IRQ_EVENT_B);

    gpio_init(PD_IRQ_EVENT_A2);
    gpio_set_dir(PD_IRQ_EVENT_A2, GPIO_IN);
    gpio_pull_up(PD_IRQ_EVENT_A2);

    gpio_init(PD_IRQ_EVENT_B2);
    gpio_set_dir(PD_IRQ_EVENT_B2, GPIO_IN);
    gpio_pull_up(PD_IRQ_EVENT_B2);

    /* PAC1720 alert inputs */
    gpio_init(PAC_ALERT_P01);
    gpio_set_dir(PAC_ALERT_P01, GPIO_IN);
    gpio_pull_up(PAC_ALERT_P01);

    gpio_init(PAC_ALERT_P23);
    gpio_set_dir(PAC_ALERT_P23, GPIO_IN);
    gpio_pull_up(PAC_ALERT_P23);

    INFO_PRINT("%s GPIO configured\r\n", INIT_TAG);
}

static void init_i2c(void) {
    /* I2C0 — EEPROM (AT24C256) */
    i2c_init(EEPROM_I2C_INSTANCE, EEPROM_I2C_BAUDRATE);
    gpio_set_function(PIN_I2C0_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_I2C0_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_I2C0_SDA);
    gpio_pull_up(PIN_I2C0_SCL);

    /* I2C1 — MCP23017s, TCA9548, PAC1720 */
    i2c_init(MCP23017_I2C_INSTANCE, MCP23017_I2C_BAUDRATE);
    gpio_set_function(PIN_I2C1_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_I2C1_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_I2C1_SDA);
    gpio_pull_up(PIN_I2C1_SCL);

    vTaskDelay(pdMS_TO_TICKS(10));
    I2C_BusInit(); /* Start the FIFO-serialised I2C task */
    INFO_PRINT("%s I2C buses initialised\r\n", INIT_TAG);
    vTaskDelay(pdMS_TO_TICKS(100));
}

static void init_spi(void) {
    /* SPI1 — W5500 Ethernet */
    spi_init(ETH_SPI_INSTANCE, ETH_SPI_BAUDRATE);
    spi_set_format(ETH_SPI_INSTANCE, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(PIN_ETH_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_ETH_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_ETH_MISO, GPIO_FUNC_SPI);
    gpio_init(PIN_ETH_CS);
    gpio_set_dir(PIN_ETH_CS, GPIO_OUT);
    gpio_put(PIN_ETH_CS, 1);

    /* W5500 reset */
    gpio_init(PIN_ETH_RST);
    gpio_set_dir(PIN_ETH_RST, GPIO_OUT);
    gpio_put(PIN_ETH_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_put(PIN_ETH_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    INFO_PRINT("%s SPI1 (W5500) initialised\r\n", INIT_TAG);
}

static void init_adc(void) {
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_gpio_init(PIN_ADC_VUSB);
    adc_gpio_init(PIN_ADC_VREF);
    adc_gpio_init(PIN_ADC_24V);
    adc_gpio_init(PIN_ADC_5V_SWITCHING);
    adc_gpio_init(PIN_ADC_5V_LDO);
    adc_gpio_init(PIN_ADC_3V3_SWITCHING);
    adc_gpio_init(PIN_ADC_3V3_LDO);
    adc_gpio_init(PIN_MUX_OUTPUT);
    INFO_PRINT("%s ADC initialised\r\n", INIT_TAG);
}

/* -------------------------------------------------------------------------- */
/*  Phase 2: Peripheral Probing                                               */
/* -------------------------------------------------------------------------- */

static void probe_peripheral(i2c_inst_t *i2c, uint8_t addr, const char *name) {
    uint8_t dummy = 0;
    int ret = i2c_bus_read_timeout_us(i2c, addr, &dummy, 1, false, 2000);
    if (ret >= 0) {
        INFO_PRINT("%s   [OK] %s @ 0x%02X\r\n", INIT_TAG, name, addr);
    } else {
        WARNING_PRINT("%s   [--] %s @ 0x%02X not found\r\n", INIT_TAG, name, addr);
    }
}

static void probe_all(void) {
    INFO_PRINT("%s === Peripheral Probe ===\r\n", INIT_TAG);
    probe_peripheral(EEPROM_I2C_INSTANCE, EEPROM_I2C_ADDR, "EEPROM AT24C256");
#if ENABLE_MCP23017_PORT_0123
    probe_peripheral(MCP23017_I2C_INSTANCE, MCP23017_PORT_01, "MCP23017 Port 0-1");
    probe_peripheral(MCP23017_I2C_INSTANCE, MCP23017_PORT_23, "MCP23017 Port 2-3");
    probe_peripheral(MCP23017_I2C_INSTANCE, MCP23017_PORT_45, "MCP23017 Port 4-5");
    probe_peripheral(MCP23017_I2C_INSTANCE, MCP23017_PORT_67, "MCP23017 Port 6-7");
    probe_peripheral(TCA9548A_I2C_INSTANCE, TCA9548A_I2C_ADDR, "TCA9548A I2C Mux");
    probe_peripheral(PAC1720_I2C_INSTANCE, PAC1720_1_I2C_ADDR, "PAC1720 #1");
    probe_peripheral(PAC1720_I2C_INSTANCE, PAC1720_2_I2C_ADDR, "PAC1720 #2");
#else
    INFO_PRINT("%s   [--] MCP23017 Port 0-7 skipped (SDA/SCL swapped in HW v1.0.0)\r\n", INIT_TAG);
#endif
    probe_peripheral(MCP23017_I2C_INSTANCE, MCP23017_USBA, "MCP23017 USB-A");

    /* W5500 version check deferred to NetTask (requires w5500_hw_init first) */
}

/* -------------------------------------------------------------------------- */
/*  Wait helper                                                               */
/* -------------------------------------------------------------------------- */

#define WAIT_READY(is_ready_fn, name)                                                              \
    do {                                                                                           \
        TickType_t _t0 = xTaskGetTickCount();                                                      \
        while (!(is_ready_fn)() && (xTaskGetTickCount() - _t0) < pdMS_TO_TICKS(INIT_TIMEOUT_MS)) { \
            vTaskDelay(pdMS_TO_TICKS(INIT_POLL_MS));                                               \
        }                                                                                          \
        if ((is_ready_fn)())                                                                       \
            INFO_PRINT("%s " name " ready\r\n", INIT_TAG);                                         \
        else                                                                                       \
            WARNING_PRINT("%s " name " NOT ready (timeout)\r\n", INIT_TAG);                        \
    } while (0)

/* -------------------------------------------------------------------------- */
/*  InitTask                                                                  */
/* -------------------------------------------------------------------------- */

static void InitTask_Function(void *pvParameters) {
    (void)pvParameters;

    /* ===== Phase 0: Logger — captures all subsequent output ===== */
    LoggerTask_Init(true);
    {
        TickType_t t0 = xTaskGetTickCount();
        while (!Logger_IsReady() && (xTaskGetTickCount() - t0) < pdMS_TO_TICKS(2000)) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    log_printf("\r\n");
    INFO_PRINT("========================================\r\n");
    INFO_PRINT("   PDNode-600 Pro  FW %s\r\n", FIRMWARE_VERSION);
    INFO_PRINT("   HW %s\r\n", HARDWARE_VERSION);
    INFO_PRINT("========================================\r\n\r\n");

    /* ===== Phase 1: Hardware Init ===== */
    INFO_PRINT("%s === Phase 1: Hardware Init ===\r\n", INIT_TAG);
    init_gpio();
    init_i2c();
    init_spi();
    init_adc();
    TCA9548_Init();

    /* Allow USB CDC to enumerate before proceeding with peripheral init */
    vTaskDelay(pdMS_TO_TICKS(2000));

    /* Wait for 5V supply to be stable */
    {
        int wait_count = 0;
        while (!gpio_get(PGOOD_SYSPMIC) && wait_count < 20) {
            WARNING_PRINT("%s Waiting for 5V PGOOD...\r\n", INIT_TAG);
            vTaskDelay(pdMS_TO_TICKS(250));
            wait_count++;
        }
        if (gpio_get(PGOOD_SYSPMIC)) {
            INFO_PRINT("%s 5V PGOOD asserted\r\n", INIT_TAG);
        }
    }

    /* ===== Phase 2: Peripheral Probing ===== */
    INFO_PRINT("\r\n%s === Phase 2: Peripheral Probe ===\r\n", INIT_TAG);
    probe_all();

    /* ===== Phase 3: Subsystem Bring-up ===== */
    INFO_PRINT("\r\n%s === Phase 3: Subsystem Bring-up ===\r\n", INIT_TAG);
    INFO_PRINT("%s Enabled: Console=%d Storage=%d Net=%d PDCard=%d USBA=%d HB=%d Health=%d\r\n",
               INIT_TAG, ENABLE_CONSOLE_TASK, ENABLE_STORAGE_TASK, ENABLE_NET_TASK,
               ENABLE_PDCARD_TASK, ENABLE_USBA_TASK, ENABLE_HEARTBEAT_TASK, ENABLE_HEALTH_TASK);

    /* Console — USB-CDC command interface */
    INFO_PRINT("%s Starting ConsoleTask...\r\n", INIT_TAG);
    ConsoleTask_Init(ENABLE_CONSOLE_TASK);
    if (ENABLE_CONSOLE_TASK)
        WAIT_READY(Console_IsReady, "ConsoleTask");

    /* Storage — must come before Net */
    INFO_PRINT("%s Starting StorageTask...\r\n", INIT_TAG);
    StorageTask_Init(ENABLE_STORAGE_TASK);
    if (ENABLE_STORAGE_TASK)
        WAIT_READY(Storage_IsReady, "StorageTask");

    /* Network */
    INFO_PRINT("%s Starting NetTask...\r\n", INIT_TAG);
    NetTask_Init(ENABLE_NET_TASK);
    if (ENABLE_NET_TASK)
        WAIT_READY(Net_IsReady, "NetTask");

    /* PDCard monitoring */
    INFO_PRINT("%s Starting PDCardTask...\r\n", INIT_TAG);
    PDCardTask_Init(ENABLE_PDCARD_TASK);
    if (ENABLE_PDCARD_TASK)
        WAIT_READY(PDCard_IsReady, "PDCardTask");

    /* USB-A monitoring */
    INFO_PRINT("%s Starting USBATask...\r\n", INIT_TAG);
    USBATask_Init(ENABLE_USBA_TASK);
    if (ENABLE_USBA_TASK)
        WAIT_READY(USBA_IsReady, "USBATask");

    /* Heartbeat LED */
    INFO_PRINT("%s Starting HeartbeatTask...\r\n", INIT_TAG);
    HeartbeatTask_Init(ENABLE_HEARTBEAT_TASK);

    /* ===== Phase 4: Health task ===== */
    INFO_PRINT("\r\n%s === Phase 4: Health Monitor ===\r\n", INIT_TAG);

    {
        TaskHandle_t h;
        h = xTaskGetHandle("Logger");
        if (h)
            Health_RegisterTask(HEALTH_ID_LOGGER, h, "Logger");
        if (ENABLE_CONSOLE_TASK) {
            h = xTaskGetHandle("Console");
            if (h)
                Health_RegisterTask(HEALTH_ID_CONSOLE, h, "Console");
        }
        if (ENABLE_STORAGE_TASK) {
            h = xTaskGetHandle("Storage");
            if (h)
                Health_RegisterTask(HEALTH_ID_STORAGE, h, "Storage");
        }
        if (ENABLE_NET_TASK) {
            h = xTaskGetHandle("Net");
            if (h)
                Health_RegisterTask(HEALTH_ID_NET, h, "Net");
        }
        if (ENABLE_PDCARD_TASK) {
            h = xTaskGetHandle("PDCard");
            if (h)
                Health_RegisterTask(HEALTH_ID_PDCARD, h, "PDCard");
        }
        if (ENABLE_USBA_TASK) {
            h = xTaskGetHandle("USBAMon");
            if (h)
                Health_RegisterTask(HEALTH_ID_USBA, h, "USBAMon");
        }
        if (ENABLE_HEARTBEAT_TASK) {
            h = xTaskGetHandle("Heartbeat");
            if (h)
                Health_RegisterTask(HEALTH_ID_HEARTBEAT, h, "Heartbeat");
        }
    }

    if (ENABLE_HEALTH_TASK) {
        HealthTask_Start();
    } else {
        INFO_PRINT("%s HealthTask disabled — watchdog NOT armed\r\n", INIT_TAG);
    }

    /* ===== Phase 5: Done ===== */
    INFO_PRINT("\r\n");
    INFO_PRINT("========================================\r\n");
    INFO_PRINT("           SYSTEM READY                 \r\n");
    INFO_PRINT("========================================\r\n\r\n");

    vTaskDelete(NULL);
}

/* -------------------------------------------------------------------------- */

void InitTask_Create(void) {
    xTaskCreate(InitTask_Function, "InitTask", INIT_STACK, NULL, INITTASK_PRIORITY, NULL);
}
