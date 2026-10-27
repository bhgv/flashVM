# Broker diagnostics and portable clients

[DisVM](../README.md) · [IDE projection inspector](../../ide/dis/PROJECTIONS.md) ·
[Dynamic-value direction](../../fir/v1/doc/DYNAMIC_VALUES.md) · [Русский](README.ru.md)

The C99 broker tracks bounded channel sessions and outstanding requests. It does
not execute application objects. Native owners publish their current phase and
progress; diagnostic queries copy metadata under a short, host-supplied mutex.
They never call a paused target, perform network I/O, or wait for its reply.

## Implemented boundary

- Global subscription with channel/generation filters `(0, 0)` covers every
  registered external channel. Each watcher has an independent bounded queue.
- Only deviations generate events: slow operation, deadline, disconnection,
  debugger pause and owner failure. Successful replies and heartbeats are silent.
- Request keys include channel, session generation and exact uint64 request ID.
  Reconnection fences stale heartbeats/replies and preserves the old request's
  original connection card.
- Owner phases distinguish queued/running, disk, network, RPC, lock and debugger
  pause. The owner updates this metadata; the broker does not infer a cause.
- A copied source occurrence includes build identity, module, file, line, UTF-8
  byte column, byte offset/length and lexical binding. Creation/connection sites
  accompany transport kind, logical service and local/peer endpoint text.
- The optional `flash_channel` pump writes actual FlashVM channels using try-send.
  A blocked or closed output retains one pending frame; rebinding retries it.
  It never waits for the UI to consume an event.
- `brokerdiag.m/.b` decodes into owned typed Limbo values. `diagnostics.as` provides
  the AS decoder and a nonblocking channel helper. Neither evaluates object code.

Generic Flash-compatible Limbo `any`, network RPC, authentication, remote export
roots/finalizers and automatic instrumentation of every VM channel are **not**
implemented here. The IDE example currently uses separate Dis processes within
one VM, not a socket or a cross-heap channel pointer.

## Lifetime and scheduling

`broker.h` documents every operation. Create the broker with a separate native
metadata mutex, keep it alive while pumps/listeners exist, and call `tick` with a
monotonic clock. The no-op mutex in `frame_example.c` is valid only because that
example is single-threaded. A real adapter must retain its target's GC root until
the owner has acknowledged completion; the broker stores IDs, not heap roots.

A timeout/disconnect diagnoses loss of a reply; it does not prove cancellation.
`forget` remains busy until owner completion. Late completion is recorded but
returns stale, so adapters cannot publish it into the new session. Owner updates
are short metadata operations; never hold the broker mutex while running target
code. Cooperative application pause leaves the broker available. A debugger that
stops the entire OS process necessarily stops its broker too.

Limits are 16 channels, 128 sites, 128 requests, 8 listeners and 32 queued events
per listener. Repeated events of the same kind/key coalesce. Queue drops are
reported as a counter during polling; overflow never blocks the target. Handle
capacity errors explicitly and retire finished requests. No successful-exchange
stream is retained. Deadline time continues to advance while an owner is paused.

## BD1 diagnostic frame

The format is ASCII, bounded by `DVM_BROKER_FRAME_MAX` (8192 bytes), versioned by
`BD1`. Scalars are fixed-width lowercase hex: uint32 has 8 digits; uint64 has 16.
Texts are a uint32 byte count followed by twice that many hex digits encoding
strict UTF-8. There are no pointer fields, native padding or float-number IDs.
AS/Limbo clients retain wide IDs as exact hex strings.

The order is: kind, timestamp, channel, generation, request; parent channel,
generation, request; object ID; phase; flags; started/updated/progress; transport;
logical/local/peer/operation/detail texts; origin/creation/connection sites.
A site contains build/path/module texts, line/column, offset, length and binding.
Flags report timed out, disconnected and finished state. See `codec.h` for the
precise contract. This is a diagnostic frame, not an arbitrary AS value codec.
Malformed/truncated/trailing data is rejected without publishing a partial event.

## Build and exercise

```sh
cmake -S disVM/broker -B build/broker
cmake --build build/broker
ctest --test-dir build/broker --output-on-failure
build/broker/disvm_broker_frame > diagnostic.bd1
```

The frame producer registers a representative network connection **as metadata**;
it does not connect to that endpoint. Optional arguments specify an actual source
path, line, byte column, byte offset, byte length, binding ID, build ID and module.
Every example function documents its ownership and blocking behavior.

For actual FlashVM channel tests, configure an explicit
`-DDVM_BROKER_FLASHVM_ROOT=/absolute/path/to/flashVM/flashVM`.
For the language fixture additionally provide explicit `DVM_BROKER_SOURCE_COMPILER`,
`DVM_BROKER_NODE`, `DVM_BROKER_LIMBO`, `DVM_BROKER_RUNTIME` and
`DVM_BROKER_MODULES` paths. The Limbo compiler/runtime/header target must match.
These are development dependencies, not deployed IDE dependencies.

`test_clients.py` compiles repeated-name source with our Coco/R frontend, obtains
the actual occurrence, generates a C broker frame and decodes it in AS and Dis.
The Dis example reads a fixture file then transfers decoded records over real
local typed channels in interpreter/JIT modes. The AS decoder runs under Node as
an algorithm check; this does not prove full AS-to-SWF execution. Native tests
also hold an application mutex while a separate diagnostic thread queries the
broker, and exercise timeout ownership, generation fencing and queue saturation.

ASan/UBSan are supported. `DVM_BROKER_SANDBOX_LSAN=ON` disables only test-process
LSan inspection where `/proc` task scanning is unavailable; it does not establish
leak-sanitizer acceptance on that host. Production code has no sanitizer hooks.

[Owned long-operation lock contract](../OWNED_OPERATIONS.md): broker metadata locking is separate from payload protection.
