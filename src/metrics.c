//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

#include "metrics.h"

#include <malloc.h>

#include "memfault/components.h"
#include "memfault/metrics/platform/timer.h"
#include "os.h"

#define METRICS_TASK_PRIO 20u
#define METRICS_STK_SIZE 256u

static OS_TCB s_metrics_tcb;
static CPU_STK s_metrics_stk[METRICS_STK_SIZE];

static MemfaultPlatformTimerCallback *s_timer_cb;
static uint32_t s_timer_period_ticks;

static void prv_metrics_task(void *p_arg) {
  (void)p_arg;
  RTOS_ERR err;

  while (1) {
    OSTimeDly(s_timer_period_ticks, OS_OPT_TIME_DLY, &err);
    s_timer_cb();
  }
}

bool memfault_platform_metrics_timer_boot(uint32_t period_sec,
                                          MemfaultPlatformTimerCallback *callback) {
  RTOS_ERR err;
  s_timer_cb = callback;
  s_timer_period_ticks = period_sec * OS_CFG_TICK_RATE_HZ;

  OSTaskCreate(&s_metrics_tcb, "metrics", prv_metrics_task, NULL, METRICS_TASK_PRIO,
               &s_metrics_stk[0], METRICS_STK_SIZE / 10u, METRICS_STK_SIZE, 0, 0, NULL,
               OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR, &err);
  return true;
}

void memfault_metrics_heartbeat_collect_data(void) {
  MEMFAULT_LOG_INFO("Heartbeat!");

  // Track libc heap utilization
  struct mallinfo info = mallinfo();

  extern uint32_t _heap_bottom;
  extern uint32_t _heap_top;
  const uint32_t heap_size = (uint32_t)(&_heap_top - &_heap_bottom);
  const uint32_t heap_pct = (10000u * (uint32_t)info.uordblks) / heap_size;

  MEMFAULT_METRIC_SET_UNSIGNED(memory_pct_max, heap_pct);

  memfault_metrics_heartbeat_debug_print();
}
