// Stub implementations for Silicon Labs platform dependencies on QEMU.
// These satisfy linker references from Micrium OS kernel code.

#include "sl_sleeptimer.h"
#include "sl_status.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Sleeptimer stub — Micrium OS uses per-task sleeptimer timers for OSTimeDly().
// We maintain a small timer table so multiple concurrent timers work correctly
// and each timer fires only after its requested delay (not on every tick).

#define MAX_TIMERS 8

typedef struct {
  sl_sleeptimer_timer_handle_t *handle;
  sl_sleeptimer_timer_callback_t callback;
  void *callback_data;
  uint32_t remaining_ticks;
  bool active;
} timer_entry_t;

static volatile uint32_t s_tick_count;
static timer_entry_t s_timers[MAX_TIMERS];

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
  (void)priority; (void)option_flags;

  // Look for an existing entry for this handle, or a free slot
  int free_slot = -1;
  for (int i = 0; i < MAX_TIMERS; i++) {
    if (s_timers[i].active && s_timers[i].handle == handle) {
      // Restart existing timer
      s_timers[i].callback = callback;
      s_timers[i].callback_data = callback_data;
      s_timers[i].remaining_ticks = timeout;
      return SL_STATUS_OK;
    }
    if (!s_timers[i].active && free_slot == -1) {
      free_slot = i;
    }
  }

  if (free_slot == -1) {
    return (sl_status_t)0x0003;  // no free timer slots
  }

  s_timers[free_slot].handle = handle;
  s_timers[free_slot].callback = callback;
  s_timers[free_slot].callback_data = callback_data;
  s_timers[free_slot].remaining_ticks = timeout;
  s_timers[free_slot].active = true;
  return SL_STATUS_OK;
}

__attribute__((weak))
sl_status_t sl_sleeptimer_stop_timer(sl_sleeptimer_timer_handle_t *handle) {
  for (int i = 0; i < MAX_TIMERS; i++) {
    if (s_timers[i].active && s_timers[i].handle == handle) {
      s_timers[i].active = false;
      return SL_STATUS_OK;
    }
  }
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
// Decrements each active timer and fires its callback on expiration.
void SysTick_Handler(void) {
  s_tick_count++;

  for (int i = 0; i < MAX_TIMERS; i++) {
    if (!s_timers[i].active) {
      continue;
    }
    if (s_timers[i].remaining_ticks > 0) {
      s_timers[i].remaining_ticks--;
    }
    if (s_timers[i].remaining_ticks == 0) {
      s_timers[i].active = false;
      s_timers[i].callback(s_timers[i].handle, s_timers[i].callback_data);
    }
  }
}
