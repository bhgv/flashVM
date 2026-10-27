# First initialization inside loops — compiler 0.43

[Русский](LOOP_INITIALIZATION.ru.md) · [Commented tutorial](../examples/tables/loop-initialization.b) ·
[Initialization joins](JOIN_INITIALIZATION.md) · [Retained cells](RETAINED_LOOPS.md)

Previously declared `big`, `int`, `string` and `table of big` locals may now
receive their first assignment inside a loop body. A later read in that same
continuing body path is valid. This applies to `while`, counted `for` and
snapshot `for in`; exact assignment types and lexical shadowing still apply.

```limbo
text: string;
part: table of big;
total := big 0;
for (index := int 0; index < int 3; index++) {
    text = "14";
    part = table of big {value=big text};
    total += part.value;
}
# text and part are not definitely initialized at this continuation.
text = string total;
return big text;
```

The tutorial returns 42 without a placeholder string or table allocation before
the loop. It is compiled by the same Coco/R grammar and serial C99 service as
the existing profile. No hand-written parser or new declaration syntax is added.

## Storage and source facts

Before emitting a loop's test, lowering reserves a dominating typed cell for
every active supported declaration. Initialized values are stored as before.
Unreadable references use `ref.nil`; scalars use existing zeroed private cells.
Neither nil nor zero grants a source initialization fact. All body writes use
these dominating cells, including after an inner branch or nested loop.

A first assignment establishes a fact on its current source path. Both open
arms of a branch must establish the fact before a joined read. A returning arm
contributes no continuing path. `continue` and `break` preserve existing control
flow: they do not turn a skipped write into a guarantee. A read in an assignment's
own right-hand side, a compound update, or a later iteration before its first
proved write remains invalid.

Loop exit restores initialization facts for declarations that were uninitialized
at entry. Even a visibly nonempty loop or an unconditional loop with a break
does not prove initialization at its continuation. Nested loops obey the same
rule: assigning in an inner body does not establish a fact after the inner loop.
An assignment before that inner loop does remain available afterward. This is a
conservative static rule, independent of the runtime trip count.

Body-local cells still initialize on each body entry. Outer cells are not cleared
on each backedge: a previous payload can remain physically rooted, but source
code cannot read it without the current path's proof. No cross-iteration
initialization inference is attempted. A later ordinary assignment outside the
loop safely reuses the dominating cell.

## Counted-for header order

The parser encounters the header step before the body, but the generated step
executes after the body or a `continue`. A first assignment to an uninitialized
outer declaration in that step is therefore rejected with `for step assignment
requires an initialized local at loop entry`. It must not create a false fact
while parsing the body.

Initialize the variable before the loop or move the first write into the body.
This release does not infer that all paths reaching a deferred step initialize
its operands. Reads in a step likewise require facts available when its header
is checked. Existing initialized loop counters and reference rebindings in a
step retain their previous behavior.

## FIR, native execution and ownership

No new FIR operation, Dis opcode, frame ABI or public API is required. The
existing independent FIR reference-cell must-analysis checks every `ref.load`
after lowering and after optimization. A possibly zero-iteration body store
cannot authorize a load on the exit edge. A raw FIR module that attempts this
is rejected independently of source facts.

The NC64 backend uses the existing mapped canonical-nil root and retaining MOVP
for reference cells. Repeated writes retain the new payload and release the old
cell owner; SSA roots keep their existing frame lifetime. The reference runner
uses its existing invocation-local owner registry. This change creates no
network capability, exported frame address or new lock operation. External
channel projections remain a separate interface.

This is a FlashVM compiler extension. It is not copied into Inferno's Limbo
compiler. The selected Inferno interpreter/JIT snapshot is unchanged.

## Run and verification

From the repository root after building:

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/tables/loop-initialization.b loop.fir -g loop.json
build/fir/fir1 opt loop.fir loop.opt.fir
build/fir/fir1_table_run loop.opt.fir
build/fir/fir1 dis loop.opt.fir loop.dis
build/runtime/disvm --jit --quantum 1 --stats --inspect-words 0 2 loop.dis
build/runtime/disvm --no-jit --quantum 2048 --stats --inspect-words 0 2 loop.dis
```

The reference prints `42`; native captures are `DATA 0 42` and `DATA 8 1`.
`fir1_join_initialization` and its two runtime variants cover all four declaration
types, both quanta, original and optimized FIR, nested loops, skipped iterations,
joined writes, continue/break, post-loop reinitialization and forbidden reads.
Negative cases preserve the caller's old FIR/debug outputs. The reusable C99
service alternates successful body initialization with a rejected deferred first
write and syntax/join failures. Both installed SDK paths compile the tutorial
through newly linked clients and execute the emitted module in both modes.
