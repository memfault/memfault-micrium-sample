// Stub CPU configuration for QEMU Cortex-M3
#pragma once

#include <common/include/lib_def.h>

#define CPU_CFG_CACHE_MGMT_EN  DEF_DISABLED
#define CPU_CFG_NAME_EN        DEF_DISABLED
#define CPU_CFG_TS_32_EN       DEF_DISABLED
#define CPU_CFG_TS_64_EN       DEF_DISABLED
#define CPU_CFG_NAME_SIZE      16u
#define CPU_CFG_TS_TMR_SIZE    4u   // CPU_WORD_SIZE_32 (octets, not bits)
