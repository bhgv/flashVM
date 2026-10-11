# Shared mutexes, channels and debugger (development v0.5)

Install `fvm_extensions_install` before executing AVM1. The opt-in namespaces
are `gc`, `thread`, `channel`, `metatable`, and `mutex`. See
[ActionScript example](../examples/actionscript/shared_mutex.as) and
[C89 embedding example](../examples/embedding/shared_mutex/main.c).
The `.as` example describes the API; FlashVM has no ActionScript source compiler.
`tests/actionscript_mutex.cpp` executes equivalent operations as real AVM1 bytes.

## Mutex contract

| Member | Behavior |
|---|---|
| `mutex.new()` | New unlocked mutex, with a context-local GC wrapper |
| `mutex.list()` | Indexed table of this interpreter's existing live wrappers, with `length` |
| `state` | Read-only accessor: numeric 1 while owned, otherwise 0 |
| `origin` | `"this"` in its creating interpreter; otherwise its numeric interpreter/thread ID |
| `set()` | Acquire; block cooperatively until available; return true on success |
| `reset()` | Release; only the owning interpreter may release; return true on success |
| `touch()` | Boolean ownership snapshot, not an atomic acquire |
| `thread.id()` | Current interpreter ID |
| `worker.id` | Read-only ID reserved before the script worker starts |

IDs are process-local integers in 1..4294967295, exactly representable as
ActionScript numbers. They are never reused. Native worker callbacks obtain the
same context ID using `fvm_context_identity`. Host contexts also have distinct
IDs. A VM must only be executed by one native thread at a time.

Mutexes are nonrecursive. Reacquiring an owned mutex, releasing an unlocked
mutex, or releasing another interpreter's mutex returns `FVM_ERROR`. Native
`fvm_mutex_set(context, mutex, 0)` returns `FVM_NOT_FOUND` on contention.
Acquisition polls at 1 ms and observes a bound worker's cooperative stop request.
No fairness or real-time scheduling guarantee is made. Holders must release in
normal code; context teardown releases its remaining ownership after the worker
callback exits. GC of a wrapper alone never unlocks a critical section.

`list` covers the interpreter's allocated wrappers, including wrappers which
have not yet been collected. It does not inspect lexical scopes or grant access
to every process-wide mutex. Repeated wrapping/receipt of a native handle in a
context returns its existing wrapper. A returned list keeps its entries alive
when that list is rooted. C local variables still need explicit GC roots/pins.

## Transfer and lifetime

`fvm_channel_send_value` and ActionScript channel methods transfer mutex/channel
capabilities through in-process queues, including capabilities nested inside
cyclic ordinary object graphs and repeated aliases. A packet retains native
handles, not source VM objects. A destination VM creates its own canonical
wrapper; the underlying mutex state or channel FIFO is shared. Worker inputs,
returns and shared channels use this same transport.

Resource wrappers are capabilities: user-added wrapper properties, prototypes,
metatables and replaced methods are local decorations and are not copied with
that capability. Ordinary surrounding tables and their metadata still use graph
serialization. Keep transferable application data in the surrounding table.

Queue-owned channel capability cycles are rejected with `FVM_UNSUPPORTED`:
sending a channel into itself, or closing a cycle of queues that retain one
another, would otherwise create an uncollectable native reference cycle. The
check also reserves edges during a pending send, so concurrent opposite sends
cannot bypass it; a competing dequeue may make a rejected send safe to retry.
Ordinary serialized table cycles are supported.

Durable `fvm_value_serialize`, file channels and network streams cannot persist
live process handles. Sending such a capability through a file returns
`FVM_UNSUPPORTED` before writing a record. Remote mutex proxies, Internet-address
origins, distributed ownership and cross-process capability restoration remain
unimplemented. No native addresses are written into a file.

## Debugger inspection

`flashvm/debug.h` supplies C89 debugger APIs. Mutex snapshots are protected by a
process-wide registry lock and copy numeric state and fixed-size location strings
(no borrowed VM pointers). A row describes a mutex and a waiter, or has waiter=0
when no interpreter is waiting. Source/function strings are truncated to 255 bytes.

- `fvm_debug_mutex_snapshot`: ID, origin, owner, state, waiter, owner acquisition
  location, waiter location, and `deadlocked`. The flag is true for a waiter in
  an actual mutex wait-for cycle. An interpreter merely waiting behind that
  cycle is not itself marked as a cycle member.
- `fvm_debug_channel_wait_snapshot`: active blocking ActionScript send/receive
  waits, channel ID, interpreter ID, operation, and script location. It reports
  waits, not proven channel or mixed channel/mutex deadlocks: channel producer
  and consumer obligations are not known to the runtime.
- `fvm_debug_channel_state`: coherent ID, queue length, capacity, closed and frozen
  flags for an in-process channel.
- `fvm_debug_channel_freeze`: stop dequeue, including primitive C receives;
  producers may still enqueue. Resume explicitly with zero. A closed frozen
  queue remains frozen until resumed. It does not suspend the whole interpreter.
- `fvm_debug_channel_message`: inspect any currently queued message without
  dequeueing. Ordinary payloads are immutable serialized snapshots, decoded into
  the debugger's context. Capability handles remain live; separately capture
  their state with the mutex/channel snapshot APIs. Inspection does not grant
  an immutable copy of an OS synchronization primitive.

The head packet/string is retained/copied under the channel lock and decoded
outside that lock. Concurrent resume, dequeue, or channel destruction after a
properly retained operation cannot invalidate its inspection reference. Use a
separate debugger context to avoid accessing a worker's live VM concurrently.
Snapshot APIs with caller-provided arrays return `FVM_NOT_FOUND` and the required
count if capacity is insufficient; no partial snapshot is returned.

AVM1 records action-block offsets and function names. An embedding compiler or
SWF tooling supplies source maps through `fvm_debug_map_action(context,
function, offset, source, line)`; the nearest preceding entry in that function
is used. Empty function names describe top-level action blocks. Script workers
receive a copy of these mappings when launched. `fvm_debug_set_location` supplies
locations for native callbacks. No source compiler, automatic decompilation,
interactive debugger UI, or complete call-stack reconstruction is claimed.
Native primitive channel calls have no VM argument and therefore no script wait
location. File-channel freezing/inspection is currently unsupported.

## Evidence

- `mutex_shared.c`: origin/state/read-only accessors/list, nonrecursive and
  owner-only behavior, graph cycles/aliases, shared channel traffic, capability
  cycle rejection, queue snapshot preservation, file rejection, and teardown.
- `mutex_deadlock.c`: two real OS workers acquire opposite locks, debugger
  identifies both wait-for cycle members and their source locations; stop/join
  removes waiters and releases abandoned ownership.
- `mutex_contention.c`: four OS workers perform 8000 protected increments.
- `actionscript_mutex.cpp`: actual AVM1 namespace/method execution, worker input
  and return identity, creator IDs, channel wait locations including source-map
  propagation, frozen packet inspection and resumed delivery.

Build profiles, full-suite counts and sanitizer limits are recorded in
[MUTEX_CHECKPOINT.md](MUTEX_CHECKPOINT.md). Windows/macOS/Android adapters are
provided as source; native execution of those builds was not available here.

`channel_debug_concurrent.c`: concurrent producer/consumer, bounded queue, shared mutex capabilities, debugger freeze/peek/resume; 25 repeated ASan/UBSan runs.
