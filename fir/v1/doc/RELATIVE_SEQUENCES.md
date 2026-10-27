# Relative indexes and copy slices — compiler 0.32

[Русский](RELATIVE_SEQUENCES.ru.md) · [Commented example](../examples/tables/relative.b) ·
[Native arrays](../../../disVM/examples/relative-sequences/main.b) ·
[Owner-operation contract](../../../disVM/OWNED_OPERATIONS.md)

Negative indexes address from the end: -1 is the last element, -2 the penultimate.
For hybrid tables the length is **only the contiguous numeric prefix starting at
zero**. String keys and sparse numeric entries do not participate. String positions
count Unicode scalars, not bytes, UTF-16 units or graphemes.

| Expression on a sequence of length n | Meaning |
| --- | --- |
| `s[-1]` | element n-1 |
| `s[-n]` | element 0 |
| `s[0:-1]` | full copy |
| `s[-2:-1]` | last two elements |
| `s[0:-2]` | all but the last |
| `s[-1:-1]` | last element as a one-element sequence |

Positive slice ends remain exclusive. **Negative slice ends include their named
element**, specifically so `[0:-1]` is a full copy. Empty `[0:-1]` is valid; indexing
an empty sequence at -1 is not. Bounds outside the sequence and reversed normalized
ranges raise; counts are not clamped. INT64_MIN normalization uses unsigned magnitude,
without overflowing signed negation.

## Tables, FIR and captured writes

Numeric reads, writes, membership and delete support relative positions. A negative
key is resolved once at table.index, before RHS evaluation. Subsequent changes to
the prefix cannot silently redirect an already captured compound assignment:

```limbo
t := table of big {1, 2, 40};
t[-1] += big (delete t[0]); # Target stays absolute index 2, although len becomes 0.
```

Snapshots retain canonical nonnegative numeric keys. The string key `"-1"` remains
an ordinary dictionary key. A table slice copies/reindexes only its numeric prefix;
source aliases stay mutable while a slice is independent. Empty-table assignment
through -1 raises instead of inventing a new last element.

The C99 table core exposes fht_resolve and fht_slice_relative. Legacy unsigned
fht_slice retains its positive half-open API. Existing get/set/delete accept relative
numeric keys. The hosted Bigtable interface appends resolveindex; previous exports
and signatures remain unchanged. Direct FIR table.index has the same eager capture
semantics as source. Relative ranges defer receiver-dependent validation until use.
String bounds in the Coco/R profile still require explicit int casts.

## Dis arrays and strings

Host-prepared C handlers normalize array/string indexes, string character writes,
slice bounds and array slice-assignment offsets. Pinned Inferno sources remain
byte-exact. Guarded native paths decline negative operands to these handlers; the
shared complete JIT executor retains the same fallback. No new Dis opcode is added.

A **Dis array slice with any negative bound owns independent element storage**.
The current implementation clones the source with the existing GC-aware arraycpy,
then narrows that independent owner. Thus partial relative copies can allocate
for the full source. Mapped records retain their fields; referenced ADT objects
retain identity, rather than becoming unrelated objects. Positive-only native
Dis array slices keep their existing shared-view semantics. String value copies
retain the established copy-on-write behavior.

The native example checks byte/int/big/real arrays, mapped records, references,
negative writes, slice assignments, Unicode and empty copies with quantum 1.
Native array fixtures use the retained reference Limbo compiler: the bounded
Coco/R source frontend does not yet implement general array declarations.
Its table/string support is tested through our own source/FIR libraries and SDK.
AS Array.slice/property semantics are not changed by this Dis/Limbo extension.

## Ownership and concurrency

The current Dis host executes a complete C operation on one VM owner thread; it
does not yield to another Dis process inside the copy. The C99 core requires
external serialization. **A universal per-object reentrant write mutex is not
implemented.** Broker metadata locking is separate and does not protect payloads.
See the linked operation-token contract for multi-threaded sharing and long operations.

## Run

```sh
build/fir/fir1_source limbo --input fir/v1/examples/tables/relative.b build/relative.fir
build/fir/fir1 opt build/relative.fir build/relative-opt.fir
build/fir/fir1 dis build/relative-opt.fir build/relative.dis
build/runtime/disvm --jit --quantum 1 --inspect-words 0 4 build/relative.dis
build/runtime/disvm --no-jit --quantum 1 --inspect-words 0 4 build/relative.dis
build/runtime/disvm --jit --quantum 1 build/runtime/relative-test.dis
```

Expected source tutorial cells: 42,0; execution flags: 1,0. The native module prints
PASS after ownership checks. Invalid bounds are tested in separate processes in
both dispatch modes, including a runtime-produced minimum signed WORD.
