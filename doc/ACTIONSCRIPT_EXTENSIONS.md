# ActionScript GC, workers and channels

## Installation and dispatch

Call `fvm_extensions_install(context)` before executing AVM1. It installs three
context-owned global namespace objects: `gc`, `thread`, `channel`. Installation
is idempotent when all three remain installed; conflicting or partially replaced
globals return `FVM_INVALID_ARGUMENT`, without overwriting user globals. Ordinary
contexts remain unchanged until installation. The C89 interface is declared in
`flashvm/extensions.h`; the implementation is C++98 and uses internal containers.

Methods bind to their receiver. A script function or global native function with
the same name cannot intercept an installed method. Current AVM1 supports named
functions rather than general function-valued properties or lexical closures.
Workers therefore accept a **function name string**. `CallMethod` executes these
methods directly; no compiler changes are needed for ordinary dot-call syntax.
Custom operators `<-`, `?-` and extended literals are not implemented.

## GC namespace

| Method | Result / contract |
| --- | --- |
| `gc.collect()` | Full object collection, returns number reclaimed |
| `gc.step()` | Threshold-triggered collection, returns number reclaimed |
| `gc.mode(mode)` | Set -100..100, suspended=101, full=102; full does one pass |
| `gc.state()` | Stored mode; one-shot full does not change it |
| `gc.count()` | Current live object count |

Only the trusted GC methods can collect during native method dispatch; their
receivers and arguments are rooted. Other native callbacks retain the previous
reentry guard. Calling GC from a native callback or an active finalizer is still
rejected or deferred according to the C API. See `GC_COMPLETION.md`.

## Thread namespace and handles

| Method | Contract |
| --- | --- |
| `thread.run(name, input?, sharedChannel?)` | Snapshot named functions and input, start private VM |
| `thread.run_suspended(...)` | Same snapshot, wait for start |
| `thread.join(handle, ...)` | Join all validated unique handles; one gives its result, several give an indexed result table |
| `thread.stopRequested()` | Cooperative cancellation state in current worker; false in host VM |
| `thread.sleep(seconds)` / `sleepms(ms)` / `sleepus(us)` | Nonnegative duration, best-effort OS sleep with cancellation checks |
| `handle.start()` | Idempotent start |
| `handle.stop()` | Request cancellation; also wakes suspended worker |
| `handle.state()` | 0 suspended, 1 running, 2 finished, 3 joined |
| `handle.join()` | Wait, return result; joins implicitly start suspended workers |
| `handle.ret()` | Undefined while unfinished, otherwise cached result |

Worker inputs/results support primitives and ordinary object graphs. Copies
preserve cycles and aliasing within each snapshot, with no shared VM objects.
Mutating input after `run_suspended` does not change the worker's input. Named
function definitions are copied at creation; host globals, arbitrary native
registrations and closures are not copied. Standard namespaces are installed in
the worker. An optional shared channel is published as `sharedChannel`; each VM
owns its own GC wrapper around the same retained native channel handle.

`join`/`ret` cache decoded object results on the worker wrapper, which traces them
as GC edges. Repeated results keep identity in the receiving VM. Dropping the
wrapper and collecting releases its result graph, requests stop and joins the
OS worker before releasing native storage. Context teardown performs the same
cleanup. AVM1 cancellation is checked at instruction safe points (every 64
instructions); builtin channel waits and sleeps also check it. Nested script-worker joins also poll cancellation and request stop on their child.
Arbitrary native callbacks cannot be forcibly interrupted. Explicitly
join workers when predictable cleanup latency matters. Self-join is rejected.

Worker errors propagate as `FVM_Result` from AVM1 execution / C method calls.
There is no ActionScript exception-object mechanism in this subset. Undefined
as `ret` can mean either unfinished work or an actual undefined result; inspect
state when the distinction matters. Multi-join validates all handles before
starting and joins remaining workers even when one reports an error.

## Channels

| Method | Contract |
| --- | --- |
| `channel.create(capacity?)` | Create wrapper; default / zero is unbounded |
| `ch.send(value)` | Wait for space, snapshot objects, return true |
| `ch.trySend(value)` | True on enqueue, false on full open queue |
| `ch.receive()` | Wait for message, copy into receiver VM |
| `ch.tryReceive()` | Undefined when empty/open; otherwise message |
| `ch.ready()` / `ch.pending()` | Readiness / queue-count snapshots |
| `ch.capacity(n)` | Set capacity; existing messages retained |
| `ch.close()` | Wake waiters; sends fail, queued messages remain drainable |

Empty closed queues and sends to closed queues return `FVM_ERROR`. Readiness
cannot guarantee the next receive will complete immediately with multiple
consumers. Undefined messages and empty `tryReceive` have the same return value;
use an envelope object if that distinction matters.

`fvm_extensions_wrap_channel(context, nativeHandle, &object)` provides an
embedding bridge. It retains the native handle; storing the wrapper in a global
or a pin keeps it alive. The wrapper releases its reference on reclamation.
Final native release must occur after all native users have stopped.

## Graph transfer C API

`flashvm/transfer.h` exposes `fvm_channel_send_value` and
`fvm_channel_receive_value`. Named/indexed properties, prototypes and metatables
are copied iteratively; cycles and repeated references survive. Source VM
reclamation/destruction after a successful send is safe. Maximum snapshot size
is 4096 objects; larger graphs return `FVM_UNSUPPORTED` before enqueueing.
Native callbacks, userdata, callable objects and builtin resource wrappers are
rejected anywhere in the graph. Share channels explicitly with the worker
argument or embedding bridge, rather than queueing native handles into cycles.

Primitive and graph messages share one FIFO. Legacy primitive receive returns
`FVM_UNSUPPORTED` without dequeuing when the head is a graph packet; use the new
receive API for mixed queues. Received strings belong to the destination context.
Snapshot failure does not enqueue a partial message. Receive consumes a packet;
on decode failure it reports an error, and partial ordinary objects are reclaimed
by a later collection. These are internal in-process snapshots, not a network
serialization format. TCP/UDP/TLS transports remain future work.

## Evidence and limits

`tests/actionscript_extensions.cpp` builds and executes real AVM1 instructions,
checks method dispatch, worker states, object/string results, input snapshot
isolation, shared channels, cached result roots, multi-join, cancellation and GC
cleanup. `channel_graph_transfer` checks cyclic graph identity, source destruction,
prototype/index edges, unsupported callbacks and legacy receive behavior.
`gc_deep_graph` marks and sweeps a 12,000-object cycle without recursive tracing.
`channel_close_races` exercises shutdown races for blocked producers/consumers.

See `VALIDATION.md` for actual commands, platforms and untested areas. This is a
tested extension to the documented AVM1 subset, not full Flash Player or arbitrary
ActionScript compatibility. Strings retain context lifetime; LeakSanitizer,
allocation-failure injection throughout the VM and native Windows/macOS/Android
runtime validation remain outstanding.
