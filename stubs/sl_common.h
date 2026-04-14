// Stub sl_common.h for standalone build
#pragma once

#include <stdint.h>
#include <stdbool.h>

#define SL_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define SL_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define SL_WEAK __attribute__((weak))
#define SL_NORETURN __attribute__((noreturn))
#define SL_ATTRIBUTE_ALIGN(x) __attribute__((aligned(x)))

