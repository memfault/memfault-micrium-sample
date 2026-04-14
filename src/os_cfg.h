//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! Micrium OS (uC/OS-III) kernel configuration for the Memfault QEMU example.

#ifndef OS_CFG_H
#define OS_CFG_H

#include <common/include/lib_def.h>

// QEMU MPS2-AN385: 8 NVIC priority bits
#define CPU_CFG_NVIC_PRIO_BITS 8u

// Tick rate (1ms ticks)
#define OS_CFG_TICK_RATE_HZ 1000u

// Maximum number of priorities (must be power of 2)
#define OS_CFG_PRIO_MAX 64u

// Minimum stack size (in CPU_STK elements)
#define OS_CFG_STK_SIZE_MIN 64u

// Debug support — required for thread list enumeration via DbgNextPtr
#define OS_CFG_DBG_EN DEF_ENABLED

// Application hooks
#define OS_CFG_APP_HOOKS_EN 0

// Timestamp support
#define OS_CFG_TS_EN 0

// Tick enabled
#define OS_CFG_TICK_EN 1

// Synchronization primitives
#define OS_CFG_FLAG_EN 1
#define OS_CFG_MUTEX_EN 1
#define OS_CFG_Q_EN 0
#define OS_CFG_SEM_EN 1
#define OS_CFG_MON_EN 0

// Task features
#define OS_CFG_STAT_TASK_EN DEF_ENABLED
#define OS_CFG_TASK_Q_EN 0
#define OS_CFG_TASK_REG_TBL_SIZE 3
#define OS_CFG_TASK_STK_REDZONE_EN 0
#define OS_CFG_TASK_STK_REDZONE_DEPTH 8

// Timers
#define OS_CFG_TMR_EN 1

// Scheduling
#define OS_CFG_SCHED_ROUND_ROBIN_EN 0
#define OS_CFG_SCHED_LOCK_TIME_MEAS_EN 0

// Task profiling / stack checking
#define OS_CFG_TASK_PROFILE_EN DEF_ENABLED
#define OS_CFG_STAT_TASK_STK_CHK_EN DEF_ENABLED

// Event flags
#define OS_CFG_FLAG_MODE_CLR_EN DEF_ENABLED

// ISR post deferral
#define OS_CFG_ISR_POST_DEFERRED_EN 0

// Dynamic tick (disable for SysTick-based ticking)
#define OS_CFG_DYN_TICK_EN 0

// Compatibility / misc
#define OS_CFG_COMPAT_INIT_EN 0
#define OS_CFG_ERRNO_EN 0

// Idle task stack size
#define OS_CFG_IDLE_TASK_STK_SIZE 256u

// Tick task stack size
#define OS_CFG_TICK_TASK_STK_SIZE 256u

#endif  // OS_CFG_H
