# Memfault Micrium OS (uC/OS-III) Sample

A minimal Micrium OS application running on QEMU (ARM Cortex-M3, MPS2-AN385),
integrated with the [Memfault Firmware SDK](https://github.com/memfault/memfault-firmware-sdk)
for coredump capture, metrics, and reboot tracking.

Uses the Micrium OS kernel from Silicon Labs'
[Simplicity SDK](https://github.com/SiliconLabs/simplicity_sdk) (`sisdk-2025.6`).

## Prerequisites

- `arm-none-eabi-gcc` toolchain
- `git` (the Simplicity SDK is fetched automatically on first build)
- `qemu-system-arm` (optional, for running)

## Building

```bash
git submodule update --init
make
```

The Simplicity SDK is sparse-cloned automatically into `simplicity_sdk/` on first
build. To use an existing checkout instead:

```bash
make SISDK_DIR=/path/to/simplicity_sdk
```

Output ELF: `build/qemu_mps2_an385/main.elf`

## Running on QEMU

```bash
make run
```

The Memfault demo shell is available over the QEMU serial console. Type `help`
for available commands, or `assert` to trigger a crash and capture a coredump.

## Project Structure

```
├── Makefile                  # Build system
├── memfault-firmware-sdk/    # Memfault SDK (submodule)
├── boards/qemu_mps2_an385/  # Board support (startup, linker, platform impl)
├── src/                      # Application code
│   ├── main.c                # Entry point, task creation
│   ├── console.c             # CLI task (Memfault demo shell)
│   ├── metrics.c             # Heartbeat metrics task
│   ├── os_cfg.h              # Micrium OS kernel config
│   └── memfault/             # Memfault platform port
└── stubs/                    # Shims for Silabs platform dependencies on QEMU
```
