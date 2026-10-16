# Live function interfaces (v0.407)

A function can be transferred as a callable interface to the original function,
including a C callback without a serialization codec. The original closure,
global object, userdata and managed capture stay in their original context.
The broker executes calls on its existing service thread and listening TCP port,
shared with channel/mutex capabilities. Each external call currently opens its own TCP
connection. Requests and replies carry target, source process/context and random
call IDs; replies with mismatched IDs are rejected.

Bind the original context with `fvm_context_set_broker`, then enable
`fvm_context_function_transfer(context, 1)`. Serialize/send functions while the
host owns this context. Afterwards `fvm_context_function_serve(context, 1)` hands
an idle context exclusively to the service thread. The host must not access its
heap while serving. `fvm_context_function_serve(context, 0)` disables new calls
and waits for active invocation, result encoding and exception cleanup before
returning heap ownership. Context destruction also quiesces calls and removes
exported roots before freeing the heap. Native userdata remains host-owned and
must outlive all calls. Managed captures are retained by the exported function.

Receiver and arguments travel in one graph, preserving aliases within a call.
Ordinary mutable objects are copies, not shared heaps. A default receiver uses
the original global object. Results, counted thrown values and diagnostic
metadata return to the interface; original pending exceptions are cleared.
Forwarding an interface preserves its authority URI. Destroying the source
revokes its targets; stopping the broker disconnects clients. Neither action
makes foreign pointers available to clients.

The default graph policy still clones functions for compatibility; opting out
restores that policy for new ordinary functions. Already imported interfaces
continue to transfer as interfaces. This is a callable facade, not a clone of
function properties/prototype or a certified transparent constructor.

Calls execute serially across one broker. Nested function calls within the same authority dispatch directly on its
thread, preserving graph copies and original heap leases. The total nesting
limit is 32 active calls; exceeding it returns `FVM_UNSUPPORTED`. Blocking broker resource operations
from the service callback and cross-authority callback cycles are not supported
as an unrestricted reentrant execution model. Network calls have a two-second socket timeout;
lost replies never trigger automatic retry because side effects may already
have happened. Persistent connection multiplexing, cancellation, authentication,
encryption and unrestricted concurrent use of one VM heap are not provided.
Treat capability URIs as bearer secrets and use a trusted network.

Tests: `function_remote` covers original native/script state, unregistered C
callbacks, GC roots, counted exceptions, errors, diagnostics, forwarding and
revocation. `function_remote_protocol` checks four reply routing identities.
`function_remote_concurrent` runs 100 calls from four independent client heaps
and checks stopped-broker/source cleanup. All nine roadmap stages remain open.

In v0.398, malformed successful/thrown replies without a value graph and
successful replies claiming a pending fault are rejected before decoding.
Protocol coverage includes these cases and an invalid result code.
`function_remote_graph` verifies cyclic arguments, receiver/argument aliases,
copy isolation, a returned C function interface and rejection after heap reclaim.

In v0.399, new-target URI construction precedes publication and pinning. The
export allocation sweep verifies failed C++ allocations leave no target/root;
canonical re-export retains exactly one root. This does not promise rollback of
an entire graph's earlier exports or a public malloc-owned URI copy failure.
The prepared-body GC case executes the original attachment, unbinds it during
the call, collects with the active capture and cyclic arguments live, then
verifies the next call uses original bytecode.

In v0.400, same-authority callbacks can cross serving contexts and return to an
already active original context without opening a connection back to their own
listener. All source contexts still require exclusive serving handoff. Ordinary
script/native call barriers and GC rules remain in force. The nesting bound is
released on success, failure and exception, including argument/return encoding.
Cross-authority callback cycles remain unsupported; resource operations do not
use this function-only direct route. `function_remote_nested` covers independent
original heaps, A→B→A, recursion bounds, counted exceptions, diagnostics, host
reclaim and subsequent recovery.

The v0.401 C89 tutorial is `examples/embedding/function_rpc/main.c`. Run its
executable with `serve`, copy the printed URI, then run another process with
`call URI`. After the client completes, enter a line in the server terminal to
reclaim the original context and shut down. The original process verifies four
attempts in its host-owned userdata; the other process verifies value 42, a
counted NUL exception, unsupported and ordinary error statuses. No callback
codec, serialized native address or shared userdata is involved.
`flashvm_function_independent_processes` automates this exchange with bounded
startup/client/shutdown waits and child cleanup. It is registered only for native
socket-enabled builds with tests, examples and Python available; the example
without arguments displays usage and returns immediately.

[Process, independent VM-thread and LAN commands](FUNCTION_RPC_EXAMPLES.md) — v0.402.

In v0.403, `fvm_context_function_revoke(context, function)` retires one canonical
original target and drops only its broker-owned GC pin. Reclaim the source with
`function_serve(context, 0)` first: a serving or active source returns `FVM_ERROR`
before any object read. No matching export returns `FVM_NOT_FOUND`. Existing
interfaces become stale; re-export creates a fresh URI while other targets and
host pins remain live. Broker target tokens now occupy fixed inline storage,
so revoke, function detachment and entry removal during stop do not allocate
strings while releasing resources/roots. `function_remote_cleanup_allocation`
checks these operations with every C++ allocation denied and mixed channel/
function entries from two original heaps. This is not a promise that arbitrary
user finalizers or full graph rollback require no memory.

In v0.405, stopping an authority after mutex traffic releases persistent peer
contexts without allocating identity strings. Peer keys use fixed nonce/context
storage. A regression holds two remote mutexes and one original function export,
denies all C++ allocations during stop, verifies released locks and retains the
function's authority pin until source detachment.

In v0.406, broker startup contains member-constructor allocation exceptions
inside the C API and preserves explicit failure statuses. A failed valid startup
returns a null handle; allocation failures report `FVM_OUT_OF_MEMORY`. The startup
sweep covers constructor/runtime/string boundaries, occupied listener failure and
successful reuse of the port after stopping the original listener.

In v0.407, incoming mutex ownership identities and function source/call IDs are
validated as counted canonical lowercase hexadecimal bytes, not C-string prefixes.
Function source context zero is rejected. Malformed metadata cannot create peer
heaps or enter original functions; valid messages still recover after rejection.
These identities are routing/correlation metadata, not authentication.
