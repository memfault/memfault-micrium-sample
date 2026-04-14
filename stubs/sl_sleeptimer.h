// Stub: QEMU example uses SysTick directly, not sleeptimer
#pragma once

#include <stdint.h>

// Micrium OS kernel references this type in OS_TCB (tick timer handle)
typedef struct { uint32_t dummy; } sl_sleeptimer_timer_handle_t;

// Callback type used by Micrium OS
typedef void (*sl_sleeptimer_timer_callback_t)(sl_sleeptimer_timer_handle_t *handle, void *data);

#include "sl_status.h"

// Stub prototypes — Micrium OS's os_core.c calls these for tick management.
// Implemented as weak no-ops in stubs/sl_sleeptimer_stubs.c.
sl_status_t sl_sleeptimer_init(void);
uint32_t sl_sleeptimer_get_timer_frequency(void);
sl_status_t sl_sleeptimer_start_timer(sl_sleeptimer_timer_handle_t *handle,
                                      uint32_t timeout,
                                      sl_sleeptimer_timer_callback_t callback,
                                      void *callback_data,
                                      uint8_t priority,
                                      uint16_t option_flags);
sl_status_t sl_sleeptimer_stop_timer(sl_sleeptimer_timer_handle_t *handle);
sl_status_t sl_sleeptimer_restart_timer(sl_sleeptimer_timer_handle_t *handle,
                                        uint32_t timeout,
                                        sl_sleeptimer_timer_callback_t callback,
                                        void *callback_data,
                                        uint8_t priority,
                                        uint16_t option_flags);
uint32_t sl_sleeptimer_get_tick_count(void);
sl_status_t sl_sleeptimer_tick64_to_ms(uint64_t tick, uint64_t *ms);
sl_status_t sl_sleeptimer_ms_to_tick(uint16_t time_ms, uint32_t *tick);
sl_status_t sl_sleeptimer_ms32_to_tick(uint32_t time_ms, uint32_t *tick);
uint64_t sl_sleeptimer_get_tick_count64(void);
