# SofTEE / PEX Design Notes

## Components

1. Kernel module (`pex.ko`)
2. Device endpoint (`/dev/pex`)
3. Runtime (`libpex`)
4. Observability endpoint (`/proc/pex_stats`)

## Context Lifecycle

- CREATE: allocates context metadata and backing pages
- MAP: exposes virtual mapping in user process
- ENTER: marks context active and starts accounting
- EXIT: marks context inactive and accumulates runtime
- DESTROY: frees pages and context slot

## Enforcement Model

- Every context has owner `{tgid, tid}`
- Access is only legal when:
  - context is active
  - current thread matches owner thread
- Violations increment fault counter and return `SIGSEGV`

## Memory Protection Mechanics

- User mapping is backed by on-demand page-fault handler (`vm_ops.fault`)
- Fault handler checks policy before returning pages
- Inactive/wrong-thread accesses are denied via fault return

## Observability

`/proc/pex_stats` exports:

- context id/name
- owner process/thread
- active state
- size
- enter count
- fault count
- total protected execution time (ns)

## Current Limits

- Fixed max contexts (`PEX_MAX_CONTEXTS`)
- Single-owner thread model
- No syscall filtering inside context
- No multi-process sharing policies