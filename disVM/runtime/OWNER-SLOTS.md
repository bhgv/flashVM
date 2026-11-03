# Shared strong-reference slots — runtime 0.64

The slot closes a specific ownership gap: reading a shared pointer and only then
retaining it is unsafe if another thread can clear the source in between.
`dvm_owner_slot_copy` retains directly into the receiver's owner-local ledger
while the source mutex still protects its existing owner. There is no temporary
unrooted pointer/envelope between acquisition and scope registration.

`dvm_owner_slot_exchange` moves an exclusively owned incoming envelope into the
slot and moves the previous owner back to that envelope. Empty incoming means
assign nil. No retain, drop or destructor runs during exchange. Accept or cancel
the returned owner after the slot mutex has been released; for a managed ticket,
final native drop queues Fast instead of touching the managed heap.

## Lifetimes and lock order

The opaque slot itself is caller-owned native storage. Publish its address with
real synchronization, retain its lifetime until every caller joins, remove the
last slot owner and only then destroy the empty slot. Destruction is not a
concurrent close operation; the mutex cannot protect its own destruction.
A populated slot refuses destruction, rather than silently losing a root.

Owner scopes and destination entries remain local to one bound context. Nil,
occupied destination, foreign/invalid scope, overflow, malformed envelope,
unbound and held-guard operations refuse without ownership change. Guard and
unbound checks precede mutex acquisition. No slot callback acquires managed
admission, performs a VM call or runs managed destruction.

A retain callback runs under the slot mutex and must return normally using bounded
native work. It cannot reenter this slot, acquire another slot or wait for managed
admission. No automatic recursive-lock recovery or arbitrary callback sandbox is
provided. Native ticket retain uses its checked native count and service metadata,
not managed payload. Do not hold the port registry mutex when calling slot APIs:
forced-lock count backends and managed ticket service can need that registry.
Mutable object data still needs its own synchronization; a retained owner is a
lifetime guarantee, not permission for parallel mutable Heap access.

## Actual managed integration

The optional `--managed-owner-probe --gc-worker` now publishes one rooted ticket
through a slot. Eight bare native receivers obtain distinct owners through the
protected copy operation. The controller unpublishes the source slot, then 32
real cycle passes execute while native owners keep the managed String rooted.
Managed borrowing still requires ACTIVE serialized admission. Complete joins,
Fast retirement, native error cleanup and zero remaining ticket/producer counts
are checked by the existing C/JIT and editor/runtime drivers.

The slot contains a native ticket, never an arbitrary managed pointer. This API
adds no shared Dis64 pointer representation or wire/file activation.

## Independent native lifetime checks

[owner_slot.c](../tests/owner_slot.c) runs eight OS readers, 16000 retained
snapshots, 1000 concurrent replacements plus the initial allocation, real final
free, boundary refusals and complete joins. Atomic-selected and forced-mutex
backends run. Three actual-source mutations per backend must reach an explicit
semantic predicate (publication, move result, distinct retain). Signals, compiler
errors, sanitizer reports and timeouts do not count as detected controls.

```sh
export TMPDIR="${TMPDIR:-$PWD/build/tmp}"
mkdir -p "$TMPDIR"
python3 disVM/tests/owner_slot.py
CFLAGS='-O1 -g -fsanitize=thread -fno-omit-frame-pointer' \
    python3 disVM/tests/owner_slot.py --positive-only
CFLAGS='-O1 -g -fsanitize=undefined -fno-sanitize-recover=all' \
    python3 disVM/tests/owner_slot.py
```

TSan positives join every reader. Negative predicates deliberately terminate
before cleanup and are excluded from that TSan run. These tests compile actual
slot, scope, context, count, guard and Linux port sources together; they do not
establish a parallel managed allocator or complete VM sanitizer certification.

[API](../include/disvm/owner_slot.h),
[commented extension functions](../examples/native-owner-slot/example.c),
[rooted managed protocol](MANAGED-OWNERS.md).
