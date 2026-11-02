# Native reference-count handoff

This small C example transfers an immutable native message to a real OS thread.
It uses the same typed count primitives as the hosted Dis adapters, without
claiming a parallel managed heap or sending a managed pointer across a VM boundary.
Every function documents which reference it borrows or consumes.

The sender starts with one reference, then retains a DISTINCT reference before
publishing the argument to thread_start. Successful creation transfers that
second reference to the worker. Sender and worker may release in either order;
only the LAST result authorizes freeing storage. After release neither accesses
the message again. Failed creation rolls back the unpublished handoff while the
sender still owns its reference. This avoids acquiring ownership through a raw
pointer that another thread could already have freed.

From repository root on native Linux:

```sh
cc -std=c99 -pthread -IdisVM/include -IflashVM/ports \
    disVM/examples/native-refcount/main.c disVM/host/refcount.c \
    flashVM/ports/linux/thread.c flashVM/ports/linux/channel_sync.c \
    -o native-refcount-example
./native-refcount-example
```

Or build the independent CMake target disvm_refcount_example in the disVM project.
The public header is C89/C++98 compatible; implementation uses compiler atomics
when the native type is always lock-free, otherwise a port registry mutex. Forced
DVM_REFCOUNT_FORCE_LOCKED selects that fallback for tests. Do not call the fallback
while already holding the port registry mutex. No operation allocates memory.

A nonzero counter does not make an arbitrary borrowed pointer safe: the caller
must already own a strong reference, or hold the source container slot lock while
obtaining one. Zero retain and overflow refuse without changing the count. Signed
Type counts also refuse negative values. All concurrent accesses to a particular
count must use this API. Plain initialization is allowed only before publication
or after the previous lifetime is completely over.

The example's payload is immutable. For mutable objects, counts alone do not
synchronize fields, collector colors, container links, Type lifetime or JIT caches.
Those need the separate object-access and GC protocols before multiple managed
mutators can run. Native destructor/GC invocation is intentionally outside the
count primitive; LAST transfers that responsibility to its caller.


From an installed runtime product directory on native Linux:

```sh
cc -std=c99 -pthread -Iinclude -Isource/ports \
    examples/native-refcount/main.c source/host/refcount.c \
    source/ports/linux/thread.c source/ports/linux/channel_sync.c \
    -o native-refcount-example
./native-refcount-example
python3 source/tests/refcount.py
```

The installed driver detects its packaged source/header paths and uses private
temporary build files. It exercises native-selected and forced-lock backends,
including five boundary controls each. These are primitive/thread checks, not
proof of concurrent managed heap mutation.
