/*
 * CONFIG.h - PDNode-600 Pro Firmware Configuration
 *
 * Pin assignments derived from the BladeCore-M54E hardware schematics (v1.0.0).
 * MCU: RP2354B (QFN-80)
 *
 * This header is the master include for the entire firmware.
 * It includes all necessary SDK, FreeRTOS, and project headers.
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ========================================================================= */
/*  Standard C headers                                                        */
/* ========================================================================= */
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================= */
/*  Pico SDK headers                                                          */
/* ========================================================================= */
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/resets.h"
#include "hardware/spi.h"
#include "hardware/timer.h"
#include "hardware/watchdog.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/unique_id.h"

/* ========================================================================= */
/*  FreeRTOS headers                                                          */
/* ========================================================================= */
#include "FreeRTOS.h"
#include "event_groups.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"

/***************************** Hardware Version *********************************/
/** @name Versioning
 * @ingroup config01
 * @{ */
/**
 * @brief Hardware version string.
 * @details Format: MAJOR.MINOR.PATCH
 */
#define HARDWARE_VERSION "1.0.0"

/**
 * @brief Hardware version as integer literal.
 * @details Encoded as: MAJOR*100 + MINOR*10 + PATCH
 */
#define HARDWARE_VERSION_LITERAL 100

/***************************** Firmware Version *********************************/
/**
 * @brief Firmware version string.
 * @details Format: MAJOR.MINOR.PATCH
 */
#define FIRMWARE_VERSION "1.0.0"

/**
 * @brief Firmware version as integer literal.
 * @details Encoded as: MAJOR*100 + MINOR*10 + PATCH
 */
#define FIRMWARE_VERSION_LITERAL 100
/** @} */

/* -------------------------------------------------------------------------- */
/*  W5500 Ethernet (SPI1)                                                     */
/* -------------------------------------------------------------------------- */
#define PIN_ETH_MISO 28 /* SPI1 RX  - W5500 MISO             */
#define PIN_ETH_CS 29   /* SPI1 CSn - W5500 SCSn             */
#define PIN_ETH_SCK 30  /* SPI1 SCK - W5500 SCLK            */
#define PIN_ETH_MOSI 31 /* SPI1 TX  - W5500 MOSI            */

/* -------------------------------------------------------------------------- */
/*  I2C0 - Onboard EEPROM (AT24C256) + M.2 connector                         */
/* -------------------------------------------------------------------------- */
#define PIN_I2C0_SDA 32 /* I2C0 data  (4.7K pull-up)         */
#define PIN_I2C0_SCL 33 /* I2C0 clock (4.7K pull-up)         */

/* -------------------------------------------------------------------------- */
/*  W5500 Ethernet Control                                                    */
/* -------------------------------------------------------------------------- */
#define PIN_ETH_RST 34 /* W5500 hardware reset              */
#define PIN_ETH_INT 35 /* W5500 interrupt (active low)      */

/* -------------------------------------------------------------------------- */
/*  Onboard Heartbeat LED                                                     */
/* -------------------------------------------------------------------------- */
#define PIN_HEARTBEAT 36 /* Blue LED, 100R series resistor    */

/* -------------------------------------------------------------------------- */
/*  ADC - Onboard                                                             */
/* -------------------------------------------------------------------------- */
#define PIN_ADC_VUSB 46 /* GPIO46/ADC6 - USB VBUS sense (5.1K-5.1K divider) */
#define PIN_ADC_VREF 47 /* GPIO47/ADC7 - 3.00V 0.1% ref (10K-10K divider)   */

/* -------------------------------------------------------------------------- */
/*  Unused GPIOs - M.2 Connector (directly accent through M.2 edge connector) */
/* -------------------------------------------------------------------------- */
#define PD_IRQ_EVENT_A2 0 /* MCP23017T INT-A USB-A Signal */
#define PD_IRQ_EVENT_B2 1 /* MCP23017T INT-B USB-A Signal */
#define PIN_I2C1_SDA 2    /* M.2 pin 53 - I2C1 data  (MCP23017) */
#define PIN_I2C1_SCL 3    /* M.2 pin 51 - I2C1 clock (MCP23017) */
#define SPI_MISO 4        /* SPI0 MISO */
#define SPI_CS 5          /* SPI0 CS */
#define SPI_CLK 6         /* SPI0 CLK */
#define SPI_MOSI 7        /* SPI0 MOSI */
#define SPI_CS_PORT7 8    /* PDCard SPI0 CS7 */
#define SPI_CS_PORT6 9    /* PDCard SPI0 CS6 */
#define SPI_CS_PORT5 10   /* PDCard SPI0 CS5 */
#define SPI_CS_PORT4 11   /* PDCard SPI0 CS4 */
#define SPI_CS_PORT3 12   /* PDCard SPI0 CS3 */
#define SPI_CS_PORT2 13   /* PDCard SPI0 CS2 */
#define SPI_CS_PORT1 14   /* PDCard SPI0 CS1 */
#define SPI_CS_PORT0 15   /* PDCard SPI0 CS0 */
#define PIN_5V_BUCK_EN 16 /* TPS51225CRUKR 5V Buck Enable */
#define PGOOD_SYSPMIC 17  /* TPS51225CRUKR Power Good */
// #define PIN_GPIO18            18      /* M.2 pin 17 - Unused */
#define PAC_ALERT_P23 19    /* PAC1720 Alert Channel 2-3 */
#define PAC_ALERT_P01 20    /* PAC1720 Alert Channel 0-1 */
#define PIN_MUX_S3 21       /* MUX Select 3 */
#define PIN_MUX_S2 22       /* MUX Select 2 */
#define PIN_MUX_S1 23       /* MUX Select 1 */
#define PIN_MUX_S0 24       /* MUX Select 0 */
#define PIN_MCP23017_RST 25 /* M.2 pin 8  - MCP23017 reset MCPAddr: ALL */
#define PD_IRQ_EVENT_A 26   /* MCP23017T INT-A MCPAddr: 0x23 */
#define PD_IRQ_EVENT_B 27   /* MCP23017T INT-B MCPAddr: 0x23 */

/* GPIO37, GPIO38, GPIO39 are NOT connected on BladeCore-M54E */
#ifdef PIN_GPIO37
#error "GPIO37 is not connected on BladeCore-M54E"
#endif
#ifdef PIN_GPIO38
#error "GPIO38 is not connected on BladeCore-M54E"
#endif
#ifdef PIN_GPIO39
#error "GPIO39 is not connected on BladeCore-M54E"
#endif

#define PIN_ADC_24V 40           /* 24V Input Voltage */
#define PIN_ADC_5V_SWITCHING 41  /* 5V Switching PSU */
#define PIN_ADC_5V_LDO 42        /* 5V LDO */
#define PIN_ADC_3V3_SWITCHING 43 /* 3.3V Switching PSU */
#define PIN_ADC_3V3_LDO 44       /* 3.3V LDO */
#define PIN_MUX_OUTPUT 45        /* MUX Output */

/* -------------------------------------------------------------------------- */
/*  SPI1 instance used by W5500                                               */
/* -------------------------------------------------------------------------- */
#define ETH_SPI_INSTANCE spi1
#define ETH_SPI_BAUDRATE (10 * 1000 * 1000) /* 10 MHz               */

/* -------------------------------------------------------------------------- */
/*  I2C0 instance used by EEPROM                                              */
/* -------------------------------------------------------------------------- */
#define EEPROM_I2C_INSTANCE i2c0
#define EEPROM_I2C_ADDR 0x50             /* AT24C256 base address (A0=A1=GND) */
#define EEPROM_I2C_BAUDRATE (400 * 1000) /* 400 kHz             */

/* -------------------------------------------------------------------------- */
/*  I2C1 instance used by MCP23017s                                           */
/* -------------------------------------------------------------------------- */
#define MCP23017_I2C_INSTANCE i2c1
#define MCP23017_I2C_ADDR 0x23             /* A0=1, A1=1, A2=0             */
#define MCP23017_I2C_BAUDRATE (100 * 1000) /* 100 kHz                      */

/* PCB v1.0.0 hardware defect: SDA/SCL traces are swapped on the PDCard MCP23017s
 * (0x20-0x23). Disable probe and init of these devices until hardware is corrected. */
#define ENABLE_MCP23017_PORT_0123 1

#define MCP23017_PORT_01 0x20 /* MCP23017 Port 0-1 (SDA/SCL swapped — do not use) */
#define MCP23017_PORT_23 0x21 /* MCP23017 Port 2-3 (SDA/SCL swapped — do not use) */
#define MCP23017_PORT_45 0x22 /* MCP23017 Port 4-5 (SDA/SCL swapped — do not use) */
#define MCP23017_PORT_67 0x23 /* MCP23017 Port 6-7 (SDA/SCL swapped — do not use) */
#define MCP23017_USBA 0x27    /* MCP23017 USB-A Port (A0=1, A1=1, A2=1) */

/* -------------------------------------------------------------------------- */
/*  MCP23017 register addresses (IOCON.BANK = 0, default)                     */
/* -------------------------------------------------------------------------- */
#define MCP23017_REG_IODIRA 0x00
#define MCP23017_REG_IODIRB 0x01
#define MCP23017_REG_OLATA 0x14
#define MCP23017_REG_OLATB 0x15

/* -------------------------------------------------------------------------- */
/*  Heartbeat LED - PWM configuration                                         */
/* -------------------------------------------------------------------------- */
/*  GPIO36 -> PWM slice 2, channel A (RP2354B: slice = (gpio >> 1) & 0xF)    */
#define HEARTBEAT_PWM_FREQ_HZ 1000
#define HEARTBEAT_FADE_STEP_MS 8

/* -------------------------------------------------------------------------- */
/*  TCA9548ARGER I2C pin assignment                                           */
/* -------------------------------------------------------------------------- */
#define TCA9548A_I2C_INSTANCE i2c1
#define TCA9548A_I2C_ADDR 0x70 /* A0=0, A1=0, A2=0 */
// SDA0, SCL0 = PDCard Port 0
// SDA1, SCL1 = PDCard Port 1
// SDA2, SCL2 = PDCard Port 2
// SDA3, SCL3 = PDCard Port 3
// SDA4, SCL4 = PDCard Port 4
// SDA5, SCL5 = PDCard Port 5
// SDA6, SCL6 = PDCard Port 6
// SDA7, SCL7 = PDCard Port 7

/* -------------------------------------------------------------------------- */
/*  CD74HC4067 analog mux channels                                            */
/* -------------------------------------------------------------------------- */
// Channels set by MUX select pins (PIN_MUX_S3..PIN_MUX_S0) to read the voltages
// ADC channel is ADC5, PIN_MUX_OUTPUT
#define PMIC_VMON_PORT7 0  /* CD74HC4067 channel 0 - PMIC VMON */
#define PMIC_VMON_PORT6 1  /* CD74HC4067 channel 1 - PMIC VMON */
#define PMIC_VMON_PORT5 2  /* CD74HC4067 channel 2 - PMIC VMON */
#define PMIC_VMON_PORT4 3  /* CD74HC4067 channel 3 - PMIC VMON */
#define PMIC_VMON_PORT3 4  /* CD74HC4067 channel 4 - PMIC VMON */
#define PMIC_VMON_PORT2 5  /* CD74HC4067 channel 5 - PMIC VMON */
#define PMIC_VMON_PORT1 6  /* CD74HC4067 channel 6 - PMIC VMON */
#define PMIC_VMON_PORT0 7  /* CD74HC4067 channel 7 - PMIC VMON */
#define VBUS_VMON_PORT0 8  /* CD74HC4067 channel 8  - VBUS VMON */
#define VBUS_VMON_PORT1 9  /* CD74HC4067 channel 9  - VBUS VMON */
#define VBUS_VMON_PORT2 10 /* CD74HC4067 channel 10 - VBUS VMON */
#define VBUS_VMON_PORT3 11 /* CD74HC4067 channel 11 - VBUS VMON */
#define VBUS_VMON_PORT4 12 /* CD74HC4067 channel 12 - VBUS VMON */
#define VBUS_VMON_PORT5 13 /* CD74HC4067 channel 13 - VBUS VMON */
#define VBUS_VMON_PORT6 14 /* CD74HC4067 channel 14 - VBUS VMON */
#define VBUS_VMON_PORT7 15 /* CD74HC4067 channel 15 - VBUS VMON */

/* -------------------------------------------------------------------------- */
/*  PAC1720-1-AIA-TR dual current monitor                                     */
/* -------------------------------------------------------------------------- */
#define PAC1720_I2C_INSTANCE i2c1
#define PAC1720_1_I2C_ADDR 0x4C
#define PAC1720_2_I2C_ADDR 0x18
#define PAC1720_RSENSE_OHM  0.02f   /* 20 mΩ shunt resistors */
// USBA_PORT0 is PAC1720_1 Sense 1
// USBA_PORT1 is PAC1720_1 Sense 2
// USBA_PORT2 is PAC1720_2 Sense 1
// USBA_PORT3 is PAC1720_2 Sense 2

/* ========================================================================= */
/*  W5500 Driver Compatibility Defines                                        */
/*  (ethernet_driver.c / ethernet_config.h use these abstract names)         */
/* ========================================================================= */
#define W5500_SPI_INSTANCE ETH_SPI_INSTANCE
#define SPI_SPEED_W5500 ETH_SPI_BAUDRATE
#define W5500_MOSI PIN_ETH_MOSI
#define W5500_MISO PIN_ETH_MISO
#define W5500_SCK PIN_ETH_SCK
#define W5500_CS PIN_ETH_CS
#define W5500_RESET PIN_ETH_RST
#define W5500_INT PIN_ETH_INT

/* ethernet_driver.c gets the rest of its defines from ethernet_config.h */

/* ========================================================================= */
/*  I2C Bus Compatibility Defines                                             */
/* ========================================================================= */
#define I2C0_SDA PIN_I2C0_SDA
#define I2C0_SCL PIN_I2C0_SCL
#define I2C0_SPEED EEPROM_I2C_BAUDRATE
#define I2C1_SDA PIN_I2C1_SDA
#define I2C1_SCL PIN_I2C1_SCL
#define I2C1_SPEED MCP23017_I2C_BAUDRATE

/* ========================================================================= */
/*  EEPROM Compatibility Defines                                              */
/* ========================================================================= */
#define EEPROM_I2C EEPROM_I2C_INSTANCE
#define EEPROM_SIZE 32768 /* AT24C256 = 32 KB */

/* ========================================================================= */
/*  Network defaults — single source of truth for all tasks and drivers       */
/* ========================================================================= */
#define DEFAULT_IP {192, 168, 0, 42}
#define DEFAULT_SUBNET {255, 255, 255, 0}
#define DEFAULT_GW {192, 168, 0, 1}
#define DEFAULT_DNS {8, 8, 8, 8}
#define DEFAULT_MAC {0x02, 0x00, 0x00, 0x60, 0x04, 0x42}
#define DEFAULT_DHCP 0
#define DEFAULT_DEVICE_NAME "PDNode-600 Pro"
#define DEFAULT_LOCATION "Rack"

#define SNMP_PORT_AGENT 161
#define NET_TASK_CYCLE_MS 5

/* ========================================================================= */
/*  Logger Configuration                                                      */
/* ========================================================================= */
#define LOGGER_MSG_MAX 256
#define LOGGER_QUEUE_LEN 64
#define LOGGER_STACK_SIZE 1024
#define CONSOLE_STACK_SIZE 2048

/* ========================================================================= */
/*  Task enable flags — set to 0 to skip a subsystem during boot debugging.  */
/*  LoggerTask is always ON (it provides the only debug output).              */
/*  Start minimal (Logger + Console only), enable one at a time.             */
/* ========================================================================= */
#define ENABLE_CONSOLE_TASK 1   /* USB-CDC command interface              */
#define ENABLE_STORAGE_TASK 1   /* EEPROM config (required by Net)        */
#define ENABLE_NET_TASK 1       /* W5500 Ethernet + HTTP + SNMP           */
#define ENABLE_PDCARD_TASK 1    /* USB-C PD port monitoring               */
#define ENABLE_USBA_TASK 1      /* USB-A port monitoring                  */
#define ENABLE_HEARTBEAT_TASK 1 /* Heartbeat LED                          */
#define ENABLE_HEALTH_TASK 1    /* Watchdog + per-task liveness monitor   */

/* ========================================================================= */
/*  Error Logger Stub (disabled — use log macros instead)                     */
/* ========================================================================= */
/* ERRORLOGGER is intentionally NOT defined so that #ifdef ERRORLOGGER blocks
 * (in legacy drivers) are compiled out. #if ERRORLOGGER blocks also evaluate
 * to 0 because undefined identifiers expand to 0 in preprocessor arithmetic. */

/* No-op stubs for error-code infrastructure not implemented in this project */
static inline void Storage_EnqueueErrorCode(uint16_t code) { (void)code; }
static inline void Storage_EnqueueWarningCode(uint16_t code) { (void)code; }

/* ERROR_PRINT_CODE — maps legacy code+format calls to standard ERROR_PRINT */
#define ERROR_PRINT_CODE(code, ...) ERROR_PRINT(__VA_ARGS__)

/* Power management stub — PDNode has no STANDBY state at this revision */
#define PWR_STATE_STANDBY 0xFF
static inline int Power_GetState(void) { return 0; }

/* ========================================================================= */
/*  Forward declarations for logger (defined in tasks/LoggerTask.c)           */
/* ========================================================================= */
void log_printf(const char *fmt, ...);
void log_printf_force(const char *fmt, ...);

/* ---------- LOGGING FLAGS ---------- */
/** @name Logging Flags
 * @ingroup config01
 * @{ */
#ifndef DEBUG
#define DEBUG 1
#endif

#ifndef DEBUG_HEALTH
#define DEBUG_HEALTH 0
#endif

#ifndef INFO
#define INFO 1
#endif

#ifndef INFO_HEALTH
#define INFO_HEALTH 0
#endif

#ifndef PLOT_EN
#define PLOT_EN 0
#endif

#ifndef NETLOG
#define NETLOG 0
#endif

#ifndef UART_IFACE
#define UART_IFACE 1

#endif
/** @} */

/* Ensure logging level flags exist even if commented out elsewhere */
/** @name Logging Macros
 * @ingroup config01
 * @{ */
#ifndef ERROR
#define ERROR 1
#endif

#ifndef WARNING
#define WARNING 1
#endif

#if DEBUG
#define DEBUG_PRINT(...) log_printf("\t[DEBUG] " __VA_ARGS__)
#else
#define DEBUG_PRINT(...) ((void)0)
#endif

#if DEBUG_HEALTH
#define DEBUG_PRINT_HEALTH(...) log_printf("\t[HEALTH] " __VA_ARGS__)
#else
#define DEBUG_PRINT_HEALTH(...) ((void)0)
#endif

#if INFO_HEALTH
#define HEALTH_INFO(...) log_printf("[INFO] " __VA_ARGS__)
#else
#define HEALTH_INFO(...) ((void)0)
#endif

#if INFO
#define INFO_PRINT(...) log_printf("[INFO] " __VA_ARGS__)
#else
#define INFO_PRINT(...) ((void)0)
#endif

#if ERROR
#define ERROR_PRINT(...) log_printf_force("[ERROR] " __VA_ARGS__)
#else
#define ERROR_PRINT(...) ((void)0)
#endif

#if ERROR && DEBUG
#define ERROR_PRINT_DEBUG(...) log_printf("[ERROR] " __VA_ARGS__)
#else
#define ERROR_PRINT_DEBUG(...) ((void)0)
#endif

#if WARNING
#define WARNING_PRINT(...) log_printf_force("[WARNING] " __VA_ARGS__)
#else
#define WARNING_PRINT(...) ((void)0)
#endif

#if PLOT_EN
#define PLOT(...) log_printf("\t[PLOT] " __VA_ARGS__)
#else
#define PLOT(...) ((void)0)
#endif

#if NETLOG
#define NETLOG_PRINT(...) log_printf("[NETLOG] " __VA_ARGS__)
#else
#define NETLOG_PRINT(...) ((void)0)
#endif

#if UART_IFACE
#define ECHO(...) log_printf_force(__VA_ARGS__)
#else
#define ECHO(...) ((void)0)
#endif
/** @} */

#endif /* CONFIG_H */
