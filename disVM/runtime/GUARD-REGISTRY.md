Runtime0.88 update: [MOVP now uses separate allocation-free pointer-cell range guards](SHARED-POINTER.md).
Per-referent sidecars and cell stripes have distinct ownership/exclusion purposes.

# Canonical object sidecars — runtime0.87

A shared payload guard must belong to the referent, not to each independently
created ticket. Otherwise two captures of one object could obtain different
locks and both claim exclusive access. The native guard registry now supplies
one canonical sidecar for all simultaneously leased instances of one identity.
Hosted managed tickets acquire a lease keyed by their actual referent.

## Registry and lifetime

`guard_registry.c` is a separate native algorithm with an opaque C89-compatible
interface in `disvm/guard_registry.h`. A registry owns one metadata mutex and one
shared64-stripe parking domain. Hash buckets contain dynamically allocated nodes;
64 is not an object/thread limit. Equal keys share one node; hash collisions do
not conflate different keys. A miss allocates its candidate outside the registry
mutex, then rechecks before publication. A losing candidate finalizes its unused
guard and releases its domain reference before freeing native storage.

Every successful acquire grants one native lease. Lease references are protected
by the registry mutex, independently of managed reference counts. Last release
finalizes and unlinks the node under that mutex, then frees after unlocking.
Held/waited guards refuse final retirement. Lease acquire/release refuses while
the current context holds any object guard. Registry destruction requires empty
storage AND externally excluded current/future users; emptiness alone is not a
lifecycle reservation. Object-guard ownership, affinity and parking rules remain
unchanged. The registry never holds its metadata mutex during payload work.

Identity is hashed/compared only, never dereferenced or retained by this native
API. The caller independently owns the key/object through every lease operation
and guard borrow. Native leases do not manufacture managed roots or provenance.
This external lifetime rule prevents address reuse while an old guard remains
legitimately borrowed. A guard pointer becomes invalid after the final lease;
it is not a persistent object ID. No native pointer is serialized or cross-VM.

## Actual managed integration

The hosted owner service owns one registry per runtime session. Capture acquires
one canonical lease before managed count/root acquisition, under existing ACTIVE
serialized admission and independent source ownership. Allocation, capacity,
retain or root-registration refusal releases the new lease as well as the new
count/ticket metadata. The existing source owner remains independent.

A ticket holds one managed strong owner and external root. Native aliases retain
the ticket; they do not each create a new managed root or a new sidecar lease.
Another independent capture of the same referent creates another ticket/lease
but shares its canonical guard. Last native ticket drop still queues Fast; it
does not touch managed payload or drop the sidecar inline. Actual stopped-world
Fast releases that ticket's lease BEFORE ptrdel and destroy. Another live ticket
keeps the same node/guard alive. Joined service finish requires empty registry,
then destroys its shared domain; the service's old process-lifetime mutex remains.

`dvm_managed_owner_guard(entry)` borrows the canonical guard only from a current
linked rooted ticket under matching acknowledged ACTIVE world admission. Obtain
all borrowed pointers before acquiring guards and keep the corresponding entries
alive through leave. Guard ownership does not authorize calling the VM, changing
scope ownership, leaving admission or using an arbitrary managed payload from
bare native code. The legacy `value` accessor still refuses guarded callers.
The metadata entry-count query pins no node and grants no subsequent access.

**NativeWorld still serializes general managed execution.** Other VM payload and
source-cell accesses are not automatically guarded by this change. Compiler
locality/escape decisions, checked shared operations, graph/root publication,
GC epochs, channels/scheduler transitions and JIT code lifetime remain separate
unfinished protocols. This registry establishes canonical lock identity and its
real rooted lifecycle; it does not activate a parallel managed heap. Heap/Type
headers, vendor snapshots, Dis64 IDs/layout and file-call admission are unchanged.

## Verification

The native test uses actual object guards, TLS, worlds and ports. Two native
objects deliberately collide in one hash bucket. Eight joined OS writers overlap
first captures, prove canonical same-key and distinct-key guards, perform16000
paired guarded writes and retire all nodes/domain references. Native-selected
and forced-lock guard/refcount backends both pass. Each backend sweeps all67
constructor allocation refusal points plus new-node refusal with a private native
allocation ledger; partial failure and final success leave zero tracked storage.

Five actual-source controls per backend damage canonical lookup, distinct-key
comparison, lease retain, guarded-release refusal or constructor cleanup. They
must fail explicit semantic10 before invalid aliases are used. The allocator
control fails its ledger before native worker launch. No timeout, compiler
failure, sanitizer report or signal is counted as successful detection.

The actual runtime probe creates two real Strings and three independent tickets.
It checks canonical/different guards, simultaneous distinct guard ownership,
guarded operation refusal, stable identity after partial Fast, survival across32
rooted Cycles and exact sidecar/Type baselines after final Fast. Initial Fast
drains prior unrelated retirement before measuring Type baselines. Existing
managed owner diagnostics now account for five actual captures/releases rather
than two; root/error/collector semantics stay explicit.

A private matching-runtime relink forces an actual root-registration VM error
and verifies source count, empty destination and sidecar rollback. Three controls
use per-ticket keys, omit retirement or omit root-error lease rollback. C/JIT and
both poll modes require exact semantic failures at safe boundaries. Existing
direct-capture missing-link/late-accept/missing-cancel controls remain intact and
are now registered in CTest. The role-only fixture links the actual new native
source closure, retaining its refusal sentinels rather than emulating a heap.

47 selected console/editor CTests pass. Both native backends/constructor ledgers
pass TSan and nonrecovering UBSan, including all source controls under UBSan.
Actual managed positive diagnostics pass TSan; generated JIT bytes remain
uninstrumented and managed admission is still serial. These are native sidecar
and actual ownership-lifecycle checks, not concurrent managed payload acceptance
or a new physical ARM64/RV64 execution claim.

## Reproduction and example

Run `python3 disVM/tests/guard_registry.py` from the checkout. The installed SDK
contains the complete native sources, headers, ports, tests and allocation ledger;
run `python3 source/tests/guard_registry.py` from its package root. Instrumented
positives accept `--positive-only`. Set TMPDIR if /tmp is unavailable.

Actual source-relink check:

```sh
python3 disVM/runtime/test_managed_guards.py   build/workspace/disvm-editor/disvm   build/workspace/disvm-editor/bootstrap/limbo-bootstrap disVM/upstream/module
```

Use the matching paths for your GNU Makefiles build. No production objects are
modified. The [native example](../examples/native-guard-registry/README.md) has
fully commented source, exact build inputs and lifetime/parking restrictions.
See [owner slots](OWNER-SLOTS.md), [managed owners](MANAGED-OWNERS.md),
[GC pending](GC-PENDING.md) and [threading](THREADING.md).
