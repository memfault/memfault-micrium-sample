//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! Application platform config for the uC/OS-II QEMU example.

#pragma once

#define MEMFAULT_USE_GNU_BUILD_ID 1
#define MEMFAULT_PLATFORM_COREDUMP_STORAGE_RAM_SIZE 8192
#define MEMFAULT_COREDUMP_COLLECT_LOG_REGIONS 1

#define MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS 60

// Enable adding custom demo shell commands
#define MEMFAULT_DEMO_SHELL_COMMAND_EXTENSIONS 1

#if !defined(MEMFAULT_PLATFORM_HAS_LOG_CONFIG)
  #define MEMFAULT_PLATFORM_HAS_LOG_CONFIG 1
#endif

// 1 = Info
#if !defined(MEMFAULT_RAM_LOGGER_DEFAULT_MIN_LOG_LEVEL)
  #define MEMFAULT_RAM_LOGGER_DEFAULT_MIN_LOG_LEVEL 1
#endif
