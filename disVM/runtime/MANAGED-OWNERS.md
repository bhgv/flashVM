# Rooted managed owners behind native tickets (runtime 0.63.0)

This is the first explicit bridge from native owner scopes to the real Dis root
registry and Fast retirement. It is not a parallel managed heap, a moving-GC pin,
a compiler escape-analysis pass or automatic ownership for every Dis variable.
Managed Heap/Type/Jit and wire layouts remain unchanged. The existing NativeWorld
still serializes all managed mutation and stops it during real collection.

## Three independent lifetimes

Capture requires an already-owned valid managed source under acknowledged ACTIVE
admission of this serialized runtime world. It also requires a real extended-up
dvm_native_process_create context and initialized error environment. IDLE, NATIVE,
pending reentry, foreign world, bare/register-only contexts, held guards and
native cleanup refuse before managed payload access. Inline-only GC mode has no
service and capture refuses; it never silently performs an unsafe fallback.

One capture creates an opaque native ticket, one DISTINCT managed strong
reference and one ptradd external root. Source ownership remains independent.
Ticket aliases use native checked counts and normal owner-scope retain/export/
accept; eight native aliases can therefore share ONE rooted managed owner.
The ticket/root stays live until the final native alias is released. Counts,
GC roots and mutable payload rights remain different obligations.

Borrowing a referent through a linked entry requires matching acknowledged ACTIVE
admission and an initialized hosted environment, outside native cleanup. The
borrow ends before scope drop or world leave. Native workers can keep/forward/
release tickets, but cannot read the managed payload while outside heap admission.
No raw pointer is serialized into a channel message or passed between VM heaps.
Returning a borrowed referent to a Dis result slot requires an independent
managed retain/publication under that ABI. Native ticket alias counts do not
make Heap.ref==1 proof of exclusive payload ownership.

## Capture, error cleanup and Fast

Capture reserves native lifecycle accounting, retains the source and protects
ptradd with the actual hosted kprotect boundary. Root-add/retain failure rolls
back only the new owner and native metadata, leaving the empty envelope unchanged.
The caller's valid source owner guarantees that rollback release is nonfinal.
After successful root publication, Setmark runs under exclusive admission.
This relies on the selected ptradd implementation failing before publication.

Last native ticket drop touches no managed header/root and calls no VM destructor.
It links its existing native allocation into an unbounded-by-policy retirement
queue and wakes Fast. No allocation is needed on that drop path. A producer pin
covers the counter RMW, queue publication and worker wake: another final owner
may already have retired a ticket before a nonfinal callback returns. No ticket
access follows a successful nonfinal drop RMW. Worker lifetime stays pinned until
the callback's wake finishes, independently of ticket storage lifetime.

Only the exact collecting native owner with this world stopped can drain. Fast
removes ptradd, invokes the retained destroy and then retires ticket metadata.
It does not substitute unconditional poolfree for destroy or bypass high-gchalt
C-stack retirement semantics. The root/reference remain held across dequeue.
The queue mutex is released before managed destructor callbacks. Fast shares its
existing top-level work budget with legacy RC work; recursive destruction is
still governed by the retained implementation, not a latency bound.

Native longjmp cleanup can therefore consume ticket owners normally: it only
queues retirement. Capture and raw borrowing refuse DURING cleanup, avoiding
nested VM error frames or managed access from a native cleanup callback.

## Shutdown and failure

Before sealing world registration, the controller checks there are no live
native ticket owners or in-flight producer callbacks. Closing capture admission
refuses unchanged while either exists. Queued rooted tickets are still drained
by Fast. After controller leave, worker idle and producer joins, finish requires
zero live/pending/tickets/producers before clearing borrowed service pointers.
Collectors/world are destroyed afterwards. Outstanding owners must never be
silently unrooted to make shutdown appear successful.

The short native service mutex is process-lifetime and reused across sessions;
it is not an allocation arena or new per-thread limit. All service metadata uses
that mutex, including foreign refusal during shutdown. Do not call managed ticket APIs or callbacks under the port registry mutex:
service synchronization uses it even with atomic reference counts. Queue/worker locks are not
held across one another or managed callbacks. Allocation/count exhaustion refuses
capture; a wake failure is latched while roots remain retained. A fatal protocol/
collector failure is not a supported recoverable runtime restart.

Native owners must explicitly end. Opaque native handles that root managed
objects can create cycles invisible to managed reachability; this API does not
add ephemeron/weak-handle semantics or collect arbitrary native/managed cycles.
Native children must cooperate/join before runtime teardown; forced OS thread
cancellation is still unsupported. Current arenas are address-stable; a moving
collector would need actual root-slot retargeting/pinning, not this claim alone.

## API and evidence

The public C89/C++98 native-extension header is
[managed_reference.h](https://github.com/bhgv/flashVM/blob/master/disVM/include/disvm/managed_reference.h).
The hosted implementation is managed_owner.c; start/drain/shutdown helpers remain
runtime-internal. This is not a standalone allocator library. Native modules must
link against a matching hosted runtime and use its supported native build path.

The optional --managed-owner-probe uses a REAL managed String, actual ptradd,
retained Cycles and actual Fast destroy. Eight native receiver scopes keep the
ticket while 32 retained cycle passes run; ACTIVE reentry borrows intact contents.
After all native drops, Fast removes root/reference. A second real String is owned
only by a native scope that raises through actual kprotect/error/longjmp: cleanup
queues its owner; Fast performs the later managed drop. Diagnostics require exact
captured=2/released=2 and zero live/pending/tickets/producers. A newer cleanup
record verifies capture/borrow refusal while native unwind runs.

The optional Makefiles-product control relinks ONE altered adapter in temporary
storage, without touching source, normal objects, binary or dependency files.
Removing ptradd makes the retained collector reclaim the block; the probe checks
its stable arena header and fails by semantic status10 BEFORE reading reclaimed
String contents. A signal/allocator crash/timeout does not count as detection.
Normal runtime tests are independent of this optional generator-specific driver.

Separate role tests use real TLS/world/collector services and root-operation
sentinels, NOT a managed heap simulator. Three removal controls require semantic
status10. They test admission, not allocation or collection. Full runtime probe
coverage and this refusal-only fixture are reported separately. New execution
is native Linux x64; interpreter TSan does not certify RX JIT, all C modules,
ARM/RV or general parallel managed access.


## Forced producer overlap and sanitizer scope

The optional Makefiles driver also links a TEST-ONLY request wrapper. After a
detached receiver queues its final ticket, the wrapper waits for REAL Fast idle
before returning the producer's wake call. Ticket/root retirement has completed,
but shutdown must still refuse until the producer finishes using worker lifetime.
A second altered adapter removes both producer-pin updates; the same wrapper
must fail semantic status10. The unchanged forced-overlap variant must finish
normally with complete joins and exact zero-pending diagnostics.

The controls can instrument just the replacement adapter/wrapper with
MANAGED_OWNER_CONTROL_CFLAGS='-fsanitize=undefined -fno-sanitize-recover=all'.
That is adapter/control UBSan evidence, not a fully instrumented whole VM.
For an instrumented Debug interpreter product, --positive-only runs the joined
forced overlap under TSan; role driver has the same option. Fatal removal tests
run separately normally/under UBSan. An attempted fatal pin-removal TSan run
reported intentionally unjoined threads and was NOT counted as passing. No race
checks are disabled; sanitizer diagnostics invalidate a claimed successful run.
The full joined root probe and helper-RC workloads also run under interpreter
TSan, without claiming instrumentation of emitted RX JIT code.


MANAGED_OWNER_CONTROL_LOCKED_COUNTS=1 additionally relinks the actual production
refcount.c with its forced registry-mutex backend. This tests real ticket/root/
producer lifetimes with fallback counts, not merely a permanent counter fixture.
It does not convert remaining plain C/JIT Heap accesses: exclusion still applies.
Both selected-native and forced-count variants pass adapter/control UBSan and
the joined interpreter-TSan overlap. This is separate from --gc-locked-poll,
which selects the native-world polling implementation rather than RC backend.


## Runtime 0.64: synchronized publication slots

[owner_slot.h](../include/disvm/owner_slot.h) publishes native ticket owners and
retains directly into receiver-local scopes while the source owner is protected.
The actual probe now uses this path for all eight receivers; removal of the slot
owner precedes the 32 cycle passes. Native ticket/root/Fast lifetime is unchanged.
See [slot lock/lifetime contract](OWNER-SLOTS.md); these are not raw managed slots.


Runtime0.65 uses direct slot put/take for controller publication/unpublication.
The error probe also moves a ticket through an empty slot back into a tracked
scope before throwing; pre-longjmp cleanup retires that owner normally. Ticket
and producer/root counts retain the same exact shutdown predicates.
