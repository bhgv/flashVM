# Mutable native object shared by OS contexts

This C example combines checked strong-reference ownership, a native object guard
and real OS threads. Four workers increment one native counter 1000 times each;
main joins them, verifies 4000 and retires object, guard, domain and contexts.
Every function documents what ownership is borrowed or consumed.

It is a native malloc object, NOT a mapped Dis Heap. No managed pointer crosses
a VM boundary and no parallel managed-mutator mode is enabled. The domain is a
shared table of 64 native parking stripes; each object contains inline guard
metadata rather than its own OS mutex/condition. 64 is not an object/thread quota.

The sender retains a DISTINCT strong reference before handing an argument to a
worker. Waiting workers keep those references too, so a blocked guard cannot be
freed under its waiters. Main retains its own root until all joins and result
inspection. Only LAST finalizes the now-quiescent guard and frees object storage.
Failed native thread creation rolls back the unpublished reference, joins prior
workers and cleans the same resources. Worker-context/protocol failure is a fatal
example error, not a demonstration of general asynchronous cancellation cleanup.

## Enter, wait and retry

Try-enter makes at most 32 owner-word CAS attempts. BUSY takes no ownership. The
caller can then wait for AVAILABLE and MUST retry try-enter: availability is a
notification, not a reservation. Another owner can win that retry. Both the
predicate wait and unlock/wakeup use the same stripe mutex, preventing a wake
between registration and sleeping from being lost. Colliding stripes only cause
extra predicate rechecks; they do not merge object ownership.

An exact bound context owns each acquired guard. Recursive acquisition refuses;
foreign/duplicate unlock refuses. Held guards are counted in TLS and block VM
polls, scheduler turn changes, heap leave/native suspension and unbind. Wait
refuses ACTIVE VM owners, collector/custom-hook contexts and any caller already
holding a guard. Inactive VM callers must publish/pin their roots BEFORE releasing
the heap lease. This API does not do that for them. This example uses bare native
contexts and stable native storage, with no registered managed roots.

Guarded work is bounded and nonthrowing. Release before VM callbacks, safepoints,
heap transitions, native I/O or dropping the object owner. The count is a protocol
fence, NOT an automatic exception-unwind ledger. C extensions must explicitly
release on their own error paths; arbitrary OS cancellation is unsupported.

An initialized guard cannot be memcpy-copied or relocated: its address selects
the wait stripe and its domain reference is owned exactly once. Clone payload
into a separately zeroed/freshly initialized guard. A strong reference prevents
freeing but is not automatically a pin against a moving GC; managed integration
would need a stable-address/pinning protocol as well.

## Build from the repository

On native Linux, from repository root:

```sh
cc -std=c99 -pthread -IdisVM/include -IflashVM/ports \
    disVM/examples/native-object-guard/main.c \
    disVM/host/object_guard.c disVM/host/refcount.c \
    disVM/host/native_context.c disVM/host/native_owner.c disVM/host/native_world.c \
    flashVM/ports/linux/thread.c flashVM/ports/linux/channel_sync.c \
    -o native-object-guard-example
./native-object-guard-example
python3 disVM/tests/object_guard.py
```

Or build disvm_object_guard_example in the independent disVM CMake project.
The disvm_object_guards static library links disvm_refcount and needs the selected
OS port implementation. Public headers are C89/C++98 compatible. Implementation
uses native pointer atomics where always lock-free; DVM_OBJECT_FORCE_LOCKED and
DVM_REFCOUNT_FORCE_LOCKED select the tested registry-mutex fallback. Never call
that fallback while already holding the registry mutex. The full guard API is
not lock-free: release/wait/finalization use stripe mutexes.

## Build from an installed runtime product

From the installed product directory:

```sh
cc -std=c99 -pthread -Iinclude -Isource/ports \
    examples/native-object-guard/main.c source/host/object_guard.c \
    source/host/refcount.c source/host/native_context.c \
    source/host/native_owner.c source/host/native_world.c \
    source/ports/linux/thread.c source/ports/linux/channel_sync.c \
    -o native-object-guard-example
./native-object-guard-example
python3 source/tests/object_guard.py
```

The driver uses actual native objects and TLS/world transitions, atomic and
forced-lock backends, eight OS owners, paired mutable-field checks, observed
parking and eighteen semantic removal controls. No compiler/signal/sanitizer or
timeout counts as a positive control. It is not a concurrent managed allocator,
marking/JIT or full debugger proof. Heap/Type/Jit/serialized layouts are unchanged.
