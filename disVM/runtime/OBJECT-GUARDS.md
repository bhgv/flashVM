# Native object guard and context fences (runtime 0.61.0)

Status: an independent, tested native object-access primitive. It is NOT embedded
in Heap, does not register GC roots and does not enable parallel managed mutation.
The current managed NativeWorld remains exclusive. The public C89/C++98 API is
[object_guard.h](../include/disvm/object_guard.h); implementation is
[object_guard.c](../host/object_guard.c). See the complete
[mutable-object example](../examples/native-object-guard/README.md).

## Ownership and waiting

A guard retains its native domain from init until fini. Domain destroy refuses
while initialized guards remain. Object storage must be live/address-stable through
all users; copying or moving initialized metadata is invalid. Caller strong refs
protect waiters; the API cannot acquire lifetime through a borrowed raw pointer.

A bound context acquires with at most 32 pointer CAS attempts. BUSY rolls back
its temporary TLS accounting. Recursive acquisition refuses. AVAILABLE from wait
never reserves ownership; reenter the appropriate VM state and retry. Foreign
or duplicate release refuses. Acquire/release owner-word publication protects
plain native payload reads/writes. The fallback uses the port registry mutex;
it must not be called recursively under that same mutex. Release/wait use stripe
mutexes, so the entire API is not a lock-free algorithm.

The 64 parking stripes are shared native resources, not a thread/object cap.
Waiter registration, predicate recheck, condition wait and unlock/wakeup share
one stripe mutex. Native wait never acquires a guard on return. Caller lifetime
must also exclude object retirement; fini refuses held/waited guards but cannot
make an illegally dangling caller pointer safe.

## Context protocol

NativeContext records current-owner held guards. Count mutation is owner-only;
foreign queries never read foreign mutable metadata. Binding/storage still uses
the existing native mutex. The opaque context implementation grows; the extended
up/REG storage placement and Heap/Type/Jit/wire layouts are unchanged.

Held guards refuse poll, turn acquisition/release, poll-hook changes, TLS unbind,
world join/enter/leave/detach/seal and collector stop/resume. HeapOwner leave/native
suspend therefore refuse before changing their phase/counters. Try-enter admits
bare native contexts or acknowledged ACTIVE HeapOwners. Idle/native/unacknowledged
reentry and collector/custom-hook contexts refuse before guard metadata access.
Wait admits bare native contexts or published inactive VM owners, never ACTIVE
owners or a caller holding any guard. It does not publish roots itself.

Accounting is an affinity/safepoint fence, not an automatic unwind/owner list.
Every C critical section must release on errors BEFORE calling VM error transfer,
callbacks, GC safepoints, heap transitions or native I/O. Unconverted C/JIT code
cannot start using guarded Heap access simply because this primitive exists.
Future integration needs pinned roots/stable address, actual object/header ABI,
allocator/RC/marking conversion, lock ordering and checked unwind/cancellation.

## Evidence scope

The fixture uses real OS contexts and native mutable objects, not a managed-heap
simulator. It observes a registered waiter before waking it; no elapsed sleep
alone establishes a park. Different objects can have different simultaneous
owners. Eight workers perform 16000 protected paired-field updates across two
objects. Real NativeWorld/HeapOwner transitions test refusal without partial
state changes, including an empty-root collector context. No managed collection
or payload scan is claimed for that role test.

Nine isolated controls/backend require semantic status 10: recursive ownership,
wait role, foreign release, poll, unbind, turn release, world leave/enter and
accounting exhaustion. Compiler/signal/sanitizer/timeout is not control detection.
Native-selected and forced-lock implementations run. Existing runtime/TLS/REG/
rendezvous suites separately verify zero-guard compatibility. All new hosted
execution evidence is native Linux x64, not ARM/RV managed-heap certification.
