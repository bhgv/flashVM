# Embedded Dis host: required services

[Русский](HOST-REQUIREMENTS.ru.md) · [Current archive boundary](README.md) ·
[IDE implementation](../ide/dis/idecore.b)

The target is Dis embedded alongside FlashVM, with FlashVM rendering and typed
module/channel adapters. A separately installed Inferno `emu` is a reference-test
option, not a deployment dependency. The current `libdisvm_core.a` is not yet
an executable host. This document records the remaining implementation boundary;
it does not claim execution or protocol conformance.

## What the present IDE actually imports

Production `ide/dis/idecore.b` includes `idecore.m` and loads no external module.
Its document service needs the real Dis scheduler, `spawn`, rendezvous channels,
strings, arrays, lists, garbage collection and exception/module machinery.

Console examples and regression clients include `sys.m` and `idecore.m`, load
`$Sys` and `idecore.dis`, and use `Sys.print`/`sprint`. Their reference-runtime
setup writes interpreter/JIT controls through `Sys.open`/`write` on `/dev/sysctl`.
That device is reference-test control, not an IDE widget API. An embedded runner
must have its own explicit execution-mode configuration and corresponding tests;
returning success from an unimplemented sysctl operation would be invalid.

No Draw, Tk, Prefab, Keyring, Inferno window manager or GUI module is needed for
these clients. FlashVM display objects, input events and controls remain the UI.

## Host closure still to implement

| Boundary | Selected source / contract | Required implementation |
| --- | --- | --- |
| Process scheduling | `upstream/emu/port/dis.c`, `include/kernel.h` | runnable/blocked processes, cancellation, locks, error unwinding and host wakeups |
| Loading and native modules | `libinterp/load.c`, `loader.c`, `link.c`, `runt.c` | compatible frame layouts/signatures, namespace lookup, builtin registration and entry invocation |
| Memory and FPU | `libinterp/heap.c`, `gc.c`, `stack.c` | allocation, GC roots, safe points, exception/FPU save and restore |
| `$Sys` and files | `upstream/emu/port/inferno.c`, `kernel.h` | descriptor ownership, error strings, bounded I/O and formatting; explicit unsupported operations |
| SWF/Dis boundary | typed `.m` declarations and generated AS scaffolds | validated signatures, copied values/capabilities, pending-call roots, cancellation and debugger boundary frames |
| Filesystem transport | Styx/9P host service | exact protocol/version negotiation, tags/fids, errors, flush and disconnect cleanup |

Dis channels are typed VM rendezvous queues. Styx/9P is a filesystem wire
protocol; it is not the byte representation of a channel. A bridge can expose
channel services through Styx, but retaining the original channel implementation
alone does not prove Plan 9/Styx compatibility. That acceptance needs protocol
round trips and cancellation/error tests against the chosen protocol version.

The source-built `mk`, `iyacc` and native Limbo bootstrap use POSIX C support,
not these runtime modules. They can build before the embedded runner exists.
Native Limbo remains temporary; the owned Coco/R Limbo → FIR compiler has priority.

## Implemented host foundation (v0.1534)

[Shared host](host/README.md) now implements owned module images from memory, embedded bytes, FlashVM files, sockets and channels; an adapter invokes production parsemod. Real native threads/synchronization reuse FlashVM ports, and opt-in TLS register isolation is tested. The chosen experiment targets actual parallel OS threads. Global GC/module/process synchronization and a complete executable host remain open.
