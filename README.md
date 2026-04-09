# SofTEE: Kernel-Assisted Protected Execution Subsystem (PEX)

SofTEE is an operating-system-centric trusted execution environment that provides
fine-grained, intra-process isolation using a Linux kernel module and user-space runtime.

## Project Layout

- `kernel/` - Linux kernel module (`pex.ko`)
- `include/` - shared UAPI definitions (`ioctl` structs/codes)
- `libpex/` - user-space runtime library (`libpex.a`)
- `examples/` - demo program for context lifecycle and thread violation
- `tests/` - isolation and fault behavior tests
- `scripts/` - helper scripts to load/unload and run scenario
- `docs/` - architecture and design notes

## Core Features

- Protected execution contexts bound to owner process+thread
- Kernel-enforced execution gating (`ENTER`/`EXIT`)
- Protected memory mapping via `/dev/pex` + `mmap`
- Fault-on-access when context is inactive or wrong thread accesses memory
- Runtime statistics and observability via `/proc/pex_stats`

## Build (Linux)

Requirements:

- Linux kernel headers (`/lib/modules/$(uname -r)/build`)
- `gcc`, `make`

```bash
make all
```

If you only want user-space components (`libpex`, demo, tests), run:

```bash
make user
```

This builds:

- `kernel/pex.ko`
- `libpex/libpex.a`
- `examples/demo`
- `tests/test_faults`

## Run

```bash
chmod +x scripts/*.sh
./scripts/load_pex.sh
./scripts/run_all.sh
./scripts/unload_pex.sh
```

## /dev + API Flow

1. Open `/dev/pex`
2. `PEX_IOCTL_CREATE` to allocate a context and kernel pages
3. `mmap` with offset `id * PAGE_SIZE` to map context
4. `PEX_IOCTL_ENTER` before touching protected memory
5. `PEX_IOCTL_EXIT` to leave protected mode
6. `PEX_IOCTL_DESTROY` to free resources

## Demonstration Scenarios Implemented

- Secure memory isolation outside protected mode
- Thread ownership enforcement (cross-thread denial)
- Controlled entry/exit boundaries
- Dynamic page-fault based enforcement
- Fault logging and runtime metrics

## Notes

- This is kernel-assisted software isolation, not hardware-backed TEE.
- It is suitable for OS research, teaching, and prototype experimentation.

## Troubleshooting (WSL2)

If you see:

`/lib/modules/<kernel>/build: No such file or directory`

your running kernel headers are missing. In WSL2 this is common with the
`*-microsoft-standard-WSL2` kernel.

Options:

1. Install matching headers (if available for your distro/kernel).
2. Build against a prepared kernel source tree and pass `KDIR`:

```bash
make KDIR=/path/to/linux-build -C kernel
```

3. Build only user-space pieces until kernel headers are configured:

```bash
make user
```