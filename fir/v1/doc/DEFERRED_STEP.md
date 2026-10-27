# Initialization at deferred for steps — compiler 0.44

[Русский](DEFERRED_STEP.ru.md) · [Commented tutorial](../examples/tables/deferred-step.b) ·
[Loop-body initialization](LOOP_INITIALIZATION.md) · [Joined initialization](JOIN_INITIALIZATION.md)

A counted-for header is parsed before its body, but the step executes after
ordinary body fallthrough or a `continue`. The Coco/R Limbo big/table compiler
now checks initialization in that execution order. A step may read a previously
declared variable first initialized on every body path reaching that step.

```limbo
part: table of big;
text: string;
total := big 0;
for (index := int 0; index < int 3; total += part.value + big text) {
    index++;
    if (index == int 2) {
        part = table of big {value=7};
        text = "7";
        continue;
    }
    part = table of big {value=7};
    text = "7";
}
return total;
```

The example returns 42. The step's first load of each operand is safe because
both the continue edge and ordinary fallthrough wrote it. A `continue` skipping
either write is rejected, even if a later iteration might supply the missing
value. The same rule covers `big`, `int`, `string`, `table of big`, compound
updates and typed receivers used in step expressions. Existing initialized bool,
key and key-sequence locals retain their ordinary behavior; no new explicit
source declaration types are introduced.

## Source proof in execution order

1. Loop entry reserves existing typed cells for the active declarations.
2. Before staging the step, save the incoming source environment. Reading an
   unreadable declared cell records an obligation and its original source mark,
   then emits a typed load into the private staged stream.
3. Restore the saved environment before parsing the body. A staged step write
   therefore cannot initialize the first body iteration or the condition.
4. At each continue targeting this counted loop, intersect its initialization
   facts into the step's incoming facts. A continue of an inner while/for-in or
   another counted loop contributes only to its own target.
5. Add ordinary fallthrough when open. Every deferred read must be initialized
   in the intersection. Break and return paths do not reach the step.
6. Publish the step and deferred debug records only after that proof succeeds.
   If no path reaches it, discard both without requiring initialization.

Each nested counted loop owns its own bounded context. Reusing a released context
clears prior obligations and incoming facts. A failed request releases staged
streams and debug state; later C99 compiler requests start with fresh contexts.
This is compiler bookkeeping, not VM allocation, a lock or a network wait.

A first assignment in the step itself is now allowed when its RHS is valid, but
it establishes no fact in the body or after loop exit. The body must prove its
own reads from its entry facts and preceding statements. Unknown identifiers,
type mismatches and step declarations remain errors, even in an unreachable step.
A read of a declared but unreadable variable in an unreachable step needs no
initialization proof, since neither its load nor debug event is published.

## Rejection and authored diagnostics

This program is invalid:

```limbo
text: string;
for (index := int 0; index < int 2; text = text+"x") {
    index++;
    if (index == int 1) {
        continue; # No value for the first step.
    }
    text = "ready";
}
return big 42;
```

The diagnostic is `for step read requires initialization on every incoming path`.
Its location is the deferred read of `text` in the header, not the body's closing
brace. The serial C99 API returns its original UTF-8 byte offset and token length.
Failure preserves the caller's previous FIR and FIRDBG1 files. With multiple
missing operands the compiler reports one unmet obligation; it does not promise
that binding order equals textual order of all reads.

Loop trip counts are not proved. Even a literal false condition does not remove
a statically reachable step before source checking. Similarly, nested-loop body
writes alone cannot initialize a variable after that inner loop. Loop continuation
facts remain conservative as described in the 0.43 guide.

## FIR and DisVM

Deferred loads name dominating private cells, never a body-local SSA payload.
The existing independent FIR `ref.nil` must-analysis checks reference loads at
the resulting join/backedge before execution and optimization. Scalar cells
retain their existing zero-storage contract; source checks keep zero from being
mistaken for a proved source value. No new FIR operation, Dis opcode, native
module signature or public C99 API is added.

The NC64 backend uses retaining MOVP and mapped frame roots for string/table
cells. The reference runner uses its existing invocation owner registry. Tests
compare original/optimized FIR and require native table/key owners to reach zero
in both JIT and interpreter modes. External channel projections do not expose
these cells. This source extension remains FlashVM-only, separate from imported
Inferno interpreter/JIT fixes.

## Build, run and tests

From the repository root after building:

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/tables/deferred-step.b step.fir -g step.json
build/fir/fir1 opt step.fir step.opt.fir
build/fir/fir1_table_run step.opt.fir
build/fir/fir1 dis step.opt.fir step.dis
build/runtime/disvm --jit --quantum 1 --stats --inspect-words 0 2 step.dis
build/runtime/disvm --no-jit --quantum 2048 --stats --inspect-words 0 2 step.dis
```

Reference output is `42`; native captures are `DATA 0 42`, `DATA 8 1`.
The initialization regression runs four explicit declaration types, branch and
continue intersections, nested contexts, table mutation receivers, unreachable
steps, step-only first writes, and forbidden body/post-loop reads. Every valid
case executes before/after optimization and at quanta 1 and 2048 in both native
modes. The C99 service alternates successful compilation with deferred proof
failure and checks the precise diagnostic token plus unchanged old outputs.
Both installed SDK paths rebuild clients and execute this tutorial independently
of the source checkout.
