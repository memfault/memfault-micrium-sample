//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

#include <sys/times.h>

#include "console.h"
#include "memfault/components.h"
#include "os.h"

// CPU clock for qemu_mps2_an385
#define CPU_CLOCK_HZ 20000000UL

// Worker task: pends on a semaphore forever (stays in blocked state for testing)
#define WORKER_TASK_PRIO 10u
#define WORKER_STK_SIZE 256u

static OS_TCB s_worker_tcb;
static CPU_STK s_worker_stk[WORKER_STK_SIZE];
static OS_SEM s_worker_sem;

static void prv_worker_task(void *p_arg) {
  (void)p_arg;
  RTOS_ERR err;

  while (1) {
    // Block indefinitely on semaphore — stays "blocked" for backend decode testing
    OSSemPend(&s_worker_sem, 0, OS_OPT_PEND_BLOCKING, NULL, &err);
  }
}

// Custom _sbrk() implementation.
// The default newlib implementation checks the heap doesn't grow past the
// current stack pointer. This breaks with static task stacks in .bss which
// are at lower addresses than the heap.
void *_sbrk(ptrdiff_t incr) {
  static char *heap_end;
  char *prev_heap_end;

  if (heap_end == NULL) {
    extern uint32_t _heap_bottom;
    heap_end = (char *)&_heap_bottom;
  }

  prev_heap_end = heap_end;
  extern uint32_t _heap_top;
  if (heap_end + incr > (char *)&_heap_top) {
    return (void *)-1;
  }

  heap_end += incr;
  return (void *)prev_heap_end;
}

// SysTick setup — Micrium OS on QEMU doesn't have sl_sleeptimer,
// so we configure SysTick directly.
static void prv_systick_init(uint32_t ticks_per_sec) {
  // SysTick registers
  volatile uint32_t *SYST_CSR = (uint32_t *)0xE000E010;
  volatile uint32_t *SYST_RVR = (uint32_t *)0xE000E014;

  *SYST_RVR = (CPU_CLOCK_HZ / ticks_per_sec) - 1;
  *SYST_CSR = 0x07;  // Enable, tick interrupt, processor clock
}

int main(void) {
  RTOS_ERR err;

  // OSInit() must precede memfault_platform_boot() because booting metrics
  // calls memfault_platform_metrics_timer_boot(), which creates a task.
  OSInit(&err);

  memfault_platform_boot();

  // Create semaphore for worker task (initially unavailable)
  OSSemCreate(&s_worker_sem, "worker_sem", 0, &err);

  // Create worker task (will block on semaphore)
  OSTaskCreate(&s_worker_tcb, "worker", prv_worker_task, NULL, WORKER_TASK_PRIO,
               &s_worker_stk[0], WORKER_STK_SIZE / 10u, WORKER_STK_SIZE, 0, 0, NULL,
               OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR, &err);

  // Create console task (handles Memfault demo shell CLI)
  console_task_start();

  // Configure SysTick
  prv_systick_init(OS_CFG_TICK_RATE_HZ);

  OSStart(&err);

  // Should never reach here
  while (1) { };
  return 0;
}
