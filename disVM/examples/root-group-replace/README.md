# Replace a preallocated root group atomically

The fully commented native C example owns three ordinary identity tokens and one
root-metadata reservation. It publishes duplicate occurrences, replaces the whole
vector, removes one occurrence using the new indices, replaces with an empty
active group, then clears and destroys its reservation. Root registration does
not retain or release the tokens. Their lifetime is supplied by this caller.

This executable is a separate native-only process with no VM collector. Its
integer addresses are opaque metadata keys, NEVER managed Heap headers. A hosted
DisVM module must use real roots from that VM and obey ACTIVE/stopped-world,
reference-lifetime, epoch and lock-order contracts; do not copy this native-key
setup into a running collector. Actual managed evidence is provided by the
separate `test_managed_root_replace.py` diagnostic.

Build from the repository root:

```sh
cc -std=c99 -pthread -I disVM/runtime -I flashVM/ports disVM/examples/root-group-replace/main.c disVM/runtime/root_batch.c disVM/runtime/root_registry.c flashVM/ports/linux/thread.c flashVM/ports/linux/channel_sync.c -o build/root-group-replace
./build/root-group-replace
```

Installed runtime/editor SDK root:

```sh
cc -std=c99 -pthread -I source/threading -I source/ports examples/root-group-replace/main.c source/threading/root_batch.c source/threading/root_registry.c source/ports/linux/thread.c source/ports/linux/channel_sync.c -o root-group-replace
./root-group-replace
```

Expected:`PASS root-group replace: duplicates, new indices, active empty, cleanup`.
The source also shows explicit C linkage for C++ clients of these C headers.
It does not instantiate another registry for use with an existing hosted VM.

One complete registry-mutex interval gives visitors either the old or new
multiset, including sparse groups, duplicates and zero occurrences. Invalid
vectors refuse unchanged before locking. The batch itself requires exclusive
caller ownership; different batches may be replaced concurrently. No allocation,
free, destructor, managed owner or VM callback occurs inside replacement.

This is a visibility primitive, not a collector snapshot lease or permission to
free a root immediately after a visitor copied its address. A concurrent graph
collector still needs its root/graph epoch protocol. Referent lifetime and strong
ownership stay separate. See [ROOT-REPLACEMENT.md](../../runtime/ROOT-REPLACEMENT.md),
installed at SDK root; existing native-module registration is documented in
[the native-module tutorial](../native-module/README.md).
