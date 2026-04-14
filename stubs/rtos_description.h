// Stub RTOS description for standalone Micrium OS build on QEMU (Cortex-M3)
#pragma once

// Include rtos_opt_def.h to get the constant definitions (RTOS_CPU_SEL_ARM_CORTEX_M3, etc.)
// that rtos_path.h compares against. RTOS_CPU_SEL itself is passed via -D compiler flag.
#include <common/include/rtos_opt_def.h>

// When RTOS_CPU_SEL != SILABS_GECKO_AUTO, rtos_path.h skips em_device.h,
// which means CMSIS core headers are never included. Pull them in here so
// Micrium OS common lib has access to __WEAK, __NOP, etc.
#include "em_device.h"
