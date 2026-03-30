/**
 * @file tasks/HeartbeatTask.h
 * @brief PWM breathing heartbeat LED on GPIO36.
 *
 * @project PDNode-600 Pro
 */

#pragma once
#include "../CONFIG.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Create and start the heartbeat LED task. */
BaseType_t HeartbeatTask_Init(bool enable);

/** Returns true once the task has been created. */
bool Heartbeat_IsReady(void);

#ifdef __cplusplus
}
#endif
