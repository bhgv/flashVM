# Exact big comparisons and terminal source decisions — FIR 0.18

[Русский](BIG_DECISIONS.ru.md) · [Fully commented Limbo example](../examples/dis/big-decisions.b) ·
[Scalar CFG foundation](BIG_CFG.md) · [Dis target](DIS_BACKEND.md)

Our Coco/R Limbo big-result profile now accepts a terminal decision tree after
its ordinary declaration/assignment prelude. Each arm is either `return expr;`
or a nested braced `if (left relation right) { result } else { result }`.
All six relations `<`, `==`, `>`, `!=`, `<=`, `>=` require two exact big operands.
There is no conversion through binary64 or narrowing to i32. Conditions remain
internal bool values; this does not add bool declarations, implicit truthiness,
comparison-valued table elements or mixed-type comparisons.

`big.lt.signed` and `big.eq` append two FIR operation IDs, preserving old IDs.
Both consume i64/i64 and produce bool. The other four source relations use
operand reversal and/or branch-edge inversion without evaluating either source
operand twice. Operand expressions evaluate in authored left-to-right order.
The comparison event points at the exact authored operator bytes, including
UTF-8 comments earlier in the source; the branch event points at `if`.

## Direct compilation and optimization

Scalar lowering materializes bool with existing BLTL (130) or BEQL (134), checked
instruction-PC fixups, MOVW and JMP. There are no runtime comparison helpers.
Same-type `big` casts use MOVL in the scalar backend. The optimizer removes those
identity copies, folds exact constant comparisons and simplifies dead branches.
Text/key casts retain their checked conversion effects. Comparisons are total
and pure, so identical comparisons can be reused and unused ones eliminated.
Division/remainder remain potentially throwing even when their result is unused.
An unselected zero-divisor arm does not execute and may disappear after opt.

The hosted acyclic table backend and independent C reference runner also
accept both comparison operations and scalar bool copies/traces. Runtime-loaded
big values compare with all 64 bits intact; bool cells have no GC pointer bit.
The native hosted materialization uses the same existing long branch instructions.
FIR 0.19 now admits these live owners/conversion imports across acyclic CFG.
Cyclic table graphs still fail preflight without writing image bytes.

## Scope and limits

Every arm terminates. Declarations/assignments inside arms, continuation after
the terminal tree, joins/phi, mutable source loops, shifts, transparent any/proxy
operations, general functions and full Dis SBL remain separate work. Existing
limits remain: shared nesting budget 64, 1000 source values, 127 decisions/255
blocks and 64 KiB source. Rejected syntax/types/limits preserve the previous FIR
and debug-sidecar destinations. Source comparators do not mutate remote owners.
The source profile metadata stays `limbo_tables` for compatibility; it does not
mean that every big program requires a table import.

## Run the example

After independently building our FIR compiler and runtime, from the repository root:

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/dis/big-decisions.b build/decision.fir -g build/decision.json
build/fir/fir1 dis build/decision.fir build/decision.dis
build/runtime/disvm --jit --inspect-words 0 6 build/decision.dis
build/runtime/disvm --no-jit --inspect-words 0 6 build/decision.dis
```

Original captures: `MIN, 0, 0, 1, 0, 0`, where MIN is -9223372036854775808.
The last three cells are executed flags. Explicit `fir1 opt INPUT OUTPUT`
removes both constant decisions; recount captures before inspection (one value,
one flag remain). The shipped C99 in-process compiler example accepts the same
source; relocated SDK acceptance checks it in both dispatch modes.

Tests cover all six relations with both outcomes, signed extrema, equality,
values above 2^53, nested decisions, wrong operand kinds, missing arms, forbidden
bindings, depth overflow, copied source spans and original/optimized execution.
Independent Python integer comparisons and the table C reference runner verify
results. Physical ARM64 execution is not established by instruction emulation.

[FIR 0.19: acyclic table CFG / Ациклический CFG таблиц](TABLE_CFG.md) · [Русский](TABLE_CFG.ru.md).
