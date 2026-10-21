# Debugger commands and multiple observers

The C89 `rdebug.h` adapter registers an actual debugger resource, for example
`/debug/ctl`, in the existing owner dispatcher. Independent clients can observe
stops and query copied stack metadata. One authorized controller claims the
right to change breakpoints/resume. [Events](RESOURCE_EVENTS.md),
[wire envelopes](RESOURCE_DISPATCH.md), [executable tutorial](../examples/debugger/resource_debugger/README.md).

## Lifetime and API

The VM context, dispatcher and service userdata are borrowed. They must outlive
the adapter. No managed values, root ownership or context pointer is transferred
to a client. All VM work, registration, revoke and destruction stays on its owner
thread. Producers may only use the synchronized dispatcher/codec APIs. Create
does not replace an existing debugger hook: install it explicitly, and uninstall
it before destroying the adapter. Shared hosts must supply real dispatcher lock
and owner hooks; NULL policy is only single-owner/external-serialization use.

| Function/type | Contract |
| --- | --- |
| [`fvm_rdebug_create`](../include/flashvm/rdebug.h#L73) | Context, dispatcher, canonical path, required service callback, borrowed userdata, adapter output and resource output. Allocates native storage and registers six immutable operations. Both outputs unchanged on failure; INVALID for missing required arguments/path, NOMEM for allocation failure, registration statuses propagate. Does not install a VM hook or grant rights. |
| [`FVM_RDebugService`](../include/flashvm/rdebug.h#L68) | Runs synchronously in a stopped owner callback after available work is pumped. Return [FVM_OK](../include/flashvm/flashvm.h#L42) to pump again, another [FVM_Result](../include/flashvm/flashvm.h#L40) to abort execution. A real transport host should wait/service its event loop with cancellation and lost-wakeup protection. Do not reenter, collect, destroy this context or throw/longjmp through C. No implicit polling thread, socket, timeout or stdin. |
| [`fvm_rdebug_breakpoint`](../include/flashvm/rdebug.h#L83) | Install using `fvm_debug_set_breakpoint_handler(context, fvm_rdebug_breakpoint, adapter)`. Takes copied metadata from a legitimate VM stop, publishes STOP, then services queued commands until exact-stop RESUME or service failure. Wrong context/reentry INVALID_ARGUMENT. Stop/offset/line beyond Uint32 UNSUPPORTED. Never invoke it manually or on a transport thread. |
| [`fvm_rdebug_revoke`](../include/flashvm/rdebug.h#L79) | Owner revokes all rights on this resource for the given session and clears its controller claim. Dispatcher validation/statuses propagate. Call dispatcher_disconnect separately after transport quiescence for other resources/jobs. A stopped VM does not silently resume when a controller disappears. |
| [`fvm_rdebug_destroy`](../include/flashvm/rdebug.h#L88) | Uninstall the VM hook and quiesce users first. Pointer-to-owned-adapter required. BUSY while stopped/running its resource handler; otherwise unregisters, invalidates observers/old queued requests and frees native storage, setting pointer NULL. It never destroys the VM or dispatcher. |
| [`fvm_rdebug_snapshot_read`](../include/flashvm/rdebug.h#L91) | Client-side counted byte decoder for SNAPSHOT replies and STOP/RESUME/ABORT event payloads. Validates canonical integers, Uint32 bounds, version/state/flags, frame count, counted labels and exact extent. INVALID leaves output unchanged. |
| [`fvm_rdebug_frame_read`](../include/flashvm/rdebug.h#L93) | Client decoder for cached FRAME metadata. Validates version, booleans, engine, counted label and exact extent; INVALID leaves output unchanged. This is not a borrowed live activation. |

## Operations and binary payloads

All integer fields below use the canonical nonnegative S-integer format, including
booleans, lengths and version. Labels are counted bytes, not NUL-terminated wire
strings. The surrounding request/reply/event is an ordinary checked S-record.
Transport authentication is separate: a checksum and a session/stop number are
not authentication. The adapter never accepts a wire-supplied C function pointer.

| Operation | Right | Request and result |
| --- | --- | --- |
| [FVM_RDEBUG_SNAPSHOT](../include/flashvm/rdebug.h#L10) | READ | Empty request; returns latest copied snapshot, initially RUNNING/stop=0/frames=0. No live VM call. |
| [FVM_RDEBUG_FRAME](../include/flashvm/rdebug.h#L11) | READ | `expected_stop, index`; cached frame, including after resume. CONFLICT for another stop; UNKNOWN outside the captured prefix. |
| [FVM_RDEBUG_BREAKPOINT](../include/flashvm/rdebug.h#L12) | CONTROL + claim | `offset, enabled, name_size, name_bytes`; enabled is 0/1, name at most [FVM_RDEBUG_NAME](../include/flashvm/rdebug.h#L7), no embedded NUL; empty name selects top-level. Maps native errors to INVALID/NOMEM/HANDLER. |
| [FVM_RDEBUG_CLAIM](../include/flashvm/rdebug.h#L14) | CONTROL | Empty; first owner-FIFO claimant wins. Same-session repeat OK; another controller CONFLICT. No automatic lease or timeout. |
| [FVM_RDEBUG_RELEASE](../include/flashvm/rdebug.h#L15) | CONTROL + claim | Empty; relinquishes the claim. Another session DENIED. Does not resume execution. |
| [FVM_RDEBUG_RESUME](../include/flashvm/rdebug.h#L17) | CONTROL + claim | `expected_stop`; inactive callback BUSY, old/duplicate stop CONFLICT. OK acknowledges callback-release intent, not future completed instructions. |

SNAPSHOT fields: `version, state, flags, stop, offset, line, frames, controller,
function_size, source_size, function_bytes, source_bytes`. FRAME fields:
`version, truncated, engine, forced, offset, argc, registers, function_size,
function_bytes`. Metadata is limited to Uint32, 127 function bytes, 255 source
bytes and 16 innermost frames; truncation is explicit. Argument/register **values**,
locals, objects and machine/bytecode addresses are not included. Captured copies
survive GC outside the stop; they describe their stop, not later stack state.

STOP, RESUME and ABORT carry snapshot payloads. Per-reader sequence/loss follows
the events contract. A slow client must query the latest snapshot after loss.
Controller changes alone do not emit stop events; query SNAPSHOT to inspect the
current claim. Explicit host recovery is required after disconnect. A service
error or publication failure aborts the callback, publishes ABORT when possible
and leaves a queryable aborted snapshot. Error after accepted resume is not a
promise that the program continued. Breakpoints force original bytecode through
the existing interpreter fallback, including explicitly attached JIT bodies;
the adapter does not implement native-code deoptimization or asynchronous break.

## Event wire API and integration boundary

`fvm_revent_record_write(tag,event,output,capacity,used)` encodes a retained copied
event; it never dequeues a subscription. Metadata is
`version,target-slot,generation,sequence,kind,missed,size,bytes`. Payloads above
[FVM_R_BYTES](../include/flashvm/rdispatch.h#L9) return S_LIMIT; invalid IDs return S_INVALID; output/used stay unchanged
on failure. Retry output from the retained copy after I/O/capacity failure.

`fvm_revent_record_read(bindings,input,size,event,consumed)` requires the incoming
direction-local schema `flashvm.resource.event`, version 1. CRC/framing errors
propagate; unknown binding S_UNKNOWN, wrong schema/payload S_INVALID. Event and
consumed change only on S_OK. It does not grant rights or authenticate a sender.

An authenticated transport can feed requests through [`fvm_rdispatch_post_record`](../include/flashvm/rwire.h#L29)
and encode replies/events. The real test does so for debugger requests and both
observers' stop events. This is the common command/event boundary, not a supplied
network listener, [FVM_Channel](../src/capi/channel_c.c#L26) graph bridge, Styx/9P server or filesystem mount.
Do not broadcast private debug state indiscriminately. Different resource owners
can live in other processes, but this adapter owns exactly one VM context.

## Build and verification

```sh
cmake -S flashVM -B build-debug -DFVM_BUILD_EXAMPLES=ON
cmake --build build-debug --target flashvm_rdebug_contract flashvm_rdebug_failure flashvm_srecord_oracle flashvm_examples_debugger_resource_debugger_main
ctest --test-dir build-debug -R '^(flashvm_rdebug_|flashvm_revents_wire|flashvm_examples_debugger_resource_debugger_main)' --output-on-failure
```

The real C89 test exercises AVM1 execution, owner stop/frame queries, two observers,
competing control claims, stale/duplicate resumes, service abort, revoke, cached
frames after GC and safe detach with queued old-generation work. It requests
interpreter/automatic-JIT/manual modes and, when available, verifies actual native
entries after breakpoint removal. Independent Python vectors verify event bytes,
full 512-byte payloads, CRC, direction bindings and atomic errors. Native adapter
allocation failure is injected separately. An additional native POSIX fixture runs
two real debugger client threads while the VM owner blocks on a condition variable;
clients only access copied metadata and synchronized native queues. Build target
`flashvm_rdebug_threads` separately on POSIX and run its same-named CTest. This does not certify every debugger
feature, transport, target platform or roadmap stage.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Channel` | struct | [src/capi/channel_c.c:26](../src/capi/channel_c.c#L26) |
| `fvm_debug_set_breakpoint_handler` | function | [include/flashvm/debug.h:54](../include/flashvm/debug.h#L54) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_R_BYTES` | macro / define | [include/flashvm/rdispatch.h:9](../include/flashvm/rdispatch.h#L9) |
| `FVM_RDEBUG_BREAKPOINT` | macro / define | [include/flashvm/rdebug.h:12](../include/flashvm/rdebug.h#L12) |
| `fvm_rdebug_breakpoint` | function | [include/flashvm/rdebug.h:83](../include/flashvm/rdebug.h#L83) |
| `FVM_RDEBUG_CLAIM` | macro / define | [include/flashvm/rdebug.h:14](../include/flashvm/rdebug.h#L14) |
| `fvm_rdebug_create` | function | [include/flashvm/rdebug.h:73](../include/flashvm/rdebug.h#L73) |
| `fvm_rdebug_destroy` | function | [include/flashvm/rdebug.h:88](../include/flashvm/rdebug.h#L88) |
| `FVM_RDEBUG_FRAME` | macro / define | [include/flashvm/rdebug.h:11](../include/flashvm/rdebug.h#L11) |
| `fvm_rdebug_frame_read` | function | [include/flashvm/rdebug.h:93](../include/flashvm/rdebug.h#L93) |
| `FVM_RDEBUG_NAME` | macro / define | [include/flashvm/rdebug.h:7](../include/flashvm/rdebug.h#L7) |
| `FVM_RDEBUG_RELEASE` | macro / define | [include/flashvm/rdebug.h:15](../include/flashvm/rdebug.h#L15) |
| `FVM_RDEBUG_RESUME` | macro / define | [include/flashvm/rdebug.h:17](../include/flashvm/rdebug.h#L17) |
| `fvm_rdebug_revoke` | function | [include/flashvm/rdebug.h:79](../include/flashvm/rdebug.h#L79) |
| `FVM_RDEBUG_SNAPSHOT` | macro / define | [include/flashvm/rdebug.h:10](../include/flashvm/rdebug.h#L10) |
| `fvm_rdebug_snapshot_read` | function | [include/flashvm/rdebug.h:91](../include/flashvm/rdebug.h#L91) |
| `FVM_RDebugService` | callback typedef | [include/flashvm/rdebug.h:68](../include/flashvm/rdebug.h#L68) |
| `fvm_rdispatch_post_record` | function | [include/flashvm/rwire.h:29](../include/flashvm/rwire.h#L29) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:40](../include/flashvm/flashvm.h#L40) |
| `fvm_revent_record_read` | function | [include/flashvm/rwire.h:49](../include/flashvm/rwire.h#L49) |
| `fvm_revent_record_write` | function | [include/flashvm/rwire.h:45](../include/flashvm/rwire.h#L45) |
<!-- source-links:end -->
