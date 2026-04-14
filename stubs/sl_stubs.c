// Stub implementations for Silicon Labs platform dependencies on QEMU.
// These satisfy linker references from Micrium OS kernel code.

#include "sl_sleeptimer.h"
#include "sl_status.h"

#include <stddef.h>
#include <stdint.h>

// Sleeptimer stubs — OSInit() calls sl_sleeptimer_start_timer() with a tick
// callback. We save it and call it from SysTick_Handler to drive the kernel.

static volatile uint32_t s_tick_count;
static sl_sleeptimer_timer_callback_t s_tick_callback;
static sl_sleeptimer_timer_handle_t *s_tick_handle;
static void *s_tick_callback_data;

__attribute__((weak))
sl_status_t sl_sleeptimer_init(void) {
  return SL_STATUS_OK;
}

__attribute__((weak))
uint32_t sl_sleeptimer_get_timer_frequency(void) {
  return 1000;  // 1kHz (matches OS_CFG_TICK_RATE_HZ)
}

__attribute__((weak))
sl_status_t sl_sleeptimer_start_timer(sl_sleeptimer_timer_handle_t *handle,
                                      uint32_t timeout,
                                      sl_sleeptimer_timer_callback_t callback,
                                      void *callback_data,
                                      uint8_t priority,
                                      uint16_t option_flags) {
  (void)timeout; (void)priority; (void)option_flags;
  // Save the callback — SysTick_Handler will invoke it each tick
  s_tick_handle = handle;
  s_tick_callback = callback;
  s_tick_callback_data = callback_data;
  return SL_STATUS_OK;
}

__attribute__((weak))
sl_status_t sl_sleeptimer_stop_timer(sl_sleeptimer_timer_handle_t *handle) {
  (void)handle;
  return SL_STATUS_OK;
}

__attribute__((weak))
sl_status_t sl_sleeptimer_restart_timer(sl_sleeptimer_timer_handle_t *handle,
                                        uint32_t timeout,
                                        sl_sleeptimer_timer_callback_t callback,
                                        void *callback_data,
                                        uint8_t priority,
                                        uint16_t option_flags) {
  return sl_sleeptimer_start_timer(handle, timeout, callback, callback_data,
                                   priority, option_flags);
}

__attribute__((weak))
uint32_t sl_sleeptimer_get_tick_count(void) {
  return s_tick_count;
}

__attribute__((weak))
uint64_t sl_sleeptimer_get_tick_count64(void) {
  return (uint64_t)s_tick_count;
}

__attribute__((weak))
sl_status_t sl_sleeptimer_tick64_to_ms(uint64_t tick, uint64_t *ms) {
  *ms = tick;  // 1:1 at 1kHz
  return SL_STATUS_OK;
}

__attribute__((weak))
sl_status_t sl_sleeptimer_ms_to_tick(uint16_t time_ms, uint32_t *tick) {
  *tick = time_ms;
  return SL_STATUS_OK;
}

__attribute__((weak))
sl_status_t sl_sleeptimer_ms32_to_tick(uint32_t time_ms, uint32_t *tick) {
  *tick = time_ms;
  return SL_STATUS_OK;
}

// SysTick_Handler — the kernel's tick source on QEMU.
// Calls the sleeptimer callback that OSInit() registered.
void SysTick_Handler(void) {
  s_tick_count++;
  if (s_tick_callback != NULL) {
    s_tick_callback(s_tick_handle, s_tick_callback_data);
  }
}
