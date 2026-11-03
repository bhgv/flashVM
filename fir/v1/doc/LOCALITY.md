# Point-of-use locality — FIR 0.73

`fir_locality_facts` analyzes an already verified, immutable FIR module into
caller-owned `FirLocalityFacts`. Separate calls can share the module and use
independent output buffers. The analysis neither allocates nor executes guest
code. It does not change the FIR 1.0 wire/text format or managed Heap layout.

A private variable home is different from the object referenced by that home.
Function-local cells remain private homes; this alone says nothing about aliases
of their payloads. Table ownership is tracked by fresh allocation identity inside
one basic block. Copies and captured slots retain their original identities;
reassigning a reference cell changes subsequent loads, not earlier snapshots.
All facts are discarded at block boundaries. Opaque callbacks, unknown writes and
suspension invalidate existing private allocation facts. New allocations after
such a boundary can establish new facts. Refcounts and absence of `spawn` are
not used as locality proofs.

## First consumer: numeric table reads

The optimizer forwards a stored immutable big value only when the read has the
same private table identity, the same known nonnegative absolute numeric key,
and an explicitly retained table lease. Store, delete and splice invalidate the
candidate. Unknown owners invalidate every candidate; another key store replaces
the candidate rather than assuming independent keys. Text and relative keys,
unleased reads and facts crossing control flow retain the original operations.

Allocation, key capture, stores, throwing value producers, `table.lock` and
`table.unlock` remain in the program. The retained numeric lookup being removed
needs no additional token or allocation; an unleased lookup may fail obtaining
a native token and therefore cannot be removed by this proof. Global operation
effects remain conservative. Rewrites are verified and facts are rebuilt on the
next optimizer iteration.

[Commented example](../examples/tables/private-forwarding/README.md).

## Checks and limits

`fir1_locality_api` checks immutable snapshots, mutable reference-cell rebinding,
opaque and CFG boundaries, unchanged input and independent concurrent analyses.
With `FIR_TABLE_CORE_ROOT` configured, `fir1_private_tables` compares optimized
and original programs using the independent table evaluator. Its optional runtime
argument checks actual Dis C/JIT with GC and two scheduling quantums.
`fir1_locality_controls` substitutes actual pass bodies: removing delete
invalidation, weakening key equality or losing cell rebinding must produce a
specific wrong result/error in the independent evaluator. Compilation failures,
timeouts and sanitizer failures do not count as successful controls.

This is the first compiler proof layer for the private/shared operation split.
It does **not** remove explicit locks, allocate miniheaps, publish managed graphs,
change root lifetimes or allow concurrent managed Heap access. Broader escape
analysis, compiler-selected local/shared runtime operations, complete C/JIT
reference/mark conversion, allocator concurrency and concurrent GC remain work.
