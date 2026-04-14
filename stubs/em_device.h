// Stub em_device.h: provides CMSIS Cortex-M3 definitions for QEMU MPS2-AN385
#pragma once

// CMSIS requires IRQn_Type to be defined by the device header before core_cm3.h
typedef enum IRQn {
  // Cortex-M3 processor exceptions
  NonMaskableInt_IRQn   = -14,
  HardFault_IRQn        = -13,
  MemoryManagement_IRQn = -12,
  BusFault_IRQn         = -11,
  UsageFault_IRQn       = -10,
  SVCall_IRQn           = -5,
  DebugMonitor_IRQn     = -4,
  PendSV_IRQn           = -2,
  SysTick_IRQn          = -1,
  // QEMU MPS2-AN385 peripheral interrupts (minimal set)
  UART0RX_IRQn          = 0,
  TIMER0_IRQn           = 8,
} IRQn_Type;

// CMSIS configuration for Cortex-M3
#define __CM3_REV             0x0201U
#define __NVIC_PRIO_BITS      8
#define __Vendor_SysTickConfig 0
#define __MPU_PRESENT          1
#define __FPU_PRESENT          0

// Cortex-M3 CMSIS definitions (__get_BASEPRI, __set_BASEPRI, __get_PRIMASK, etc.)
#include "core_cm3.h"
