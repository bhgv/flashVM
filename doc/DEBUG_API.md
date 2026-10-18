# Debugger C API reference

C89 declarations live in `flashvm/debug.h`, `crash.h`, `source_debug.h`,
`exception.h`, `jit.h` and `execution_mode.h`. This reference covers every public
debug/crash/source inspection function, plus execution/JIT controls used by the
tutorials. General ownership and execution APIs retain their own header contracts.

All context operations run on the owning VM thread. Resource registry snapshots
have their own internal process locks; they do not suspend another context.
Borrowed views must be copied before their documented invalidation boundary.
A live fault callback cannot execute script, collect, mutate or destroy the VM;
a breakpoint callback additionally may edit breakpoints and future mode.
A fault callback returns by ordinary error unwinding, not instruction retry.
These APIs are not async-signal-safe OS crash handlers.

Use [examples/debugger](../examples/debugger/README.md) for runnable, line-commented
C89 tutorials. See [fault report contracts](CRASH_DIAGNOSTICS.md) and
[SWF/SWD source contracts](SWD_DEBUGGING.md) for formats, limitations and evidence.
The Russian reference repeats the exact parameter/result/ownership details below.

## Index / Указатель

- [fvm_debug_set_breakpoint](#fvm_debug_set_breakpoint) — Set or remove an exact action-offset breakpoint for a function name.
- [fvm_debug_set_breakpoint_handler](#fvm_debug_set_breakpoint_handler) — Register the synchronous interpreter breakpoint callback.
- [fvm_debug_set_location](#fvm_debug_set_location) — Set the host source label and line used for subsequent resource diagnostics.
- [fvm_debug_map_action](#fvm_debug_map_action) — Copy a manual function/action-to-source mapping.
- [fvm_debug_mutex_snapshot](#fvm_debug_mutex_snapshot) — Copy coherent process-local mutex owner/waiter rows.
- [fvm_debug_mutex_detail_snapshot](#fvm_debug_mutex_detail_snapshot) — Copy coherent process-local mutex owner/waiter rows with endpoint metadata.
- [fvm_debug_channel_freeze](#fvm_debug_channel_freeze) — Freeze or resume delivery without stopping producers.
- [fvm_debug_channel_state](#fvm_debug_channel_state) — Copy queue/file/remote delivery metadata without consuming a message.
- [fvm_debug_channel_message](#fvm_debug_channel_message) — Decode an independent snapshot of an unread message without dequeueing.
- [fvm_debug_channel_wait_snapshot](#fvm_debug_channel_wait_snapshot) — Copy active ActionScript channel wait rows.
- [fvm_debug_resource_errors](#fvm_debug_resource_errors) — Copy the bounded oldest-to-newest diagnostic event queue.
- [fvm_debug_exception_events](#fvm_debug_exception_events) — Copy the bounded oldest-to-newest diagnostic event queue.
- [fvm_debug_object_count](#fvm_debug_object_count) — Count managed heap objects.
- [fvm_debug_object_at](#fvm_debug_object_at) — Borrow one managed heap objects row by zero-based index.
- [fvm_debug_native_function_count](#fvm_debug_native_function_count) — Count global native-function registry entries.
- [fvm_debug_native_function_at](#fvm_debug_native_function_at) — Borrow one global native-function registry entries row by zero-based index.
- [fvm_debug_native_function_at_bytes](#fvm_debug_native_function_at_bytes) — Borrow a complete counted registry key.
- [fvm_debug_script_function_count](#fvm_debug_script_function_count) — Count named script-function registry entries.
- [fvm_debug_script_function_at](#fvm_debug_script_function_at) — Borrow one named script-function registry entries row by zero-based index.
- [fvm_debug_script_function_at_bytes](#fvm_debug_script_function_at_bytes) — Borrow a complete counted registry key.
- [fvm_crash_configure](#fvm_crash_configure) — Choose the first-chance fault action and optional file/callback target.
- [fvm_crash_set_detail](#fvm_crash_set_detail) — Select preservation cost independently from fault action.
- [fvm_crash_clear](#fvm_crash_clear) — Release the retained report without changing configured action/detail.
- [fvm_crash_info](#fvm_crash_info) — Query report size and original versus diagnostic status.
- [fvm_crash_frame](#fvm_crash_frame) — Borrow one copied postmortem activation, innermost first.
- [fvm_crash_frame_engine](#fvm_crash_frame_engine) — Classify interpreted, emitted JIT or embedding-owned native execution.
- [fvm_crash_source_line](#fvm_crash_source_line) — Borrow the copied executing source line resolved from SWD.
- [fvm_crash_data](#fvm_crash_data) — Borrow retained trace JSON or complete in-memory ZIP bytes.
- [fvm_crash_argument](#fvm_crash_argument) — Borrow a live argument while a fault/breakpoint debugger is paused.
- [fvm_crash_local](#fvm_crash_local) — Borrow a live local while a fault/breakpoint debugger is paused.
- [fvm_crash_register](#fvm_crash_register) — Borrow a live register while a fault/breakpoint debugger is paused.
- [fvm_debug_load_swd](#fvm_debug_load_swd) — Atomically attach a matching SWF/SWD source bundle.
- [fvm_debug_clear_symbols](#fvm_debug_clear_symbols) — Remove the currently attached source bundle.
- [fvm_debug_source_count](#fvm_debug_source_count) — Count source modules embedded in the attached SWD.
- [fvm_debug_source_at](#fvm_debug_source_at) — Borrow one complete embedded source module.
- [fvm_debug_source_position](#fvm_debug_source_position) — Resolve a copied action body and relative offset to source text.
- [fvm_debug_current_source](#fvm_debug_current_source) — Resolve the currently active VM action for a debugger callback.
- [fvm_debug_register_name](#fvm_debug_register_name) — Resolve an existing register slot to its SWD local name.
- [fvm_jit_source](#fvm_jit_source) — Borrow the immutable original AVM1 source for one native handle.
- [fvm_jit_machine_code](#fvm_jit_machine_code) — Borrow the immutable RX mapping payload for one native handle.
- [fvm_jit_available](#fvm_jit_available) — Query native compiler/runtime ABI availability.
- [fvm_jit_backend_name](#fvm_jit_backend_name) — Return the static backend ABI label.
- [fvm_jit_code_size](#fvm_jit_code_size) — Measure aggregate emitted bytes including child blocks.
- [fvm_execution_options_init](#fvm_execution_options_init) — Initialize explicit interpreter-first execution options.
- [fvm_execution_set_mode](#fvm_execution_set_mode) — Change policy for subsequent entries.
- [fvm_execution_get_options](#fvm_execution_get_options) — Read requested execution policy.
- [fvm_execution_set_logger](#fvm_execution_set_logger) — Install synchronous replay logging.
- [fvm_execution_get_stats](#fvm_execution_get_stats) — Copy execution/cache/replay counters.
- [fvm_avm1_execute_options](#fvm_avm1_execute_options) — Apply persistent options before a root action body.
- [fvm_function_invoke_options](#fvm_function_invoke_options) — Apply persistent options before an ordinary function invocation.
- [fvm_debug_frame_count](#fvm_debug_frame_count) — Count borrowed activations during a live debugger stop.
- [fvm_debug_frame_at](#fvm_debug_frame_at) — Borrow one live function frame without constructing a report.

## fvm_debug_set_breakpoint

Set or remove an exact action-offset breakpoint for a function name.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `context`: owning VM. `function`: terminated name; empty selects root actions. `offset`: byte offset inside the action body. `enabled`: exactly 0 or 1.

**Results / результаты.** `OK`: installed/removed or already in the requested state. `INVALID_ARGUMENT`: null/invalid context/name, destruction/GC-release phase or invalid enabled value. `OUT_OF_MEMORY`: allocating a new name/list failed.

**Lifetime / время жизни.** Function name is copied. No code/VM handle ownership is transferred.

**Behavior / поведение.** Every same-named body is excluded from JIT on its next entry, including an explicit JIT attachment. Running native frames are not patched or deoptimized. An arbitrary non-action offset can be registered but never hits; removing a missing point succeeds.


## fvm_debug_set_breakpoint_handler

Register the synchronous interpreter breakpoint callback.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `context`: owning VM. `handler`: callback or null to remove it. `userdata`: borrowed host state passed unchanged.

**Results / результаты.** `OK` on registration; `INVALID_ARGUMENT` for a null, destroying or GC-releasing context.

**Lifetime / время жизни.** Handler/userdata remain host-owned until replaced or context destruction. The location and its strings are callback-only views.

**Behavior / поведение.** The handler returns OK to continue interpretation, another status to stop this VM call. It may change breakpoints or future execution mode; it must not execute script, collect or destroy the context. Callbacks are recursion-suppressed. A source-aware stop can call [fvm_debug_current_source](../include/flashvm/source_debug.h#L39). Host callbacks that throw C++ exceptions are converted to OOM/ERROR.


## fvm_debug_set_location

Set the host source label and line used for subsequent resource diagnostics.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `context`: owning VM. `source`: terminated host source name. `line`: host line number, zero if unknown.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for null inputs/destroying context; `OUT_OF_MEMORY` if copying source fails.

**Lifetime / время жизни.** The source label is copied. This creates no file loader or source-content ownership.

**Behavior / поведение.** This changes current host location metadata; it does not add an action map or a breakpoint. Use [fvm_debug_map_action](../include/flashvm/debug.h#L63) for persistent body-offset mappings and load_swd for actual source contents.


## fvm_debug_map_action

Copy a manual function/action-to-source mapping.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `context`: owning VM. `function`: terminated name, empty for root. `offset`: relative action offset. `source`: terminated source label. `line`: mapped line.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for null inputs/destruction; `OUT_OF_MEMORY` for copied metadata.

**Lifetime / время жизни.** Names and source labels are copied into context-owned mapping storage.

**Behavior / поведение.** The nearest preceding mapping for the same function applies. Registering the same function/offset replaces its mapping. Same-named bodies share manual maps; manual report maps take precedence over SWD fallback. It does not store source text.


## fvm_debug_mutex_snapshot

Copy coherent process-local mutex owner/waiter rows.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `out`: caller-owned [FVM_DebugMutex](../include/flashvm/debug.h#L67) array, or null only with capacity zero. `capacity`: row capacity. `count`: required nonnull output for total needed rows.

**Results / результаты.** `OK` if all rows fit; `NOT_FOUND` if capacity is insufficient (count is still set, rows unchanged); `INVALID_ARGUMENT` for missing count or null array with nonzero capacity.

**Lifetime / время жизни.** All fields/strings are copied under the process registry lock; no borrowed mutex pointers escape. Caller owns output storage.

**Behavior / поведение.** Probe with null/zero, allocate, retry if another thread grows the registry. There is one row per waiter, or one waiter=0 row for an unawaited handle. No network request occurs. Deadlock fields describe the local wait graph; they do not certify a remote/distributed cycle. Names/endpoints use bounded struct arrays and may be truncated.


## fvm_debug_mutex_detail_snapshot

Copy coherent process-local mutex owner/waiter rows with endpoint metadata.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `out`: caller-owned [FVM_DebugMutexDetail](../include/flashvm/debug.h#L90) array, or null only with capacity zero. `capacity`: row capacity. `count`: required nonnull output for total needed rows.

**Results / результаты.** `OK` if all rows fit; `NOT_FOUND` if capacity is insufficient (count is still set, rows unchanged); `INVALID_ARGUMENT` for missing count or null array with nonzero capacity.

**Lifetime / время жизни.** All fields/strings are copied under the process registry lock; no borrowed mutex pointers escape. Caller owns output storage.

**Behavior / поведение.** Probe with null/zero, allocate, retry if another thread grows the registry. There is one row per waiter, or one waiter=0 row for an unawaited handle. No network request occurs. Deadlock fields describe the local wait graph; they do not certify a remote/distributed cycle. Names/endpoints use bounded struct arrays and may be truncated.


## fvm_debug_channel_freeze

Freeze or resume delivery without stopping producers.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `channel`: valid retained queue/file/compatible remote endpoint. `frozen`: zero to resume; any nonzero value freezes.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for null channel; remote transport/protocol failures propagate, including unsupported older broker controls.

**Lifetime / время жизни.** The handle remains caller-owned; keep it alive through the synchronous call.

**Behavior / поведение.** Local queues retain unread messages while frozen; local files pause reads and preserve cursors. Sending/writing remains possible. A closed frozen queue still needs resume before draining. Compatible remote brokers control authority-side freezing, but a remotely closed endpoint still rejects receive. Do not destroy concurrently used handles.


## fvm_debug_channel_state

Copy queue/file/remote delivery metadata without consuming a message.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `channel`: retained endpoint. `out`: required [FVM_DebugChannel](../include/flashvm/debug.h#L100) output struct (identity, pending, capacity, closed, frozen).

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for null inputs; transport, malformed framing, scan-budget or file-position errors otherwise. Failed scans leave output unchanged.

**Lifetime / время жизни.** The result is a copied observation, not a reservation or delivery guarantee.

**Behavior / поведение.** Local queue counters are read under its lock. A file reports complete unread framed records, capacity=0 and restores its cursor after scanning (65536 records / 64 MiB work). Closed file pending=0. Remote identity names the local proxy and remote counters are Uint32. Concurrent producers/consumers may change state immediately after return.


## fvm_debug_channel_message

Decode an independent snapshot of an unread message without dequeueing.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `context`: destination VM on its owning thread. `channel`: retained endpoint. `index`: zero-based unread message index. `out`: required value output.

**Results / результаты.** `OK`; `NOT_FOUND` for unavailable indexed messages; `INVALID_ARGUMENT` for invalid inputs; OOM, serialization/resource/protocol/closed/scan failures propagate. Failed decoding leaves output Undefined.

**Lifetime / время жизни.** Decoded managed values belong to context. Pin returned object graphs before any later GC. Remote capabilities remain live shared resources.

**Behavior / поведение.** File peeks restore the read cursor and have the same bounded scan policy as state inspection. This freezes neither live resource state nor an external process. Registered graph codecs/resource identity rules still apply. Inspection is synchronous and can perform remote transport; do not call it from a crash callback that forbids mutation/allocation of VM state.


## fvm_debug_channel_wait_snapshot

Copy active ActionScript channel wait rows.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `out`: caller-owned [FVM_DebugChannelWait](../include/flashvm/debug.h#L127) rows, nullable only with capacity zero. `capacity`: maximum rows. `count`: required needed-row output.

**Results / результаты.** `OK` if all rows fit; `NOT_FOUND` if capacity is insufficient (count updated, rows and queue unchanged); `INVALID_ARGUMENT` for invalid array/count arguments.

**Lifetime / время жизни.** Copied rows contain bounded source/function/address strings and numeric identities; no live VM/handle ownership is transferred.

**Behavior / поведение.** This is a coherent process registry observation of actual script send/receive waits. It does not infer channel deadlocks or enumerate every host blocking operation.


## fvm_debug_resource_errors

Copy the bounded oldest-to-newest diagnostic event queue.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `out`: caller-owned [FVM_DebugResourceError](../include/flashvm/debug.h#L143) rows, nullable only with capacity zero. `capacity`: maximum rows. `count`: required needed-row output. `clear`: nonzero removes the copied batch only after successful full copying.

**Results / результаты.** `OK` if all rows fit; `NOT_FOUND` if capacity is insufficient (count updated, rows and queue unchanged); `INVALID_ARGUMENT` for invalid array/count arguments.

**Lifetime / время жизни.** Copied rows contain bounded source/function/address strings and numeric identities; no live VM/handle ownership is transferred.

**Behavior / поведение.** The process-wide queue retains at most 64 events and drops the oldest on overflow. Probe/allocate/copy may need retry if new events arrive. clear=0 is nondestructive. Resource events are coarse intercept records (result can be [FVM_ERROR](../include/flashvm/flashvm.h#L43)); inspect structured exception info for richer fault codes.


## fvm_debug_exception_events

Copy the bounded oldest-to-newest diagnostic event queue.

Declaration / декларация: [`flashvm/exception.h`](../include/flashvm/exception.h).

**Parameters / параметры.** `out`: caller-owned [FVM_ExceptionInfo](../include/flashvm/exception.h#L41) rows, nullable only with capacity zero. `capacity`: maximum rows. `count`: required needed-row output. `clear`: nonzero removes the copied batch only after successful full copying.

**Results / результаты.** `OK` if all rows fit; `NOT_FOUND` if capacity is insufficient (count updated, rows and queue unchanged); `INVALID_ARGUMENT` for invalid array/count arguments.

**Lifetime / время жизни.** Copied rows contain bounded source/function/address strings and numeric identities; no live VM/handle ownership is transferred.

**Behavior / поведение.** The process-wide queue retains at most 64 events and drops the oldest on overflow. Probe/allocate/copy may need retry if new events arrive. clear=0 is nondestructive. This queue records structured resource exceptions, not every arbitrary script throw; use crash first-chance reports for script faults.


## fvm_debug_object_count

Count managed heap objects.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `context`: owning VM. `out`: required size_t count output.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` if either input is null.

**Lifetime / время жизни.** No ownership is transferred and the count is not a frozen snapshot across subsequent VM mutations.

**Behavior / поведение.** Call on the VM thread while collection/execution mutation is stopped. Heap enumeration includes classes/instances; use ordinary table/debug inspection cautiously and pin retained objects before GC.


## fvm_debug_object_at

Borrow one managed heap objects row by zero-based index.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `context`: owning VM. `index`: zero-based row below its current count. `out`: required [FVM_Object](../src/core/runtime.hpp#L118) ** object output.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for null arguments or an out-of-range index (legacy APIs do not use NOT_FOUND for end).

**Lifetime / время жизни.** The object is borrowed; enumeration does not pin it. Pin before retaining across GC.

**Behavior / поведение.** Use the corresponding _count result under a stable owning-thread observation. Numeric object indices can move after collection. Registry order is implementation storage order, not lexical sorting or a stable ID.


## fvm_debug_native_function_count

Count global native-function registry entries.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `context`: owning VM. `out`: required size_t count output.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` if either input is null.

**Lifetime / время жизни.** No ownership is transferred and the count is not a frozen snapshot across subsequent VM mutations.

**Behavior / поведение.** Call on the VM thread while collection/execution mutation is stopped. This is a specific registry, not every callable closure or first-class function in the heap. Borrowed registry keys can be invalidated by registry mutation.


## fvm_debug_native_function_at

Borrow one global native-function registry entries row by zero-based index.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `context`: owning VM. `index`: zero-based row below its current count. `out`: required const char ** terminated registry-name output.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for null arguments or an out-of-range index (legacy APIs do not use NOT_FOUND for end).

**Lifetime / время жизни.** The terminated name is borrowed until registry mutation/context destruction. Use the counted _at_bytes form for embedded NUL and complete byte identity.

**Behavior / поведение.** Use the corresponding _count result under a stable owning-thread observation. Numeric object indices can move after collection. Registry order is implementation storage order, not lexical sorting or a stable ID.


## fvm_debug_native_function_at_bytes

Borrow a complete counted registry key.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `context`: owning VM. `index`: current zero-based registry row. `out`: required const char ** name. `size`: required byte-count output.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for null arguments or an out-of-range index.

**Lifetime / время жизни.** The byte view is borrowed until registry mutation/context destruction; it is not a newly allocated string.

**Behavior / поведение.** Preserves embedded NUL and long names. Prefer this form in debugger UIs, copying the bytes if inspection will continue after mutation. Native and script registries remain distinct from heap function objects.


## fvm_debug_script_function_count

Count named script-function registry entries.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `context`: owning VM. `out`: required size_t count output.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` if either input is null.

**Lifetime / время жизни.** No ownership is transferred and the count is not a frozen snapshot across subsequent VM mutations.

**Behavior / поведение.** Call on the VM thread while collection/execution mutation is stopped. This is a specific registry, not every callable closure or first-class function in the heap. Borrowed registry keys can be invalidated by registry mutation.


## fvm_debug_script_function_at

Borrow one named script-function registry entries row by zero-based index.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `context`: owning VM. `index`: zero-based row below its current count. `out`: required const char ** terminated registry-name output.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for null arguments or an out-of-range index (legacy APIs do not use NOT_FOUND for end).

**Lifetime / время жизни.** The terminated name is borrowed until registry mutation/context destruction. Use the counted _at_bytes form for embedded NUL and complete byte identity.

**Behavior / поведение.** Use the corresponding _count result under a stable owning-thread observation. Numeric object indices can move after collection. Registry order is implementation storage order, not lexical sorting or a stable ID.


## fvm_debug_script_function_at_bytes

Borrow a complete counted registry key.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `context`: owning VM. `index`: current zero-based registry row. `out`: required const char ** name. `size`: required byte-count output.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for null arguments or an out-of-range index.

**Lifetime / время жизни.** The byte view is borrowed until registry mutation/context destruction; it is not a newly allocated string.

**Behavior / поведение.** Preserves embedded NUL and long names. Prefer this form in debugger UIs, copying the bytes if inspection will continue after mutation. Native and script registries remain distinct from heap function objects.


## fvm_crash_configure

Choose the first-chance fault action and optional file/callback target.

Declaration / декларация: [`flashvm/crash.h`](../include/flashvm/crash.h).

**Parameters / параметры.** `context`: owning VM outside execution. `mode`: NONE, BACKTRACE, ZIP, ZIP_CODE or DEBUGGER. `path`: optional terminated ZIP output path, null for memory-only. `handler`: required for DEBUGGER, otherwise unused. `userdata`: borrowed host state, invalid without a handler.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for invalid enum/arguments, live execution, destruction, GC-release or active diagnostic callback; `OUT_OF_MEMORY` if copying path fails. Failed path copy leaves old policy intact.

**Lifetime / время жизни.** The path is copied. Callback/state remain host-owned until replacement or destruction. Configuration does not clear an older retained report.

**Behavior / поведение.** First-chance means an enclosing Try may later handle the failure. ZIP paths are overwritten only when that capture reaches file writing; capture failure can leave an older file. Check diagnostic_status. Debugger return continues error unwinding, not instruction retry. No signal/SEH handler is installed.


## fvm_crash_set_detail

Select preservation cost independently from fault action.

Declaration / декларация: [`flashvm/crash.h`](../include/flashvm/crash.h).

**Parameters / параметры.** `context`: owning VM outside execution. `detail`: NATURAL or FORCE.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for invalid detail/context, live execution or an active callback/destruction/GC-release.

**Lifetime / время жизни.** Policy is context-owned; no argument values are copied by this configuration call.

**Behavior / поведение.** NATURAL borrows already rooted activation/argument state and adds no entry value copies or per-instruction local spills. FORCE copies supplied argument cells and strings at entry, adds GC roots and exposes formal register parameter names. Current JIT already materializes locals/registers and remains native. Object identity is retained, not a deep entry-time object graph snapshot; failed preservation is explicit in diagnostic_status/frame.forced.


## fvm_crash_clear

Release the retained report without changing configured action/detail.

Declaration / декларация: [`flashvm/crash.h`](../include/flashvm/crash.h).

**Parameters / параметры.** `context`: required owning VM, outside a diagnostic callback.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for null context or active diagnostic callback.

**Lifetime / время жизни.** All previously borrowed frame/JSON/ZIP/source-line views become invalid immediately.

**Behavior / поведение.** This does not clear an ActionScript pending exception and does not delete an output file. Use [fvm_exception_clear](../include/flashvm/exception.h#L65) for exception handling and host file APIs for deliberate file removal.


## fvm_crash_info

Query report size and original versus diagnostic status.

Declaration / декларация: [`flashvm/crash.h`](../include/flashvm/crash.h).

**Parameters / параметры.** `context`: owning VM. `frames`: required size_t output. `status`: required original VM result output. `diagnostic_status`: required capture/preservation/I/O result output.

**Results / результаты.** `OK` when a report exists; `NOT_FOUND` with frames=0 when absent; `INVALID_ARGUMENT` for null arguments. Diagnostic status is written even when report construction failed; original status is only defined on OK.

**Lifetime / время жизни.** Copied scalar outputs remain valid independently of subsequent report replacement.

**Behavior / поведение.** A VM EXCEPTION and diagnostic OK is a successful report of a failing script. OOM or section-limit exhaustion reports diagnostic OUT_OF_MEMORY; file/callback errors can report ERROR without replacing the VM failure. A live DEBUGGER callback still runs after capture allocation failure.


## fvm_crash_frame

Borrow one copied postmortem activation, innermost first.

Declaration / декларация: [`flashvm/crash.h`](../include/flashvm/crash.h).

**Parameters / параметры.** `context`: owning VM. `index`: retained frame index. `out`: required [FVM_CrashFrame](../include/flashvm/crash.h#L32) output struct.

**Results / результаты.** `OK`; `NOT_FOUND` if no report or index is beyond retained frames; `INVALID_ARGUMENT` for null context/output. Failed lookup leaves out unchanged.

**Lifetime / время жизни.** Name/source/source-bytecode/native-byte views belong to the report until clear/replacement/context destruction. They survive original function/code deletion and GC.

**Behavior / поведение.** offset is an AVM1 action offset, not a native PC. jit marks emitted code; query frame_engine to distinguish other engines. forced confirms entry preservation. machine_address is an opaque original RX address, never a pointer to dereference after code release; machine_code is a copied payload only for ZIP_CODE. Frames are bounded to 256 and JSON marks truncation.


## fvm_crash_frame_engine

Classify interpreted, emitted JIT or embedding-owned native execution.

Declaration / декларация: [`flashvm/crash.h`](../include/flashvm/crash.h).

**Parameters / параметры.** `context`: owning VM. `index`: retained frame. `out`: required [FVM_CrashEngine](../include/flashvm/crash.h#L25) output.

**Results / результаты.** `OK`; `NOT_FOUND` for an absent report/frame; `INVALID_ARGUMENT` for null inputs.

**Lifetime / время жизни.** The returned enum is a copied scalar.

**Behavior / поведение.** An arbitrary [FVM_ScriptBody](../include/flashvm/function.h#L36) attachment is NATIVE, while [fvm_jit_body](../include/flashvm/jit.h#L24) is JIT. Ordinary C stack frames/native callbacks are not a platform stack-unwind result. Native host bodies may have no exposed machine-byte view.


## fvm_crash_source_line

Borrow the copied executing source line resolved from SWD.

Declaration / декларация: [`flashvm/crash.h`](../include/flashvm/crash.h).

**Parameters / параметры.** `context`: owning VM. `index`: retained frame. `out`: required const char **. `size`: required byte-count output.

**Results / результаты.** `OK` for a known line, including an empty one; `NOT_FOUND` for missing report/frame/source resolution; `INVALID_ARGUMENT` for null arguments.

**Lifetime / время жизни.** The line copy belongs to the report and survives later clearing/replacing imported symbols.

**Behavior / поведение.** CR/LF terminators are excluded. A manual source label alone contains no source text. Source text is never fabricated from bytecode when an associated SWD is missing or ambiguous.


## fvm_crash_data

Borrow retained trace JSON or complete in-memory ZIP bytes.

Declaration / декларация: [`flashvm/crash.h`](../include/flashvm/crash.h).

**Parameters / параметры.** `context`: owning VM. `zip`: zero for JSON, nonzero for ZIP. `out`: required const void **. `size`: required byte count.

**Results / результаты.** `OK` if a report exists, including size=0 when no ZIP was requested/built; `NOT_FOUND` if no report; `INVALID_ARGUMENT` for null arguments.

**Lifetime / время жизни.** The bytes are borrowed until report clear/replacement/destruction; they are not NUL-terminated strings and must not be freed by the caller.

**Behavior / поведение.** Check info.diagnostic_status before claiming a file or ZIP succeeded. JSON escapes preserve original bytes, numbers are strings and object identities are diagnostic-only. ZIP includes raw object/class fields, bytecode and optional machine/source sections, not a restorable VM image.


## fvm_crash_argument

Borrow a live argument while a fault/breakpoint debugger is paused.

Declaration / декларация: [`flashvm/crash.h`](../include/flashvm/crash.h).

**Parameters / параметры.** `context`: live fault or breakpoint debugger callback VM. `frame`: innermost-first active frame index. `index`: zero-based supplied argument. `out`: required value output.

**Results / результаты.** `OK`; `NOT_FOUND` outside a fault/breakpoint callback or for an unavailable frame/index/bank; `INVALID_ARGUMENT` for null required inputs. Failed lookup leaves outputs unchanged.

**Lifetime / время жизни.** Value/object/string/name views are borrowed only until callback return. Do not collect, execute script, mutate or destroy context while using them.

**Behavior / поведение.** NATURAL sees the already supplied argument storage; FORCE sees preserved entry scalar/string values when frame.forced is true. Object contents are current, not deep entry snapshots. Original arguments are distinct from mutated parameter/register bindings.


## fvm_crash_local

Borrow a live local while a fault/breakpoint debugger is paused.

Declaration / декларация: [`flashvm/crash.h`](../include/flashvm/crash.h).

**Parameters / параметры.** `context`: live fault or breakpoint debugger callback VM. `frame`: active frame. `index`: zero-based available local row. `name`/`name_size`: required counted-name outputs. `out`: required value output.

**Results / результаты.** `OK`; `NOT_FOUND` outside a fault/breakpoint callback or for an unavailable frame/index/bank; `INVALID_ARGUMENT` for null required inputs. Failed lookup leaves outputs unchanged.

**Lifetime / время жизни.** Value/object/string/name views are borrowed only until callback return. Do not collect, execute script, mutate or destroy context while using them.

**Behavior / поведение.** Enumerates raw activation properties without getters. FORCE additionally exposes materialized Function2 register parameters under formal names after raw locals; these are diagnostic aliases, not new ActionScript bindings. Anonymous register locals can be inspected separately and named through SWD. Root activations may have no local object.


## fvm_crash_register

Borrow a live register while a fault/breakpoint debugger is paused.

Declaration / декларация: [`flashvm/crash.h`](../include/flashvm/crash.h).

**Parameters / параметры.** `context`: live fault or breakpoint debugger callback VM. `frame`: active frame. `index`: ABI register slot. `out`: required value output.

**Results / результаты.** `OK`; `NOT_FOUND` outside a fault/breakpoint callback or for an unavailable frame/index/bank; `INVALID_ARGUMENT` for null required inputs. Failed lookup leaves outputs unchanged.

**Lifetime / время жизни.** Value/object/string/name views are borrowed only until callback return. Do not collect, execute script, mutate or destroy context while using them.

**Behavior / поведение.** Reads already-maintained private/global register storage. No value spill is forced by the query. Use SWD register_name for a known source name; absent names remain unavailable. Values are not retained after the callback.


## fvm_debug_load_swd

Atomically attach a matching SWF/SWD source bundle.

Declaration / декларация: [`flashvm/source_debug.h`](../include/flashvm/source_debug.h).

**Parameters / параметры.** `context`: owning VM outside execution. `swf`/`swf_size`: complete FWS/CWS input. `swd`/`swd_size`: complete FWD6--8 data. All spans must remain valid for this call.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for invalid state/spans, malformed/truncated records, duplicate modules, conflicting lines or DebugID mismatch; `NOT_FOUND` if SWF has no DebugID; `UNSUPPORTED` for unknown/version/work-limit records; `OUT_OF_MEMORY` on allocation. Failed import keeps the previous bundle.

**Lifetime / время жизни.** Inputs are copied/decoded. One bundle is owned per context; a successful new load replaces the prior bundle and invalidates borrowed source views.

**Behavior / поведение.** SWD SCRIPT/OFFSET/ID/REGISTERS/BREAKPOINT/end records are parsed; breakpoint records are validated but not auto-enabled. ID must exactly match the 16-byte root SWF DebugID. CWS offsets refer to canonical uncompressed bytes. Source paths are labels only: no file/network access or automatic source retrieval. Limit: 16 MiB SWD, 65536 records; source bundles stay process-local and are not graph-transferred.


## fvm_debug_clear_symbols

Remove the currently attached source bundle.

Declaration / декларация: [`flashvm/source_debug.h`](../include/flashvm/source_debug.h).

**Parameters / параметры.** `context`: owning VM outside execution.

**Results / результаты.** `OK`, including no previously loaded bundle; `INVALID_ARGUMENT` for null/destroying context or live execution/diagnostic callback.

**Lifetime / время жизни.** All borrowed source/module/name views expire. Copied crash source lines and ZIP data remain valid until their report is cleared.

**Behavior / поведение.** Does not erase manual action maps, existing breakpoints or the loaded movie. A new SWF can be associated only through an explicit matching load_swd call.


## fvm_debug_source_count

Count source modules embedded in the attached SWD.

Declaration / декларация: [`flashvm/source_debug.h`](../include/flashvm/source_debug.h).

**Parameters / параметры.** `context`: owning VM. `count`: required size_t output.

**Results / результаты.** `OK` with zero when no bundle is loaded; `INVALID_ARGUMENT` for null inputs.

**Lifetime / время жизни.** The scalar count does not pin the bundle against a subsequent explicit replacement.

**Behavior / поведение.** Use source_at by ordinal to inspect module IDs, original filenames and complete embedded contents. No source files are opened.


## fvm_debug_source_at

Borrow one complete embedded source module.

Declaration / декларация: [`flashvm/source_debug.h`](../include/flashvm/source_debug.h).

**Parameters / параметры.** `context`: owning VM. `index`: zero-based module ordinal. `module_id`, `name`, `name_size`, `text`, `text_size`: required outputs.

**Results / результаты.** `OK`; `NOT_FOUND` without a bundle or beyond its module count; `INVALID_ARGUMENT` for null outputs/context.

**Lifetime / время жизни.** Names/content are borrowed counted byte views until symbol clear/replacement/context destruction; the module ID is a copied scalar.

**Behavior / поведение.** Text can be empty and may use CRLF/CR/LF. Keep byte lengths and decode text in the host UI as appropriate. This API shows the original supplied contents, not a decompiled substitute.


## fvm_debug_source_position

Resolve a copied action body and relative offset to source text.

Declaration / декларация: [`flashvm/source_debug.h`](../include/flashvm/source_debug.h).

**Parameters / параметры.** `context`: owning VM. `body`/`body_size`: exact AVM1 body bytes (original, JIT view or crash copy). `offset`: relative byte offset below body size. `out`: required [FVM_DebugSourcePosition](../include/flashvm/source_debug.h#L10).

**Results / результаты.** `OK`; `NOT_FOUND` for missing/unmapped/ambiguous source; `INVALID_ARGUMENT` for invalid spans/output/offset; `UNSUPPORTED` when bounded candidate/work limits are exceeded. Failed lookup leaves out unchanged.

**Lifetime / время жизни.** The returned source-name and line-text pointers belong to the attached bundle, not the body buffer; line excludes CR/LF.

**Behavior / поведение.** Lookup is cold and bounded, based on exact bytes in the DebugID-associated canonical SWF plus SWD absolute offsets. Identical bodies with conflicting locations are rejected instead of showing another function file. Limits: 4096 body candidates and 128 MiB compared work per query. No bytecode-to-source scan is added to the instruction hot path.


## fvm_debug_current_source

Resolve the currently active VM action for a debugger callback.

Declaration / декларация: [`flashvm/source_debug.h`](../include/flashvm/source_debug.h).

**Parameters / параметры.** `context`: owning VM during a breakpoint/debugger/native host callback. `out`: required [FVM_DebugSourcePosition](../include/flashvm/source_debug.h#L10).

**Results / результаты.** `OK`; `NOT_FOUND` without an active source-bearing frame/bundle; lookup INVALID_ARGUMENT/UNSUPPORTED outcomes otherwise.

**Lifetime / время жизни.** Returned source views are borrowed from the symbol bundle; use them only while that bundle remains loaded.

**Behavior / поведение.** Both interpreted and JIT activations publish source associations when symbols are attached. Ordinary breakpoint callbacks receive SWD filename/line automatically on a hit. Lookup happens on inspection/hit, not every instruction. It does not asynchronously suspend another VM thread.


## fvm_debug_register_name

Resolve an existing register slot to its SWD local name.

Declaration / декларация: [`flashvm/source_debug.h`](../include/flashvm/source_debug.h).

**Parameters / параметры.** `context`: owning VM. `body`/`body_size`/`offset`: same exact-body query as source_position. `reg`: ABI register number. `name`/`name_size`: required counted-name outputs.

**Results / результаты.** `OK`; `NOT_FOUND` for absent/ambiguous names, body origins or out-of-scope records; INVALID_ARGUMENT/UNSUPPORTED for invalid/bounded lookup outcomes.

**Lifetime / время жизни.** The returned name is borrowed from the bundle until clear/replacement/destruction.

**Behavior / поведение.** Uses the latest applicable register declaration inside a unique matched body span. It neither materializes a missing value nor guesses unknown names. Pair with crash_register during a live stop. Register records from another identical body are rejected conservatively.


## fvm_jit_source

Borrow the immutable original AVM1 source for one native handle.

Declaration / декларация: [`flashvm/jit.h`](../include/flashvm/jit.h).

**Parameters / параметры.** `code`: valid unreleased [FVM_JitCode](../src/jit/internal.hpp#L59). `out`: required const void **. `size`: required byte-count output.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for null handle/outputs. An invalid freed handle is a host ownership violation, not safely detectable input.

**Lifetime / время жизни.** Borrowed bytes live until [fvm_jit_destroy](../include/flashvm/jit.h#L28) and must not be written or freed. Source views are separate from copied report views.

**Behavior / поведение.** Source is the exact owned copy used for compilation; use it with source_position or host disassembly tooling.


## fvm_jit_machine_code

Borrow the immutable RX mapping payload for one native handle.

Declaration / декларация: [`flashvm/jit.h`](../include/flashvm/jit.h).

**Parameters / параметры.** `code`: valid unreleased [FVM_JitCode](../src/jit/internal.hpp#L59). `out`: required const void **. `size`: required byte-count output.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for null handle/outputs. An invalid freed handle is a host ownership violation, not safely detectable input.

**Lifetime / время жизни.** Borrowed bytes live until [fvm_jit_destroy](../include/flashvm/jit.h#L28) and must not be written or freed. Source views are separate from copied report views.

**Behavior / поведение.** Mapping view excludes page padding and nested mappings. It identifies emitted bytes, not source-line/native-PC relocation tables. Active structured ZIP_CODE reports capture the actual failing child mapping.


## fvm_jit_available

Query native compiler/runtime ABI availability.

Declaration / декларация: [`flashvm/jit.h`](../include/flashvm/jit.h).

**Parameters / параметры.** No arguments.

**Results / результаты.** Returns 1 only when the selected native ABI, function-pointer width and executable-memory facility are supported; otherwise 0. Opt-in macOS ARM64 also queries per-thread JIT protection support. This does not allocate a code slot or prove MAP_JIT/signing permission.

**Lifetime / время жизни.** No code-page allocation or ownership transfer. The availability query does not prepare or attach a function.

**Behavior / поведение.** Checks build/ABI/facility availability, not AVM1 coverage or host signing permission. Compilation can still return UNSUPPORTED or OUT_OF_MEMORY. macOS ARM64 is disabled by default and requires the documented deployment/SDK opt-in. Наличие ABI не удостоверяет разрешение ОС или исполнение конкретного тела; см. [ARM64 policy](JIT_MACOS_ARM64.md) и [JIT contracts](JIT.md).


## fvm_jit_backend_name

Return the static backend ABI label.

Declaration / декларация: [`flashvm/jit.h`](../include/flashvm/jit.h).

**Parameters / параметры.** No arguments.

**Results / результаты.** Returns a static terminated label, including unsupported.

**Lifetime / время жизни.** No ownership transfer or allocation. Backend label is static process data.

**Behavior / поведение.** Identifies the compiled ABI, including macos-arm64 for its enabled target. The label can exist even when the OS denies executable publication; it is not an execution counter. Use [fvm_execution_get_stats](../include/flashvm/execution_mode.h#L71) and native_entries to prove native execution. Имя backend не удостоверяет успешную компиляцию или нативный вызов; см. [JIT contracts](JIT.md).


## fvm_jit_code_size

Measure aggregate emitted bytes including child blocks.

Declaration / декларация: [`flashvm/jit.h`](../include/flashvm/jit.h).

**Parameters / параметры.** `code`: native handle, or null for zero.

**Results / результаты.** Returns a size_t byte count; zero for null.

**Lifetime / время жизни.** No ownership transfer or allocation. Backend label is static process data.

**Behavior / поведение.** See [JIT ownership/compilation](JIT.md) for compile, attach, execute and destroy contracts. Availability is not a promise that every AVM1 body compiles or that executable allocation cannot later fail.


## fvm_execution_options_init

Initialize explicit interpreter-first execution options.

Declaration / декларация: [`flashvm/execution_mode.h`](../include/flashvm/execution_mode.h).

**Parameters / параметры.** `out`: required options structure.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for null output.

**Lifetime / время жизни.** No new ownership transfer beyond the ordinary owning-context API; callbacks/userdata are borrowed and scalar outputs are copied.

**Behavior / поведение.** Sets struct_size, INTERPRETER and flags=0 independently of the DEFS context default. See [execution modes](EXECUTION_MODES.md).


## fvm_execution_set_mode

Change policy for subsequent entries.

Declaration / декларация: [`flashvm/execution_mode.h`](../include/flashvm/execution_mode.h).

**Parameters / параметры.** `context`: owning VM. `mode`: MANUAL/INTERPRETER/JIT. `flags`: zero or REPLAY_ON_EXCEPTION.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for invalid mode/flags/context phase.

**Lifetime / время жизни.** No new ownership transfer beyond the ordinary owning-context API; callbacks/userdata are borrowed and scalar outputs are copied.

**Behavior / поведение.** Active native code is not patched. Interpreter bypasses [fvm_jit_body](../include/flashvm/jit.h#L24) attachments; arbitrary host bodies retain their own ABI. Replay can repeat side effects and is not rollback. See [execution modes](EXECUTION_MODES.md).


## fvm_execution_get_options

Read requested execution policy.

Declaration / декларация: [`flashvm/execution_mode.h`](../include/flashvm/execution_mode.h).

**Parameters / параметры.** `context`: owning VM. `out`: required options with struct_size initialized.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for invalid context/output/struct size.

**Lifetime / время жизни.** No new ownership transfer beyond the ordinary owning-context API; callbacks/userdata are borrowed and scalar outputs are copied.

**Behavior / поведение.** Shows requested policy even when diagnostic replay temporarily forces interpretation. See [execution modes](EXECUTION_MODES.md).


## fvm_execution_set_logger

Install synchronous replay logging.

Declaration / декларация: [`flashvm/execution_mode.h`](../include/flashvm/execution_mode.h).

**Parameters / параметры.** `context`: owning VM. `logger`: callback, or null for stderr. `userdata`: borrowed host state, invalid without a callback.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for invalid state/arguments.

**Lifetime / время жизни.** No new ownership transfer beyond the ordinary owning-context API; callbacks/userdata are borrowed and scalar outputs are copied.

**Behavior / поведение.** Logger may query stats/change future mode but must not execute script, collect, throw or destroy VM. Ordinary native execution has no per-instruction logger calls. See [execution modes](EXECUTION_MODES.md).


## fvm_execution_get_stats

Copy execution/cache/replay counters.

Declaration / декларация: [`flashvm/execution_mode.h`](../include/flashvm/execution_mode.h).

**Parameters / параметры.** `context`: owning VM. `out`: required stats with struct_size initialized.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for invalid context/output/struct size.

**Lifetime / время жизни.** No new ownership transfer beyond the ordinary owning-context API; callbacks/userdata are borrowed and scalar outputs are copied.

**Behavior / поведение.** Counters wrap according to unsigned long width. last_replay_status describes the retry, not the original returned exception. See [execution modes](EXECUTION_MODES.md).


## fvm_avm1_execute_options

Apply persistent options before a root action body.

Declaration / декларация: [`flashvm/execution_mode.h`](../include/flashvm/execution_mode.h).

**Parameters / параметры.** `context`, `data`, `size`: ordinary root-execution inputs. `options`: null to retain current policy, or initialized explicit options.

**Results / результаты.** Option validation errors occur before execution; ordinary AVM1 statuses then propagate.

**Lifetime / время жизни.** No new ownership transfer beyond the ordinary owning-context API; callbacks/userdata are borrowed and scalar outputs are copied.

**Behavior / поведение.** Options persist in the context after this call. Invalid options do not execute source. See [execution modes](EXECUTION_MODES.md).


## fvm_function_invoke_options

Apply persistent options before an ordinary function invocation.

Declaration / декларация: [`flashvm/execution_mode.h`](../include/flashvm/execution_mode.h).

**Parameters / параметры.** `context`, `function`, `receiver`, `argc`, `argv`, `out`: ordinary invocation inputs; `options`: optional initialized policy.

**Results / результаты.** Option validation errors occur before invocation; ordinary function statuses then propagate.

**Lifetime / время жизни.** No new ownership transfer beyond the ordinary owning-context API; callbacks/userdata are borrowed and scalar outputs are copied.

**Behavior / поведение.** Arguments/result/receiver follow ordinary function ownership. Null options preserve policy; the existing activation and GC path remains authoritative. See [execution modes](EXECUTION_MODES.md).


## fvm_debug_frame_count

Count borrowed activations during a live debugger stop.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `context`: owning VM in a breakpoint or fault debugger callback. `count`: required size_t output.

**Results / результаты.** `OK`; `NOT_FOUND` with count=0 outside a live stop; `INVALID_ARGUMENT` for null context/output.

**Lifetime / время жизни.** Count is copied; frame views remain live only until the callback returns.

**Behavior / поведение.** Up to 256 frames are exposed, innermost first. The API requires no postmortem report and therefore works with crash action NONE. Pair with frame_at and live argument/local/register queries; it does not asynchronously suspend a running thread.


## fvm_debug_frame_at

Borrow one live function frame without constructing a report.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `context`: owning VM inside a breakpoint or fault debugger callback. `index`: innermost-first frame. `out`: required [FVM_DebugFrame](../include/flashvm/debug.h#L16) view.

**Results / результаты.** `OK`; `NOT_FOUND` outside a stop or beyond the live/capped frame range; `INVALID_ARGUMENT` for null arguments.

**Lifetime / время жизни.** Function label, AVM1 and native mapping views are borrowed until callback return. They are not owned copies and cannot be freed/written.

**Behavior / поведение.** Fields expose actual engine, supplied argc, existing register count, current/suspended action offset, forced-entry success and preservation_status. Use source_position on its body/offset for caller source lines. Inspection adds no local spill and invokes no script. The native mapping is protected while active; do not destroy or mutate VM/code during the callback.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_avm1_execute_options` | function | [include/flashvm/execution_mode.h:73](../include/flashvm/execution_mode.h#L73) |
| `fvm_crash_argument` | function | [include/flashvm/crash.h:77](../include/flashvm/crash.h#L77) |
| `fvm_crash_clear` | function | [include/flashvm/crash.h:62](../include/flashvm/crash.h#L62) |
| `fvm_crash_configure` | function | [include/flashvm/crash.h:55](../include/flashvm/crash.h#L55) |
| `fvm_crash_data` | function | [include/flashvm/crash.h:74](../include/flashvm/crash.h#L74) |
| `fvm_crash_frame` | function | [include/flashvm/crash.h:68](../include/flashvm/crash.h#L68) |
| `fvm_crash_frame_engine` | function | [include/flashvm/crash.h:70](../include/flashvm/crash.h#L70) |
| `fvm_crash_info` | function | [include/flashvm/crash.h:65](../include/flashvm/crash.h#L65) |
| `fvm_crash_local` | function | [include/flashvm/crash.h:79](../include/flashvm/crash.h#L79) |
| `fvm_crash_register` | function | [include/flashvm/crash.h:82](../include/flashvm/crash.h#L82) |
| `fvm_crash_set_detail` | function | [include/flashvm/crash.h:60](../include/flashvm/crash.h#L60) |
| `fvm_crash_source_line` | function | [include/flashvm/crash.h:72](../include/flashvm/crash.h#L72) |
| `FVM_CrashEngine` | enum | [include/flashvm/crash.h:25](../include/flashvm/crash.h#L25) |
| `FVM_CrashFrame` | struct | [include/flashvm/crash.h:32](../include/flashvm/crash.h#L32) |
| `fvm_debug_channel_freeze` | function | [include/flashvm/debug.h:114](../include/flashvm/debug.h#L114) |
| `fvm_debug_channel_message` | function | [include/flashvm/debug.h:125](../include/flashvm/debug.h#L125) |
| `fvm_debug_channel_state` | function | [include/flashvm/debug.h:118](../include/flashvm/debug.h#L118) |
| `fvm_debug_channel_wait_snapshot` | function | [include/flashvm/debug.h:139](../include/flashvm/debug.h#L139) |
| `fvm_debug_clear_symbols` | function | [include/flashvm/source_debug.h:25](../include/flashvm/source_debug.h#L25) |
| `fvm_debug_current_source` | function | [include/flashvm/source_debug.h:39](../include/flashvm/source_debug.h#L39) |
| `fvm_debug_exception_events` | function | [include/flashvm/exception.h:70](../include/flashvm/exception.h#L70) |
| `fvm_debug_frame_at` | function | [include/flashvm/debug.h:34](../include/flashvm/debug.h#L34) |
| `fvm_debug_frame_count` | function | [include/flashvm/debug.h:32](../include/flashvm/debug.h#L32) |
| `fvm_debug_load_swd` | function | [include/flashvm/source_debug.h:22](../include/flashvm/source_debug.h#L22) |
| `fvm_debug_map_action` | function | [include/flashvm/debug.h:63](../include/flashvm/debug.h#L63) |
| `fvm_debug_mutex_detail_snapshot` | function | [include/flashvm/debug.h:98](../include/flashvm/debug.h#L98) |
| `fvm_debug_mutex_snapshot` | function | [include/flashvm/debug.h:87](../include/flashvm/debug.h#L87) |
| `fvm_debug_native_function_at` | function | [include/flashvm/debug.h:166](../include/flashvm/debug.h#L166) |
| `fvm_debug_native_function_at_bytes` | function | [include/flashvm/debug.h:172](../include/flashvm/debug.h#L172) |
| `fvm_debug_native_function_count` | function | [include/flashvm/debug.h:164](../include/flashvm/debug.h#L164) |
| `fvm_debug_object_at` | function | [include/flashvm/debug.h:162](../include/flashvm/debug.h#L162) |
| `fvm_debug_object_count` | function | [include/flashvm/debug.h:160](../include/flashvm/debug.h#L160) |
| `fvm_debug_register_name` | function | [include/flashvm/source_debug.h:41](../include/flashvm/source_debug.h#L41) |
| `fvm_debug_resource_errors` | function | [include/flashvm/debug.h:156](../include/flashvm/debug.h#L156) |
| `fvm_debug_script_function_at` | function | [include/flashvm/debug.h:170](../include/flashvm/debug.h#L170) |
| `fvm_debug_script_function_at_bytes` | function | [include/flashvm/debug.h:175](../include/flashvm/debug.h#L175) |
| `fvm_debug_script_function_count` | function | [include/flashvm/debug.h:168](../include/flashvm/debug.h#L168) |
| `fvm_debug_set_breakpoint` | function | [include/flashvm/debug.h:51](../include/flashvm/debug.h#L51) |
| `fvm_debug_set_breakpoint_handler` | function | [include/flashvm/debug.h:54](../include/flashvm/debug.h#L54) |
| `fvm_debug_set_location` | function | [include/flashvm/debug.h:59](../include/flashvm/debug.h#L59) |
| `fvm_debug_source_at` | function | [include/flashvm/source_debug.h:29](../include/flashvm/source_debug.h#L29) |
| `fvm_debug_source_count` | function | [include/flashvm/source_debug.h:27](../include/flashvm/source_debug.h#L27) |
| `fvm_debug_source_position` | function | [include/flashvm/source_debug.h:35](../include/flashvm/source_debug.h#L35) |
| `FVM_DebugChannel` | struct | [include/flashvm/debug.h:100](../include/flashvm/debug.h#L100) |
| `FVM_DebugChannelWait` | struct | [include/flashvm/debug.h:127](../include/flashvm/debug.h#L127) |
| `FVM_DebugFrame` | struct | [include/flashvm/debug.h:16](../include/flashvm/debug.h#L16) |
| `FVM_DebugMutex` | struct | [include/flashvm/debug.h:67](../include/flashvm/debug.h#L67) |
| `FVM_DebugMutexDetail` | struct | [include/flashvm/debug.h:90](../include/flashvm/debug.h#L90) |
| `FVM_DebugResourceError` | struct | [include/flashvm/debug.h:143](../include/flashvm/debug.h#L143) |
| `FVM_DebugSourcePosition` | struct | [include/flashvm/source_debug.h:10](../include/flashvm/source_debug.h#L10) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `fvm_exception_clear` | function | [include/flashvm/exception.h:65](../include/flashvm/exception.h#L65) |
| `FVM_ExceptionInfo` | struct | [include/flashvm/exception.h:41](../include/flashvm/exception.h#L41) |
| `fvm_execution_get_options` | function | [include/flashvm/execution_mode.h:67](../include/flashvm/execution_mode.h#L67) |
| `fvm_execution_get_stats` | function | [include/flashvm/execution_mode.h:71](../include/flashvm/execution_mode.h#L71) |
| `fvm_execution_options_init` | function | [include/flashvm/execution_mode.h:61](../include/flashvm/execution_mode.h#L61) |
| `fvm_execution_set_logger` | function | [include/flashvm/execution_mode.h:69](../include/flashvm/execution_mode.h#L69) |
| `fvm_execution_set_mode` | function | [include/flashvm/execution_mode.h:64](../include/flashvm/execution_mode.h#L64) |
| `fvm_function_invoke_options` | function | [include/flashvm/execution_mode.h:76](../include/flashvm/execution_mode.h#L76) |
| `fvm_jit_available` | function | [include/flashvm/jit.h:13](../include/flashvm/jit.h#L13) |
| `fvm_jit_backend_name` | function | [include/flashvm/jit.h:15](../include/flashvm/jit.h#L15) |
| `fvm_jit_body` | function | [include/flashvm/jit.h:24](../include/flashvm/jit.h#L24) |
| `fvm_jit_code_size` | function | [include/flashvm/jit.h:35](../include/flashvm/jit.h#L35) |
| `fvm_jit_destroy` | function | [include/flashvm/jit.h:28](../include/flashvm/jit.h#L28) |
| `fvm_jit_machine_code` | function | [include/flashvm/jit.h:33](../include/flashvm/jit.h#L33) |
| `fvm_jit_source` | function | [include/flashvm/jit.h:31](../include/flashvm/jit.h#L31) |
| `FVM_JitCode` | struct | [src/jit/internal.hpp:59](../src/jit/internal.hpp#L59) |
| `FVM_Object` | struct | [src/core/runtime.hpp:118](../src/core/runtime.hpp#L118) |
| `FVM_ScriptBody` | callback typedef | [include/flashvm/function.h:36](../include/flashvm/function.h#L36) |
<!-- source-links:end -->
