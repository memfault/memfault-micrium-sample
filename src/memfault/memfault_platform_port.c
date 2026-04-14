//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

#include <stdio.h>
#include <time.h>

#include "memfault/components.h"
#include "memfault/panics/arch/arm/cortex_m.h"
#include "memfault/ports/reboot_reason.h"
#include "os.h"

#define MEMFAULT_DEBUG_LOG_BUFFER_SIZE_BYTES (512)

// Reboot tracking storage — must be in no-init RAM to survive resets
MEMFAULT_PUT_IN_SECTION(".noinit.mflt_reboot_info") static uint8_t
  s_reboot_tracking[MEMFAULT_REBOOT_TRACKING_REGION_SIZE];

// Memfault logging storage
static uint8_t s_log_buf_storage[512];

static eMemfaultPlatformLogLevel s_min_log_level = MEMFAULT_RAM_LOGGER_DEFAULT_MIN_LOG_LEVEL;

void memfault_platform_get_device_info(sMemfaultDeviceInfo *info) {
  *info = (sMemfaultDeviceInfo){
    .device_serial = "micrium-example",
    .hardware_version = BOARD,
    .software_type = "qemu-app",
    .software_version = "1.0.0",
  };
}

void memfault_platform_log(eMemfaultPlatformLogLevel level, const char *fmt, ...) {
  char log_buf[MEMFAULT_DEBUG_LOG_BUFFER_SIZE_BYTES];
  va_list args;
  va_start(args, fmt);

  if (level >= s_min_log_level) {
    vsnprintf(log_buf, sizeof(log_buf), fmt, args);
    switch (level) {
      case kMemfaultPlatformLogLevel_Debug:
        printf("\033[0;32m");
        break;
      case kMemfaultPlatformLogLevel_Info:
        printf("\033[0;37m");
        break;
      case kMemfaultPlatformLogLevel_Warning:
        printf("\033[0;33m");
        break;
      case kMemfaultPlatformLogLevel_Error:
        printf("\033[0;31m");
        break;
      default:
        break;
    }
    printf("%s", log_buf);
    printf("\033[0m\n");
  }

  va_end(args);
}

void memfault_platform_log_raw(const char *fmt, ...) {
  char log_buf[MEMFAULT_DEBUG_LOG_BUFFER_SIZE_BYTES];
  va_list args;
  va_start(args, fmt);

  vsnprintf(log_buf, sizeof(log_buf), fmt, args);
  printf("%s\n", log_buf);

  va_end(args);
}

void memfault_data_export_base64_encoded_chunk(const char *base64_chunk) {
  printf("%s\n", base64_chunk);
}

bool memfault_platform_time_get_current(sMemfaultCurrentTime *time_output) {
  time_t time_now = time(NULL);
  struct tm *tm_time = gmtime(&time_now);

  // Sanity-check the year
  if ((tm_time->tm_year < 123) || (tm_time->tm_year > 200)) {
    return false;
  }

  *time_output = (sMemfaultCurrentTime){
    .type = kMemfaultCurrentTimeType_UnixEpochTimeSec,
    .info = {
      .unix_timestamp_secs = (uint64_t)time_now,
    },
  };
  return true;
}

uint64_t memfault_platform_get_time_since_boot_ms(void) {
  RTOS_ERR err;
  // OSTimeGet() returns ticks; with OS_CFG_TICK_RATE_HZ=1000 this equals ms
  return (uint64_t)OSTimeGet(&err);
}

void memfault_platform_reboot_tracking_boot(void) {
  sResetBootupInfo reset_info = { 0 };
  memfault_reboot_reason_get(&reset_info);
  memfault_reboot_tracking_boot(s_reboot_tracking, &reset_info);
}

int memfault_platform_boot(void) {
  puts(MEMFAULT_BANNER_COLORIZED);

  memfault_platform_reboot_tracking_boot();

  static uint8_t s_event_storage[1024];
  const sMemfaultEventStorageImpl *evt_storage =
    memfault_events_storage_boot(s_event_storage, sizeof(s_event_storage));
  memfault_trace_event_boot(evt_storage);

  memfault_reboot_tracking_collect_reset_info(evt_storage);

  sMemfaultMetricBootInfo boot_info = {
    .unexpected_reboot_count = memfault_reboot_tracking_get_crash_count(),
  };
  memfault_metrics_boot(evt_storage, &boot_info);

  memfault_log_boot(s_log_buf_storage, MEMFAULT_ARRAY_SIZE(s_log_buf_storage));

  memfault_build_info_dump();
  memfault_device_info_dump();
  MEMFAULT_LOG_INFO("Memfault Initialized!");

  return 0;
}
