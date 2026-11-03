# Native reference counts: conversion boundary at runtime0.60.0

Status: typed atomic primitives and selected host sites are converted. General
parallel Heap/Type access is NOT enabled. NativeWorld still grants exclusive
managed heap ownership. Mixing atomic and plain concurrent accesses to the same
word is forbidden; existing unconverted paths stay safe only under that exclusion.

## Primitive contract

`disvm/refcount.h` operates on native unsigned-long Heap-shaped counts and signed
int Type-shaped counts. It does not inspect payload, infer a header, register a
GC root, mark an object, run a destructor or synchronize mutable fields. Counter
storage must already be live/aligned, held through an owned strong reference or
a source-slot protocol. Initial plain assignment is allowed only before publication
or after all access from the previous lifetime has ended.

Retain refuses zero, signed negatives and maximum overflow without mutation.
Release refuses zero/negative and distinguishes LIVE from LAST. LAST transfers
retirement responsibility exactly once; it does not free memory automatically.
A successful drop never accesses the counter after its RMW. Another owner can
therefore finish LAST and free the object before a nonfinal drop returns.
The implementation uses acquire loads and acquire/release CAS RMWs, including
nonfinal releases, so the final owner observes prior released writes. It is
stronger than the minimal ordering needed for a retain; weakening it is separate
work requiring its own ordering review and target tests.

Always-lock-free native types use compiler atomics. Other types use the port
registry mutex, with no allocation or silent unsynchronized fallback. Forced
DVM_REFCOUNT_FORCE_LOCKED exercises that path. Do not call the fallback while
already holding the registry mutex. It is a compatibility implementation, not
an assertion of lock-free progress on every compiler/platform.

## Converted hosted sites

| Site | Conversion | Remaining exclusion |
| --- | --- | --- |
| runtime/main.c inspect_retain | Atomic load/checked retain; failed retain rolls back ptradd | Payload/type-map validation and root table |
| runtime/main.c newprog | Checked module retain before global process publication | Process list, scheduler and environment |
| runtime/main.c start_module | Checked nonfinal drop of initial local module owner | Stack/module construction and cleanup |
| runtime/bigtable.c Bigtable_acquire | Atomic checked retain with root rollback | Lease bookkeeping, payload and marking |
| runtime/runtime_rc.c last-owner eligibility | Atomic count observation | Queue transfer, root table, colors and collector protocol |

`ptradd/ptrdel` register/remove external GC roots. They do NOT increment/decrement
Heap.ref. Root registration, a strong RC retain and Setmark are distinct operations.
The deferred RC queue consumes the caller's existing last owner and registers a
root; it must not acquire an extra count accidentally. This conversion preserves
that behavior.

## Unconverted production paths

- Retained libinterp/heap.c, xec.c, link/load/stack/string and other bodies still
  contain direct Heap.ref/Type.ref operations. Exclusive mode remains mandatory.
- ARM64 and other native/JIT retain/mark sequences require their own atomic/barrier
  conversion. Merely changing native field offsets or C helper bodies is insufficient.
- Setmark/color/nprop, allocator lists, Type lifetime, root tables and channel/alt
  publication need a shared protocol. Object locking has not been implemented.
- Deferred RC queue operations still use world exclusion. An atomic count load
  does not turn queue state/ownership transfer into a parallel data structure.
- Debugger/native/complete/AOT entries, errors and context publication must obey
  the same mode. Existing hosted TLS REG does not by itself convert upstream
  Inferno's global REG or certify arbitrary cross-context entry.

No Heap/Type layout, file profile/section/opcode ID or Dis64 wire semantics changes.
Selected upstream snapshots remain byte-identical. Owning-upstream conversion and
JIT/context ABI audits must precede importing later stages. See
[object access proposal](SHARED-HEAP-CONCURRENCY.ru.md) and
[native message example](../examples/native-refcount/README.md).

## Validation scope

The primitive fixture uses8 real OS threads and160000 balanced pairs per type,
plus simultaneous final drops and plain payload reads by the unique final owner.
Each backend also retires200 actual native messages in sender/worker handoffs,
so lifetime testing is not limited to permanent count storage.
Both native-selected and forced-lock backends run. Five boundary controls/backend
must reach semantic status10; signals/sanitizer/compiler failures do not count.
This is actual counter/order evidence, not a concurrent managed allocator/GC test.
Whole runtime tests separately exercise converted host sites under exclusive mode.


## Runtime0.66: retained heap Type sites

[Exact adaptation and limits](TYPE-COUNTS.md): hosted heap.c now converts all
Type retain/release/preflight sites through checked native primitives. Pinned
source stays unchanged. Other C/JIT Type sites and every remaining Heap counter
still require serial admission; this is not a global atomic Type conversion.


## Runtime0.67: retained heap Heap sites

[Heap-count adaptation](HEAP-COUNTS.md) converts heap.c's3 retains,2 drops and
5 observations, preserving the RC-defer hook and legacy gchalt protection.
Together with Type conversion this covers count RMW/observations in hosted
heap.c only. Other C/JIT/GC sites remain unconverted under serial admission.


## Runtime0.68: explicit opcode retains

[Opcode adaptation](OPCODE-COUNTS.md) converts11 Heap and5 Type increments in
hosted xec.c. It preserves guards/captures/Setmark/publication order. Module.ref,
preflight observations and whole-header snapshots are separate unconverted
accesses under serialized admission; JIT emitted retains are not changed.
