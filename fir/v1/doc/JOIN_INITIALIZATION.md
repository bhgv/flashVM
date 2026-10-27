# Definite initialization through joins — compiler 0.42

[Русский](JOIN_INITIALIZATION.ru.md) · [Commented tutorial](../examples/tables/join-initialization.b) ·
[Function statements](FUNCTION_FLOW.md) · [Retained cells](RETAINED_LOOPS.md)

The Coco/R Limbo big/table profile now accepts a previously declared variable
initialized on every continuing arm of a condition. Reads after the join use
its common typed cell. A partial assignment is allowed when the variable is not
read afterward; it does not establish definite initialization.

```limbo
answer: big;
text: string;
selected: table of big;
count: int;
if (enabled) {
    answer = big 40;
    text = "ready";
    selected = table of big {extra=2};
    count = int 1;
} else {
    answer = big 21;
    text = "fallback";
    selected = table of big {extra=21};
    count = int 2;
}
return answer + selected.extra;
```

Here `enabled` is an initialized bool. The four explicit declaration types above
are supported. This does not introduce declaration keywords for inferred bool,
key sequences or snapshot keys; their existing initialized locals remain usable.
Assignments still require exact types. Shadowing a variable in an arm initializes
the shadow only, not its outer declaration.

## Source facts and loop boundaries

Before a structured choice, the compiler reserves typed storage for declared
locals without marking them readable. At an open/open join the initialized fact
is the intersection of both exits. A returning arm contributes no continuing
path; the sole surviving arm keeps its own facts. Nested choices use the same
rules. Source reads continue to reject an uninitialized variable even when its
physical scalar slot contains zero or its reference slot contains nil.

Outer first initialization in a possibly empty loop remains rejected. Body-local
variables may be initialized by joined inner conditions and read later in that
iteration. On loop exit the compiler restores uninitialized entry facts and their
entry cell IDs, so a body-only cell never becomes an outer dominating definition.
This includes while, counted for and snapshot traversal. A later ordinary
initialization outside the loop is valid and does not store into a body-only cell.

```limbo
for (index := int 0; index < int 2; index++) {
    local: string;
    if (index == int 0) {
        local = "first";
    } else {
        local = "second";
    }
    # Both normal paths stored a string before this read.
    length := len local;
}
```

The CLI and reusable serial C99 source API share these facts. Declarations and
assignments preserve the same binding identity and authored debug origins.
Existing total-return, scope and unreachable-statement checks remain in force.

## FIR ref.nil and independent proof

One append-only FIR producer reserves a private reference cell without allocating
an empty table/string as a placeholder:

```fir
%cell: string.cell = ref.nil
%text: string = const.string "42"
ref.store %cell, %text
%value: string = ref.load %cell
```

`ref.nil` accepts `string.cell`, `table.cell`, `keys.cell` or `key.cell`. It clears
an old retained owner on repeated execution; a snapshot-key cell also clears its
scalar position. The cell may remain unused. Every load must have a preceding
store on every normal path. Nil storage is not an initialized source payload.
Scalar big/bool cells retain their existing FIR zero-initialization contract;
source initialization facts are checked independently of those defaults.

The FIR verifier proves reference initialization without trusting source metadata:

1. Identify nil-created SSA cells; private cells cannot be copied or escape.
2. Start function entry with no initialized cells and other block states at top.
3. Intersect predecessor exit facts. `ref.nil` kills a fact; `ref.store` generates it.
4. Iterate to a descending fixed point, then scan each block in instruction order
   and reject a load without its fact.

This handles backedges, returning predecessors, repeated constructors and joined
stores. It does not assume that a constant-looking condition must take one arm
before normal verification/optimization. The bounded workspace is private C
memory, freed on every success/failure path; modules without `ref.nil` skip it.
Type, arity, reachability and dominance verification run before this proof.

A raw FIR load before any store, after a one-arm-only store, or after a possibly
zero-iteration storing loop is rejected with `reference cell read before
initialization`. Optimization cannot publish an invalid module or replace the
caller's old output. Initialized `ref.local` retains its existing constructor
contract and does not need a separate nil-cell fact.

## NC64 ownership and reference execution

The Dis backend reserves one mapped canonical-nil module word only when required.
The loader initializes it according to its pointer map. Existing retaining MOVP
copies it into the mapped frame cell, releasing the prior owner safely. The second
word of a key capability stays scalar and unmapped. No new Dis opcode, native
module import, heap placeholder object or SWF format is introduced. Old programs
without the new constructor keep their prior layout.

The independent reference runner clears its private value record. The same FIR
proof prevents that empty record from being exposed by a load. On valid stores,
its invocation-local table/key/text registry keeps ordinary payload lifetimes.
The native implementation instead uses actual mapped roots and reference counts.
Tests require table/key owner counts to return to zero under both dispatch modes.

The source extension and FIR producer remain FlashVM-only. Private frame cells
continue to be separate from external broker capabilities and do not export frame
addresses, add a network RPC operation or change channel locking contracts.

## Build and run

From the repository root after building:

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/tables/join-initialization.b join.fir -g join.json
build/fir/fir1 opt join.fir join.opt.fir
build/fir/fir1_table_run join.opt.fir
build/fir/fir1 dis join.opt.fir join.dis
build/runtime/disvm --jit --quantum 1 --stats --inspect-words 0 4 join.dis
build/runtime/disvm --no-jit --quantum 2048 --stats --inspect-words 0 4 join.dis
```

The reference prints 42. The tutorial's native captures are `42, 0, 1, 0` (two
return sites and their flags). Both installed SDKs compile it through freshly
linked C99 clients, optimize/reparse FIR and run both VM modes.

`fir1_join_initialization` and `disvm_join_initialization_{jit,interpreter}` cover
both FIR states, both quanta, each explicit declaration type, nested and partial
initialization, shadows, surviving returns, body-local cells and loop-exit facts.
Independent raw fixtures exercise all four reference cell types, repeated nil
constructors, both outcomes, returning predecessors, malformed types and rejected
join/backedge loads. Existing scalar zero-load, retained-cell, terminal-tree and
C99 failed-request recovery regressions continue to run.
