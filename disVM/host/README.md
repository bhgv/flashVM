# Shared FlashVM/Dis host and native-thread experiment

[Русский](README.ru.md) · [Module input API](../include/disvm/module_image.h) ·
[Shared ports](../include/disvm/flashvm_host.h) · [Native context](../include/disvm/native_context.h)

This host layer reuses actual FlashVM platform sources. It does not import
Inferno GUI, networking stacks or a second OS-thread implementation for Dis.
The retained Dis parser is already in `upstream/libinterp/load.c` (`parsemod`);
`readmod.c` is Inferno's filesystem-facing wrapper. Our
[module_parse.c](module_parse.c) feeds the same production parser from owned bytes.

## Build

```sh
./build-workspace.sh --projects disvm-host --tests
```

Standalone, including Windows/macOS/Android toolchain builds, with CMake 3.10+:

```sh
mkdir -p build/disvm-host
cd build/disvm-host
cmake ../../disVM/host -DDVM_FLASHVM_SOURCE_ROOT=/absolute/path/to/flashVM/flashVM
cmake --build .
ctest --output-on-failure
```

Pass the Android NDK toolchain through CMake in the usual way; this selects
`ANDROID`, not the Linux port. Windows selects `windows`, macOS selects `mac`,
Android selects `android`, Linux selects `linux`. The corresponding
`ports/PORT/thread.c` and `channel_sync.c` are the same files FlashVM uses.
`ports/file_port.c` supplies portable binary/UTF-8-path files, and
`ports/network/socket.c` supplies POSIX/Winsock deadline-aware networking.
The new [native_context.c](native_context.c) uses Windows TLS or POSIX pthread TLS.

When embedded under a parent CMake project with an existing FlashVM target, set
`DVM_FLASHVM_TARGET` to its name. That links the existing target instead of
compiling duplicate port implementations. Standalone builds compile just the
required port files. The channel adapter references the actual FlashVM channel
API; an executable using it must also link FlashVM and its selected dependencies.
`DVM_FLASHVM_LIBRARIES` optionally supplies that library list and enables the real
channel integration test. No mock channel or automatic SDK search is used.

Host-adapter sources support the four platform selections. Actual execution
acceptance in this series is Linux AMD64. Windows, macOS and Android require
native/cross builds and device runs; the legacy interpreter core's native ABI
and headers are a separate unfinished platform boundary. This standalone host
build avoids the core's existing POSIX-only configuration restriction; it does
not imply that a complete Windows Dis VM already builds or runs.

## One module input boundary

All [image functions](../include/disvm/module_image.h) produce an owned native
`DVM_ModuleImage`, independent of either VM heap:

| Source | Entry point | Current behavior |
| --- | --- | --- |
| IDE compiler output / memory | `dvm_image_copy` | deep-copy exact bytes, caller may release its compiler buffer |
| Embedded executable resource | `dvm_image_copy` | pass the resource address and exact byte extent; no filesystem needed |
| Disk | `dvm_image_file` | use FlashVM file port; close only the handle opened here |
| Network | `dvm_image_socket` | borrow FlashVM's connected socket, retain its timeout/cancellation policy |
| Channel | `dvm_image_send_channel`, `dvm_image_channel` | real FlashVM channel with a counted ASCII `dis-hex:` binary envelope |
| Any host byte provider | `dvm_image_read` | callback handles short reads; collect exactly the framed size |

The hard upper bound is 8 MiB minus one byte, matching retained `readmod` policy;
caller quota can be smaller. Empty images are rejected. Output must initially be
`{0,0,0}`. Errors leave it empty and release partial native allocations.
`dvm_image_dispose` releases the owned bytes/name and is idempotent. File/socket
sources require an exact frame size from resource/protocol metadata; they do not
consume trailing bytes or invent a new network protocol. URL parsing, HTTP/TLS,
Styx version negotiation, source registries and executable resource discovery are
not supplied by these byte adapters. Embedded bytes are supported; a full ROM
filesystem/module-name namespace is still to be connected.

FlashVM strings are validated WTF-8, so arbitrary Dis cannot be stuffed into a
raw string. The channel helper encodes each byte as two ASCII hex characters
with prefix `dis-hex:`. Receiver validates framing, digits and decoded quota,
then releases the owned message. Nonblocking send/full or receive/empty returns
`DVM_IMAGE_PENDING`. Invalid/oversized received messages are consumed. The
producer/queue needs its own encoded-memory quota; decoded limits cannot prevent
a producer from allocating a huge channel message before reception.

`dvm_image_parse` requires an initialized Dis host with valid types, error frames,
GC and owner locking. It returns the real upstream Module, which must be released
through Dis `unload` on its owner. It is not a cross-heap capability and must never
be sent as a FlashVM value. The input image can be released after parsing. This
entry point does not itself initialize a VM, resolve imports or invoke an export.

## Native execution: current experiment

The chosen direction is real OS threads with per-thread register/process/runtime
state, not serial execution of every Limbo process in one native thread.
[Native contexts](../include/disvm/native_context.h) allocate distinct register
storage, bind exclusively via TLS, reject nested/concurrent binding, and require
explicit unbind before thread exit. The host must retain/root its process and
runtime for the complete bound lifetime. Context APIs do not own or collect those
VM objects. Destroy is allowed only after users are quiescent and the thread is
unbound/joined. No forced native-thread termination is provided.

Experimental full-core compilation:

```sh
DVM_NATIVE_REGISTERS=ON ./build-workspace.sh --projects disvm --tests
```

This opt-in generates build-only views of selected interpreter sources:
unqualified global `R` becomes `DVM_R`, while saved fields such as `p->R`, comments
and literals remain unchanged. The overlay resolves `DVM_R` through the currently
bound native context. Original sources and manifest hashes remain untouched.
Access without a bound register allocation of exactly `sizeof(REG)` aborts rather
than silently sharing global registers. Python is needed to generate this view.

**This is register isolation, not a completed parallel Dis runtime.** Global
module tables, GC/heap metadata, process groups, error/FPU state, native module
registration, pending calls, channel rendezvous and JIT publication still require
synchronization and integration. The selected scheduler/OS reference bodies remain
available for that work. There is no default parallel `spawn`, executable Dis
runner, safe shared-heap VM or successful IDE execution claim in this checkpoint.
Enabling TLS is not permission to run unsynchronized production GC/module paths.

## Evidence

The tests exercise real FlashVM file/socket/thread/synchronization implementations,
real binary channel transport linked to FlashVM, and copied/embedded byte input.
Two native threads execute 4000 retained production `addw` instruction bodies using
different real REG files, with a barrier at every operation. They do not hold one
global VM execution mutex. The production parser adapter is checked against the
retained upstream loader fixture; its explicit GC/JIT/allocator test doubles are
not presented as a complete VM. Windows/macOS/Android execution and full Dis/JIT
runtime acceptance remain open.

UBSan checks also pass for the TLS instruction fixture and changed channel queue paths. Address/LeakSanitizer attempts could not finalize in this environment (restricted `/proc` thread inspection); they are not counted as passing sanitizer acceptance.

## Extended up and collection safepoints (v0.1535)

The typed constructor [dvm_native_process_create](../include/disvm/native_process.h)
now allocates an extended native `up`: the retained `Proc` is its first member,
with the active `REG` embedded after it. `getup()` exposes the compatible Proc
prefix through the same TLS owner. The register adapter derives the active REG
from that up allocation; there is no second TLS key for R. The current process
accessor maps directly to `up->iprog`. Existing `Proc` error frames and OS/FPU
environment fields remain part of the same per-thread object. `Prog.R` still
holds the saved state of a suspended process, not the active native registers.

Use `dvm_native_process_create`, then `dvm_native_context_bind`, for interpreter
threads. The generic context constructor remains available for host-only state,
but is not a valid Inferno up and cannot drive the typed register adapter.
`native_registers.c` supplies getup for this selected host; do not link another
Inferno OS getup implementation into the same executable. Full Proc environment,
FPU setup, child groups and cleanup still require the remaining host integration.
No native-pointer width change or Dis bytecode/frame ABI change is introduced.

[Native worlds](../include/disvm/native_world.h) coordinate collector access:

1. A bound native owner joins a world, publishing its register/process root set.
   An installed poll hook prevents context unbind until explicit detach.
2. Enter marks an active mutator. Normal instructions run without the world mutex.
3. Poll between complete instructions parks if collection was requested. The
   experimental generated xec view inserts this poll immediately before decode,
   C-translated steps and leaf-JIT dispatch in the normal instruction loop.
4. Leave publishes stable roots before blocking in I/O/channels or going inactive.
   Reenter waits for collection to finish. Roots and referenced storage must remain
   live and unchanged while inactive. The host must integrate these calls around
   actual blocking runtime operations; they are not automatically wired yet.
5. A distinct bound collector calls stop_begin, visits all published root sets,
   then stop_end. Competing/nested collectors are rejected. The prior parked
   generation drains before another stop can begin. No forced OS suspension occurs.

The root visitor runs with the world mutex held: no reentry, blocking, exceptions,
longjmp, context destruction or root storage release is allowed. A real collector
must prepare fallible work outside this callback and pair every successful stop
with resume even on failure. Uncooperative instructions/native callbacks can delay
collection indefinitely; no timeout or asynchronous thread termination is claimed.
Legacy whole-module compiled paths have no internal poll instrumentation here.

This implements coordination/root publication, not production GC marking or sweep.
Global heaps, module tables, native modules, exceptions, scheduler/spawn and JIT
publication still need to obey the corresponding ownership/synchronization rules.
Do not enable unrestricted shared-heap parallel execution based on this test alone.

The new test runs two retained production addw bodies with extended up state and
100 stop/scan/resume cycles; each scan checks both real register/operand snapshots.
The earlier 4000-instruction test now also checks independent up error/process
fields. The generated-source test rejects an unrecognized dispatch boundary
rather than silently placing a safepoint in the wrong location.

[GC backend and native Fast/Cycles workers](GC.md).

[Versioned module namespace / Версии модулей](../MODULE-STORE.md).
