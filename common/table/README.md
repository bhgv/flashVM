# Hybrid scalar table core

[Transactional splice](SPLICE.md) · [Guarded native owner](../owned/README.md)

[Русский](README.ru.md)

This standalone C99 library owns dynamically growing `big`/int64 values under
nonnegative int64 indices and byte-counted string keys. Numeric `1` and string
`"1"` are different keys. There is no fixed element count. Allocation failure is
reported; this does not promise unlimited memory.

Dense indices use initialized array slots with separate presence bits. Sparse
numeric and string keys use a growing chained hash directory. Dense growth
promotes covered numeric entries without changing their values or pair count.
`fht_length` is the first absent index from zero; it is independent of physical
capacity and total stored pairs. Deletion never shifts keys.

`fht_keys` copies all keys into an immutable independent snapshot: numeric keys
ascending, then string keys in insertion order. Replacing a string value retains
its position; deleting/reinserting appends it. Snapshot string bytes remain valid
after source destruction. `fht_slice` copies a half-open sequence range into a
new zero-based table and omits dictionary/sparse-tail entries.

The allocator is injectable. Failed inserts/snapshots/slices preserve logical
source contents and never publish partial results. The core does not lock or run
application callbacks. The owning VM/process must serialize operations. It owns
only copied scalar values and key bytes: no Dis/FlashVM pointer is stored here.
Reference/object keys, arbitrary values and VM GC tracing are future work.

Build independently:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Tests compare 12,000 numeric mutations against an independent model, check exact
signed big limits, holes, binary/Unicode-compatible keys, detached slices and
snapshots, and inject allocation failures with outstanding-block accounting.
The source uses the repository MIT license. It is not copied from Lua and does
not alter Inferno or its Dis container ABI.

## Exact scalar adapters

`fht_big_parse(text, size, result)` accepts a complete byte-counted signed decimal
integer with optional +/-. Invalid syntax, embedded zero or signed-64 overflow
returns FHT_INVALID without changing result. `fht_big_format(value, text, capacity)`
requires at least 21 bytes, returns the canonical decimal length excluding NUL,
and writes nothing for invalid storage. Both borrow their arguments, allocate
nothing and never use floating point. The independent C tests compare 20,005
formatted values against snprintf and check parsing extrema/failures. See the
[Limbo cast contract](../../fir/v1/doc/TYPE_REFLECTION.md).
