# Transactional dense-prefix replacement

[Русский](SPLICE.ru.md) · [Guarded owner](../owned/README.md)

`fht_splice(destination, first, end, replacement)` replaces a signed range with
all dense values of replacement. Negative bounds follow relative sequence rules:
negative end is inclusive, so `[0:-1]` replaces the entire prefix. Positive end
is exclusive. The API is available to C clients; Limbo/FIR slice assignment is
not yet wired to it. This does not change native Dis array slice-store behavior.

For example, values `[1, 2, 3, 4]` replaced at `[1:3]` by `[9]` become
`[1, 9, 4]`. Inserting that result into itself at `[1:1]` becomes
`[1, 1, 9, 4, 9, 4]`. No write is published until all source reads finish,
so source/destination aliasing is supported. Passing an empty replacement deletes;
an empty destination accepts insertion at `[0:0]` or whole-prefix `[0:-1]`.

Text keys and their order survive. Sparse numeric keys keep their absolute keys.
If a growing prefix would overwrite one of those sparse values, FHT_INVALID is
returned. The destination remains intact on invalid bounds, collisions, overflow
or allocation failure. Allocation failure returns FHT_NOMEM. The replacement's
text/sparse entries do not become part of the inserted sequence.

The implementation builds a separate full table, then swaps compatible owned
storage. It needs O(destination count + replacement prefix length) temporary
storage. This favors rollback and correctness; in-place optimizations are future
work. Existing Fht owner identity stays stable, but no internal pointer may be
retained. Allocator context must remain valid for both old and replacement storage.

Serialize reads as well as writes. The plain Fht API intentionally does not hide
locks or native thread dependencies. Use the guarded owner adapter for host
transactions; for two owners acquire guards in stable order, while a self-splice
needs one guard. Never let a borrowed mutable Fht pointer escape its transaction.
