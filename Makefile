# Error on any unset variable
MAKEFLAGS=--warn-undefined-variables

# Default to a silent build. Run make with --trace for a verbose build.
.SILENT:

# Mac and Linux stat have different options required, set them up here
ifeq ($(shell uname), Darwin)
    STAT = "stat -f '%z'"
else
    STAT = "stat -c '%s'"
endif

BOARD ?= qemu_mps2_an385

$(info Building for $(BOARD))

BOARD_DIR := boards/$(BOARD)

BUILD_DIR := build/$(BOARD)

ELF = $(BUILD_DIR)/main.elf

ARM_CC ?= arm-none-eabi-gcc
ARM_CXX ?= arm-none-eabi-g++
# if cc isn't set by the user, set it to ARM_CC
ifeq ($(origin CC),default)
CC := $(ARM_CC)
CXX := $(ARM_CXX)
endif
# use ccache if available
CCACHE := $(shell command -v ccache 2> /dev/null)
ifdef CCACHE
CC := ccache $(CC)
CXX := ccache $(CXX)
endif
OBJCOPY ?= $(shell $(CC) -print-prog-name=objcopy)
SIZE ?= $(shell $(CC) -print-prog-name=size)
READELF ?= $(shell $(CC) -print-prog-name=readelf)

# Simplicity SDK — clone if not present
SISDK_DIR ?= simplicity_sdk
SISDK_BRANCH ?= sisdk-2025.6

SISDK_STALE = \
    $(shell \
        if ! (cd $(SISDK_DIR) 2>/dev/null && git rev-parse --verify $(SISDK_BRANCH) >/dev/null 2>&1); then \
            echo SISDK_STALE; \
        fi \
    )
.PHONY: SISDK_STALE
$(SISDK_DIR)/.git/HEAD: $(SISDK_STALE)
	$(info Simplicity SDK needs updating, fetching from git (sparse checkout — platform/ only))
	rm -rf $(SISDK_DIR)
	git clone --depth 1 --branch $(SISDK_BRANCH) --filter=blob:none --sparse \
	    https://github.com/SiliconLabs/simplicity_sdk.git $(SISDK_DIR)
	cd $(SISDK_DIR) && git sparse-checkout set platform/micrium_os platform/common platform/CMSIS/Core/Include

.DEFAULT_GOAL :=

MICRIUM_OS_DIR := $(SISDK_DIR)/platform/micrium_os

include $(BOARD_DIR)/Makefile

# Micrium OS kernel sources
MICRIUM_SRCS += \
    $(MICRIUM_OS_DIR)/kernel/source/os_core.c \
    $(MICRIUM_OS_DIR)/kernel/source/os_dbg.c \
    $(MICRIUM_OS_DIR)/kernel/source/os_flag.c \
    $(MICRIUM_OS_DIR)/kernel/source/os_mon.c \
    $(MICRIUM_OS_DIR)/kernel/source/os_msg.c \
    $(MICRIUM_OS_DIR)/kernel/source/os_mutex.c \
    $(MICRIUM_OS_DIR)/kernel/source/os_prio.c \
    $(MICRIUM_OS_DIR)/kernel/source/os_q.c \
    $(MICRIUM_OS_DIR)/kernel/source/os_sem.c \
    $(MICRIUM_OS_DIR)/kernel/source/os_stat.c \
    $(MICRIUM_OS_DIR)/kernel/source/os_task.c \
    $(MICRIUM_OS_DIR)/kernel/source/os_time.c \
    $(MICRIUM_OS_DIR)/kernel/source/os_tmr.c \
    $(MICRIUM_OS_DIR)/kernel/source/os_var.c \
    $(MICRIUM_OS_DIR)/kernel/source/os_cfg_app.c \

# Micrium OS ARMv7-M port
MICRIUM_SRCS += \
    $(MICRIUM_OS_DIR)/ports/source/gnu/armv7m_os_cpu_a.S \
    $(MICRIUM_OS_DIR)/ports/source/gnu/armv7m_os_cpu_c.c \
    $(MICRIUM_OS_DIR)/ports/source/gnu/armv7m_cpu_a.S \
    $(MICRIUM_OS_DIR)/ports/source/gnu/armv7m_cpu_c.c \

# Micrium OS CPU and common sources
MICRIUM_SRCS += \
    $(MICRIUM_OS_DIR)/cpu/source/cpu_core.c \
    $(MICRIUM_OS_DIR)/common/source/lib/lib_mem.c \
    $(MICRIUM_OS_DIR)/common/source/lib/lib_str.c \
    $(MICRIUM_OS_DIR)/common/source/lib/lib_math.c \
    $(MICRIUM_OS_DIR)/common/source/kal/kal_kernel.c \
    $(MICRIUM_OS_DIR)/common/source/rtos/rtos_utils.c \

# Silabs platform sources needed by Micrium OS
MICRIUM_SRCS += \
    $(SISDK_DIR)/platform/common/src/sl_core_cortexm.c \
    stubs/sl_stubs.c \

# Add application sources
SRCS += \
    src/main.c \
    src/console.c \
    src/metrics.c \
    src/memfault/memfault_platform_port.c \
    $(BOARD_DIR)/startup.c \
    $(BOARD_DIR)/memfault_platform_impl.c \
    $(MICRIUM_SRCS) \

# Use Memfault SDK worker to gather initial Memfault SDK sources and include dirs
MEMFAULT_SDK_ROOT := memfault-firmware-sdk
MEMFAULT_COMPONENTS ?= core util panics metrics demo
include $(MEMFAULT_SDK_ROOT)/makefiles/MemfaultWorker.mk

# Add CFLAGS defines for each of the memfault components enabled above
CFLAGS += $(foreach each, $(MEMFAULT_COMPONENTS), -DMEMFAULT_COMPONENT_$(each)_)

# Add additional SDK sources to project
SRCS += \
    $(MEMFAULT_COMPONENTS_SRCS) \
    $(MEMFAULT_SDK_ROOT)/ports/panics/src/memfault_platform_ram_backed_coredump.c \

# All build output kept within build/
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

# Include paths — stubs FIRST so our rtos_description.h overrides the SDK's
INCLUDE_PATHS += \
    -Istubs \
    -Isrc \
    -Isrc/memfault \
    -I. \
    -I$(MICRIUM_OS_DIR) \
    -I$(MICRIUM_OS_DIR)/kernel/include \
    -I$(MICRIUM_OS_DIR)/ports/source/gnu \
    -I$(MICRIUM_OS_DIR)/cfg \
    -I$(SISDK_DIR)/platform/common/inc \
    -I$(SISDK_DIR)/platform/CMSIS/Core/Include \
    -I$(MEMFAULT_COMPONENTS_INC_FOLDERS) \
    -I$(MEMFAULT_SDK_ROOT)/ports/include \
    -I$(MEMFAULT_SDK_ROOT) \

# generic (non-arch-specific) CFLAGS
CFLAGS += \
    -nostartfiles \
    -Werror \
    -Wall \
    -Wextra \
    -Werror=undef \
    -ffunction-sections \
    -fdata-sections \
    -ggdb3 \
    -Og \
    -MD \
    -fdebug-prefix-map="$(shell pwd)"=. \
    -DBOARD=\"$(BOARD)\" \
    -DRTOS_CPU_SEL=14u \
    -DRTOS_MODULE_KERNEL_AVAIL \
    -DRTOS_MODULE_KERNEL_OS_AVAIL \
    $(ARCH_CFLAGS) \

LINKER_SCRIPT = $(BOARD_DIR)/linker.ld

LDFLAGS += \
    -Wl,-T$(LINKER_SCRIPT) \
    -Wl,--gc-sections \
    --specs=nano.specs \
    --specs=rdimon.specs \
    -Wl,-lc \
    -Wl,-lrdimon \
    -Wl,-Map=$(BUILD_DIR)/main.map \
    -Wl,--build-id \
    -Wl,--print-memory-usage \

.PHONY: all
all: $(ELF)

# Require SDK clone to complete before building sources
$(SRCS): $(SISDK_DIR)/.git/HEAD

# Store computed cflags in a file; it's a prerequisite for all objects.
RAW_CFLAGS := $(CFLAGS) $(LDFLAGS)
CFLAGS_STALE = \
    $(shell \
        if ! (echo "$(RAW_CFLAGS)" | diff -q $(BUILD_DIR)/cflags - > /dev/null 2>&1); then \
            echo CFLAGS_STALE; \
        fi \
    )
.PHONY: CFLAGS_STALE
$(BUILD_DIR)/cflags: $(CFLAGS_STALE) Makefile
	mkdir -p $(dir $@)
	echo "$(RAW_CFLAGS)" > $@

# Rule for .c files
$(BUILD_DIR)/%.c.o: %.c $(BUILD_DIR)/cflags
	mkdir -p $(dir $@)
	$(info Compiling $<)
	$(CC) -std=gnu11 $(CFLAGS) $(INCLUDE_PATHS) -c $< -o $@

# Rule for GAS assembly files
$(BUILD_DIR)/%.S.o: %.S $(BUILD_DIR)/cflags
	mkdir -p $(dir $@)
	$(info Assembling $<)
	$(CC) -x assembler-with-cpp $(CFLAGS) $(INCLUDE_PATHS) -c $< -o $@

$(ELF).uncompressed: $(OBJS) $(LINKER_SCRIPT)
	$(info Linking $@)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@

$(ELF): $(ELF).uncompressed
	echo -n 'Compressing debug info... '
	$(OBJCOPY) --compress-debug-sections $^ $@
	echo From $$("$(STAT)" $^) to $$("$(STAT)" $@) bytes
	$(SIZE) $@
	$(READELF) -n $@ | grep 'Build ID'

-include $(OBJS:.o=.d)

.PHONY: debug
debug: $(ELF)
	$(info Starting debugger)
	$(DEBUG_COMMAND)

.PHONY: gdb
gdb: $(ELF)
	$(GDB_COMMAND)

.PHONY: run
run: $(ELF)
	$(RUN_COMMAND)

.PHONY: clean
clean:
	rm -rf build
