//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

#include <stdint.h>
#include <sys/types.h>

#include "memfault/components.h"

typedef struct UART_t {
  volatile uint32_t DATA;
  volatile uint32_t STATE;
  volatile uint32_t CTRL;
  volatile uint32_t INTSTATUS;
  volatile uint32_t BAUDDIV;
} UART_t;

#define UART0_ADDR ((UART_t *)(0x40004000))
#define UART_DR(baseaddr) (*(unsigned int *)(baseaddr))

#define UART_STATE_TXFULL (1 << 0)
#define UART_CTRL_TX_EN (1 << 0)
#define UART_CTRL_RX_EN (1 << 1)
#define UART_STATE_RXRDY (2 << 0)

void uart_init(void) {
  UART0_ADDR->BAUDDIV = 16;
  UART0_ADDR->CTRL = UART_CTRL_TX_EN | UART_CTRL_RX_EN;
}

int _read(__attribute__((unused)) int file, __attribute__((unused)) char *buf,
          __attribute__((unused)) int len) {
  for (int i = 0; i < len; i++) {
    if (UART0_ADDR->STATE & UART_STATE_RXRDY) {
      char data = UART0_ADDR->DATA;
      // remap backspace/del
      data = (data == 0x7f) ? '\b' : data;
      data = (data == 0x7e) ? 0x7f : data;
      buf[i] = data;
    } else {
      return i;
    }
  }
  return len;
}

int _write(__attribute__((unused)) int file, __attribute__((unused)) char *buf, int len) {
  int todo;
  for (todo = 0; todo < len; todo++) {
    UART_DR(UART0_ADDR) = *buf++;
  }
  return len;
}

extern int main(void);

// Following symbols are defined by the linker.
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t __bss_start__;
extern uint32_t __bss_end__;
extern void __stack(void);

__attribute__((noinline)) static void prv_cinit(void) {
  for (uint32_t *dst = &_sdata, *src = &_sidata; dst < &_edata;) {
    *(dst++) = *(src++);
  }
  for (uint32_t *dst = &__bss_start__; (uintptr_t)dst < (uintptr_t)&__bss_end__;) {
    *(dst++) = 0;
  }
}

__attribute__((noreturn)) void Reset_Handler(void) {
// enable mem/bus/usage faults
#define SCBSHCSR_ (*(uint32_t *)0xE000ED24)
  SCBSHCSR_ |= (1UL << 16U) | (1UL << 17U) | (1UL << 18U);

  prv_cinit();
  uart_init();

  (void)main();

  while (1) { };
}

// Micrium OS Cortex-M port handlers (CMSIS naming convention)
extern void PendSV_Handler(void);
extern void SysTick_Handler(void);

// A minimal vector table for a Cortex-M3.
__attribute__((section(".isr_vector"))) void (*const g_pfnVectors[])(void) = {
  __stack,                        // initial stack pointer
  Reset_Handler,                  // 1  Reset
  NMI_Handler,                    // 2  NMI
  HardFault_Handler,              // 3  Hard Fault
  MemManage_Handler,              // 4  Memory Management
  BusFault_Handler,               // 5  Bus Fault
  UsageFault_Handler,             // 6  Usage Fault
  0,                              // 7  reserved
  0,                              // 8  reserved
  0,                              // 9  reserved
  0,                              // 10 reserved
  0,                              // 11 SVC
  0,                              // 12 DebugMon
  0,                              // 13 reserved
  PendSV_Handler,                 // 14 PendSV (context switch)
  SysTick_Handler,                // 15 SysTick
  0,                              // 16 UART 0 RX
  0,                              // 17 UART 0 TX
  0,                              // 18 UART 1 RX
  0,                              // 19 UART 1 TX
  0,                              // 20 UART 2 combined
  0,                              // 21 UART 2
  0,                              // 22 GPIO 0
  0,                              // 23 GPIO 1
  MEMFAULT_EXC_HANDLER_WATCHDOG,  // 24 Timer 0
};
