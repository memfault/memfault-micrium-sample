//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

#include "memfault/components.h"
#include "memfault/panics/arch/arm/cortex_m.h"
#include "os.h"

#define SCB_AIRCR_ADDR (0xE000ED0C)
#define SCB_AIRCR_VECTKEY_WRITE (0x5FAUL << 16U)
#define SCB_AIRCR_SYSRESETREQ (1U << 2)
#define SCB_AIRCR_RESET_SYSTEM (SCB_AIRCR_VECTKEY_WRITE | SCB_AIRCR_SYSRESETREQ)

#define CMSDK_RSTINFO_ADDR (0x4001F010)
#define CMSDK_RSTINFO_MASK (0x07)
#define CMSDK_RSTINFO_SYSRESETREQ_MASK (1 << 0)
#define CMSDK_RSTINFO_WATCHDOG_MASK (1 << 1)
#define CMSDK_RSTINFO_LOCKUP_MASK (1 << 2)

void memfault_platform_reboot(void) {
  volatile uint32_t *SCB_AIRCR = (uint32_t *)SCB_AIRCR_ADDR;
  *SCB_AIRCR = SCB_AIRCR_RESET_SYSTEM;

  while (1) { }  // unreachable
}

size_t memfault_platform_sanitize_address_range(void *start_addr, size_t desired_size) {
  static const struct {
    uint32_t start_addr;
    size_t length;
  } s_mcu_mem_regions[] = {
    { .start_addr = 0x20000000, .length = 0x800000 },
  };

  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(s_mcu_mem_regions); i++) {
    const uint32_t lower_addr = s_mcu_mem_regions[i].start_addr;
    const uint32_t upper_addr = lower_addr + s_mcu_mem_regions[i].length;
    if ((uint32_t)start_addr >= lower_addr && ((uint32_t)start_addr < upper_addr)) {
      return MEMFAULT_MIN(desired_size, upper_addr - (uint32_t)start_addr);
    }
  }

  return 0;
}

static uint32_t prv_read_reboot_reg(void) {
  volatile uint32_t *cmsdk_rstinfo = (uint32_t *)CMSDK_RSTINFO_ADDR;
  return (*cmsdk_rstinfo & CMSDK_RSTINFO_MASK);
}

static void prv_clear_reboot_reg(void) {
  volatile uint32_t *cmsdk_rstinfo = (uint32_t *)CMSDK_RSTINFO_ADDR;
  *cmsdk_rstinfo = 1;
}

//! Decode the reset info register
//!
//! NB: QEMU does not follow the behavior specified in AN385 and always returns 0
static eMemfaultRebootReason prv_decode_reboot_reg(uint32_t reboot_reg) {
  eMemfaultRebootReason result = kMfltRebootReason_Unknown;
  if (reboot_reg & CMSDK_RSTINFO_SYSRESETREQ_MASK) {
    result = kMfltRebootReason_SoftwareReset;
  } else if (reboot_reg & CMSDK_RSTINFO_WATCHDOG_MASK) {
    result = kMfltRebootReason_HardwareWatchdog;
  } else if (reboot_reg & CMSDK_RSTINFO_LOCKUP_MASK) {
    result = kMfltRebootReason_Lockup;
  } else {
    result = kMfltRebootReason_PowerOnReset;
  }

  prv_clear_reboot_reg();
  return result;
}

void memfault_reboot_reason_get(sResetBootupInfo *reset_info) {
  uint32_t reboot_reg = prv_read_reboot_reg();
  eMemfaultRebootReason reboot_reason = prv_decode_reboot_reg(reboot_reg);

  *reset_info = (sResetBootupInfo){
    .reset_reason_reg = reboot_reg,
    .reset_reason = reboot_reason,
  };
}

static uint32_t prv_read_psp_reg(void) {
  uint32_t reg_val;
  __asm volatile("mrs %0, psp" : "=r"(reg_val));
  return reg_val;
}

// Coredump region array — sized to hold:
//   1 active stack + 1 optional ISR PSP stack
//   + 1 __memfault_capture_bss region
//   + per-task: TCB + stack (2 regions each)
#define COREDUMP_MAX_REGIONS (3 + 2 * 20)

static sMfltCoredumpRegion s_coredump_regions[COREDUMP_MAX_REGIONS];

extern uint32_t __memfault_capture_bss_end;
extern uint32_t __memfault_capture_bss_start;

const sMfltCoredumpRegion *memfault_platform_coredump_get_regions(
  const sCoredumpCrashInfo *crash_info, size_t *num_regions) {
  int region_idx = 0;

  // 1. Capture active stack (MSP or PSP depending on where crash occurred)
  size_t stack_size_to_collect = memfault_platform_sanitize_address_range(
    crash_info->stack_address, MEMFAULT_PLATFORM_ACTIVE_STACK_SIZE_TO_COLLECT);
  s_coredump_regions[region_idx++] =
    MEMFAULT_COREDUMP_MEMORY_REGION_INIT(crash_info->stack_address, stack_size_to_collect);

  // 2. If crash was in ISR (MSP active), also collect the PSP (task stack)
  const bool msp_was_active = (crash_info->exception_reg_state->exc_return & (1 << 2)) == 0;
  if (msp_was_active) {
    void *psp = (void *)prv_read_psp_reg();
    const uint32_t extra_stack_bytes = 128;
    stack_size_to_collect = memfault_platform_sanitize_address_range(
      psp, MEMFAULT_PLATFORM_ACTIVE_STACK_SIZE_TO_COLLECT + extra_stack_bytes);
    s_coredump_regions[region_idx++] =
      MEMFAULT_COREDUMP_MEMORY_REGION_INIT(psp, stack_size_to_collect);
  }

  // 3. Capture Micrium OS core state (OSTCBCurPtr, OSTaskDbgListPtr, OSRunning, ...)
  const size_t bss_capture_size =
    (uint32_t)&__memfault_capture_bss_end - (uint32_t)&__memfault_capture_bss_start;
  s_coredump_regions[region_idx++] =
    MEMFAULT_COREDUMP_MEMORY_REGION_INIT(&__memfault_capture_bss_start, bss_capture_size);

  // 4. Walk the debug TCB list and capture each task's TCB + stack
#if (OS_CFG_DBG_EN == DEF_ENABLED)
  OS_TCB *tcb = OSTaskDbgListPtr;
  while (tcb != NULL) {
    // Validate the TCB pointer itself before dereferencing
    if (memfault_platform_sanitize_address_range(tcb, sizeof(OS_TCB)) < sizeof(OS_TCB)) {
      break;
    }

    if (region_idx < (int)MEMFAULT_ARRAY_SIZE(s_coredump_regions)) {
      // Capture the TCB struct itself (needed for thread decode)
      s_coredump_regions[region_idx++] =
        MEMFAULT_COREDUMP_MEMORY_REGION_INIT(tcb, sizeof(OS_TCB));
    }

    if (tcb->StkBasePtr != NULL && tcb->StkSize > 0 &&
        region_idx < (int)MEMFAULT_ARRAY_SIZE(s_coredump_regions)) {
      const size_t stk_bytes = (size_t)tcb->StkSize * sizeof(CPU_STK);
      const size_t sanitized =
        memfault_platform_sanitize_address_range(tcb->StkBasePtr, stk_bytes);
      if (sanitized > 0) {
        s_coredump_regions[region_idx++] =
          MEMFAULT_COREDUMP_MEMORY_REGION_INIT(tcb->StkBasePtr, sanitized);
      }
    }

    // Validate DbgNextPtr before following it
    OS_TCB *next = tcb->DbgNextPtr;
    if (next != NULL &&
        memfault_platform_sanitize_address_range(next, sizeof(OS_TCB)) < sizeof(OS_TCB)) {
      break;
    }
    tcb = next;
  }
#endif

  *num_regions = (size_t)region_idx;
  return &s_coredump_regions[0];
}
