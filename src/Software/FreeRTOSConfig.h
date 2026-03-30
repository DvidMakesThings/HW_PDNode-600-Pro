/**
 * @file FreeRTOSConfig.h
 * @brief FreeRTOS configuration for PDNode-600 Pro (RP2354B @ 200 MHz)
 *
 * @project PDNode-600 Pro - Managed USB-C PDU
 * @version 1.0.0
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* clang-format off */

/* ISR handlers — use Pico SDK ISR names */
#define vPortSVCHandler     isr_svcall
#define xPortPendSVHandler  isr_pendsv
#define xPortSysTickHandler isr_systick

/* -------------------------------------------------------------------------- */
/*  Task priorities (highest number = highest priority)                       */
/* -------------------------------------------------------------------------- */
#define HEALTHTASK_PRIORITY         (configMAX_PRIORITIES - 1)
#define INITTASK_PRIORITY           (configMAX_PRIORITIES - 2)
#define NETTASK_PRIORITY            (tskIDLE_PRIORITY + 5)
#define STORAGETASK_PRIORITY        (tskIDLE_PRIORITY + 4)
#define PDCARDTASK_PRIORITY         (tskIDLE_PRIORITY + 3)
#define USBATASK_PRIORITY           (tskIDLE_PRIORITY + 3)
#define CONSOLETASK_PRIORITY        (tskIDLE_PRIORITY + 3)
#define LOGTASK_PRIORITY            (tskIDLE_PRIORITY + 2)
#define HEARTBEATTASK_PRIORITY      (tskIDLE_PRIORITY + 1)

/* -------------------------------------------------------------------------- */
/*  Scheduler                                                                 */
/* -------------------------------------------------------------------------- */
#define configUSE_PREEMPTION                    1
#define configUSE_TIME_SLICING                  1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configUSE_TICKLESS_IDLE                 0
#define configCPU_CLOCK_HZ                      200000000UL   /* 200 MHz */
#define configTICK_RATE_HZ                      ((TickType_t)1000)
#define configMAX_PRIORITIES                    24
#define configMINIMAL_STACK_SIZE                ((configSTACK_DEPTH_TYPE)512)
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configMAX_TASK_NAME_LEN                 16
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   3

/* -------------------------------------------------------------------------- */
/*  Synchronization                                                           */
/* -------------------------------------------------------------------------- */
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             0
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_TASK_NOTIFICATIONS            1
#define configUSE_QUEUE_SETS                    0
#define configQUEUE_REGISTRY_SIZE               10
#define configUSE_NEWLIB_REENTRANT              0
#define configENABLE_BACKWARD_COMPATIBILITY     0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 5

/* -------------------------------------------------------------------------- */
/*  System types                                                              */
/* -------------------------------------------------------------------------- */
#define configSTACK_DEPTH_TYPE              uint32_t
#define configMESSAGE_BUFFER_LENGTH_TYPE    size_t

/* -------------------------------------------------------------------------- */
/*  Memory allocation                                                         */
/* -------------------------------------------------------------------------- */
#define configSUPPORT_STATIC_ALLOCATION     0
#define configSUPPORT_DYNAMIC_ALLOCATION    1
#define configAPPLICATION_ALLOCATED_HEAP    0
#define configTOTAL_HEAP_SIZE               (192 * 1024)   /* 192 KB */

/* -------------------------------------------------------------------------- */
/*  Hooks                                                                     */
/* -------------------------------------------------------------------------- */
#define configUSE_IDLE_HOOK                 1
#define configUSE_TICK_HOOK                 0
#define configCHECK_FOR_STACK_OVERFLOW      2
#define configUSE_MALLOC_FAILED_HOOK        1
#define configUSE_DAEMON_TASK_STARTUP_HOOK  0

/* -------------------------------------------------------------------------- */
/*  Runtime stats / trace                                                     */
/* -------------------------------------------------------------------------- */
#define configGENERATE_RUN_TIME_STATS       0
#define configUSE_TRACE_FACILITY            0
#define configUSE_STATS_FORMATTING_FUNCTIONS 0

/* -------------------------------------------------------------------------- */
/*  Co-routines (disabled)                                                    */
/* -------------------------------------------------------------------------- */
#define configUSE_CO_ROUTINES               0
#define configMAX_CO_ROUTINE_PRIORITIES     1

/* -------------------------------------------------------------------------- */
/*  Software timers                                                           */
/* -------------------------------------------------------------------------- */
#define configUSE_TIMERS                    1
#define configTIMER_TASK_PRIORITY           (tskIDLE_PRIORITY + 2)
#define configTIMER_QUEUE_LENGTH            10
#define configTIMER_TASK_STACK_DEPTH        1024

/* -------------------------------------------------------------------------- */
/*  Cortex-M33 (RP2350) required defines                                      */
/* -------------------------------------------------------------------------- */
#define configENABLE_FPU                    1   /* RP2350 has FPU             */
#define configENABLE_MPU                    0   /* MPU not used               */
#define configENABLE_TRUSTZONE              0   /* NTZ variant, no TrustZone  */
#define configRUN_FREERTOS_SECURE_ONLY      1   /* Required for NTZ port      */
#define configNUMBER_OF_CORES               1   /* Single-core; disables SMP spinlock claims */

/* RP2350 uses 4 priority bits → 16 levels (0=highest, 15=lowest).
 * Level 1 (register value 0x10) allows ISRs at priority 1–15 to call fromISR() API. */
#define configPRIO_BITS                     4
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 16

/* -------------------------------------------------------------------------- */
/*  RP2350 / Pico SDK interop                                                 */
/* -------------------------------------------------------------------------- */
#define configSUPPORT_PICO_SYNC_INTEROP     1
#define configSUPPORT_PICO_TIME_INTEROP     1

/* -------------------------------------------------------------------------- */
/*  Assert                                                                    */
/* -------------------------------------------------------------------------- */
void vAssertCalled(const char *file, int line);
#define configASSERT(x)  if ((x) == 0) { vAssertCalled(__FILE__, __LINE__); }

/* -------------------------------------------------------------------------- */
/*  Optional API includes                                                     */
/* -------------------------------------------------------------------------- */
#define INCLUDE_vTaskPrioritySet             1
#define INCLUDE_uxTaskPriorityGet            1
#define INCLUDE_vTaskDelete                  1
#define INCLUDE_vTaskSuspend                 1
#define INCLUDE_vTaskDelayUntil              1
#define INCLUDE_vTaskDelay                   1
#define INCLUDE_xTaskGetSchedulerState       1
#define INCLUDE_xTaskGetCurrentTaskHandle    1
#define INCLUDE_uxTaskGetStackHighWaterMark  1
#define INCLUDE_uxTaskGetStackHighWaterMark2 1
#define INCLUDE_xTaskGetIdleTaskHandle       1
#define INCLUDE_eTaskGetState                1
#define INCLUDE_xTaskAbortDelay              1
#define INCLUDE_xTaskGetHandle               1
#define INCLUDE_xTaskResumeFromISR           1
#define INCLUDE_xQueueGetMutexHolder         1
#define INCLUDE_xResumeFromISR               1
#define INCLUDE_xEventGroupSetBitFromISR     1
#define INCLUDE_xTimerPendFunctionCall       1

/* clang-format on */

#endif /* FREERTOS_CONFIG_H */
