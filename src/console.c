//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

#include "console.h"

#include <stdio.h>
#include <unistd.h>

#include "memfault/components.h"
#include "os.h"

#if defined(MEMFAULT_DEMO_SHELL_COMMAND_EXTENSIONS)

static const char *prv_task_state_str(const OS_TCB *tcb) {
  if (tcb == OSTCBCurPtr) {
    return "Running";
  }
  switch (tcb->TaskState) {
    case OS_TASK_STATE_RDY:
      return "Ready";
    case OS_TASK_STATE_DLY:
      return "Delayed";
    case OS_TASK_STATE_PEND:
    case OS_TASK_STATE_PEND_TIMEOUT:
      return "Blocked";
    case OS_TASK_STATE_SUSPENDED:
    case OS_TASK_STATE_DLY_SUSPENDED:
    case OS_TASK_STATE_PEND_SUSPENDED:
    case OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED:
      return "Suspended";
    case OS_TASK_STATE_DEL:
      return "Deleted";
    default:
      return "Unknown";
  }
}

static int prv_micrium_tasks_cmd(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  printf("%-16s %8s  %-10s  %5s / %-5s\n", "Name", "Priority", "State", "Used", "Total");
  printf("%-16s %8s  %-10s  %5s   %-5s\n", "----", "--------", "-----", "----", "-----");

#if (OS_CFG_DBG_EN == DEF_ENABLED)
  OS_TCB *tcb = OSTaskDbgListPtr;
  while (tcb != NULL) {
    const char *name = (tcb->NamePtr != NULL) ? (const char *)tcb->NamePtr : "?";

    CPU_STK_SIZE total = tcb->StkSize;
    // Estimate used stack by counting zero words from base
    CPU_STK_SIZE free_words = 0;
    CPU_STK *p = tcb->StkBasePtr;
    while (free_words < total && *p == 0u) {
      p++;
      free_words++;
    }
    CPU_STK_SIZE used = total - free_words;

    printf("%-16s %8u  %-10s  %5lu / %-5lu\n", name, (unsigned)tcb->Prio,
           prv_task_state_str(tcb), (unsigned long)used, (unsigned long)total);

    tcb = tcb->DbgNextPtr;
  }
#else
  printf("(OS_CFG_DBG_EN disabled — task list not available)\n");
#endif

  return 0;
}

static const sMemfaultShellCommand s_micrium_shell_extensions[] = {
  {
    .command = "micrium_tasks",
    .handler = prv_micrium_tasks_cmd,
    .help = "Print all Micrium OS tasks with priority, state, and stack usage",
  },
};

#endif  // MEMFAULT_DEMO_SHELL_COMMAND_EXTENSIONS

#define CONSOLE_TASK_PRIO 5u
#define CONSOLE_STK_SIZE 512u

static OS_TCB s_console_tcb;
static CPU_STK s_console_stk[CONSOLE_STK_SIZE];

static int prv_send_char(char c) {
  int sent = putchar(c);
  fflush(stdout);
  return sent;
}

static void prv_console_task(void *p_arg) {
  (void)p_arg;
  RTOS_ERR err;

  while (1) {
    char c;
    if (read(0, &c, sizeof(c))) {
      memfault_demo_shell_receive_char(c);
    } else {
      // Yield to avoid spinning at 100% CPU when no input is available
      OSTimeDly(10, OS_OPT_TIME_DLY, &err);
    }
  }
}

void console_task_start(void) {
  RTOS_ERR err;

  OSTaskCreate(&s_console_tcb, "console", prv_console_task, NULL, CONSOLE_TASK_PRIO,
               &s_console_stk[0], CONSOLE_STK_SIZE / 10u, CONSOLE_STK_SIZE, 0, 0, NULL,
               OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR, &err);

#if defined(MEMFAULT_DEMO_SHELL_COMMAND_EXTENSIONS)
  memfault_shell_command_set_extensions(s_micrium_shell_extensions,
                                        MEMFAULT_ARRAY_SIZE(s_micrium_shell_extensions));
#endif

  const sMemfaultShellImpl impl = {
    .send_char = prv_send_char,
  };
  memfault_demo_shell_boot(&impl);
}
