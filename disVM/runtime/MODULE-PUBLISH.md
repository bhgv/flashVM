# dvm_module_publish

[Русский](MODULE-PUBLISH.ru.md) · [Header](vfs/module_adapter.h) · [Example](../examples/remote-scalar/STYX.md)

```c
DVM_Provider *dvm_module_publish(DVM_Provider *original, DVM_Provider *adapter);
```

The native host pairs a captured exact file provider with an adapter from
[dvm_module_adapter](MODULE-ADAPTER.md). The original must support cancellable
path operations and exact-root capture. The constructor may perform metadata
I/O and returns an owned provider, or NULL on refusal, cancellation or allocation
failure. Both arguments are borrowed. The result retains both sources.

Mount the result as an exact file. A `bind` of that file retains the publication.
Stat, ordinary read opens and launch-address discovery use the original. Write opens are refused. A typed
Dis load of the same selected VFS node uses the local adapter and its retained
namespace. The original is never parsed as the adapter. The host associates
the original, adapter and matching Styx route in this publication. A plain
Styx mount does not supply that association automatically.

A publisher may also map the file into a native VFS directory view and mount the
directory. For `load`, each view traversed must allow both read and execute on
that path. The loader follows up to 32 provider selections. Binding a file out
of the directory retains the projection and its policy. A read-only alias
continues to allow ordinary file reads.

For the scalar profile, `dvm_module_publish_profile(original, adapter, profile,
tag)` additionally borrows a fixed-root readable profile provider and a nonzero
32-bit tag. Publication retains the captured profile provider. Ordinary typed
`load` compares the profile record to the instruction's compiled import table
before reading the adapter image. This profile supports exactly one named import.
Unavailable, invalid or mismatched profiles fail the load. The previous
constructor still publishes without a profile.

In the paired DisVM test, the owner exports `.interface` alongside the original
`.dis` on the same Styx session. The profile contains the signature emitted by
the compiler for the sample's `.m`. The caller reads that S-record during `load`. An incompatible `big` import is refused before the call.
The owner and client still use the sample's generated scalar dispatch path.
