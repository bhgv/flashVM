# Borrow a rooted Heap value during a fallible native callback

`with_reference.c` is a commented teaching helper for a real hosted C builtin.
It accepts a mapped owning slot and a synchronous callback. It captures a strong
owner/root while the source slot is excluded, invokes the callback after full
unlock, then consumes the lease exactly once. The real kprotect result controls
cleanup; a primary body error survives a secondary consuming-retirement error.

This file is not a standalone native executable and registers no new builtin.
Compile/link it into a matching DisVM host as described by the
[native-module example](../native-module/README.md). Include the current runtime
headers and the exact generated/native ABI headers/flags of that host. Never
make up a Heap header/native key, substitute a plain malloc object, or link a
second GC/root registry into an existing VM. The diagnostic `test_heap_hold.py`
executes the protocol using actual String/List/Type/GC objects.

Read the example in order:

1. `HeldTask` owns stable native stack storage through the protected callback.
2. `dvm_runtime_heap_hold` reserves metadata, claims the source cell, retains and
   marks the actual owner, publishes its root, then releases exclusion.
3. `invoke_task` lends the value synchronously. The callback may raise a real VM
   error. It cannot keep the native pointer or implicitly consume its owner.
4. `dvm_runtime_heap_finish` clears lease metadata before real consuming destroy.
   Fast may temporarily register its own retirement root; that is a different
   owner protocol, not a leaked lease. Final drainage is checked after Fast.

`expected=NULL` accepts any genuine managed type. An exact Type adds an identity
filter, not pointer provenance. `allow_nil` permits H with an active-empty group;
C NULL payloads still refuse. An already live hold cannot be overwritten.
The original slot/referent must be independently live until capture finishes.
This API adds neither mapping proof nor arbitrary pointer validation.

The caller still supplies ACTIVE/world, frame/GC/code lifetime and compatible
lock order. NativeWorld remains required; this example does not enable parallel
managed allocation, transfer a graph across VM heaps or provide a collector
snapshot lease. See [HEAP-HOLD.md](../../runtime/HEAP-HOLD.md), installed at SDK root.
