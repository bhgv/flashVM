# Executable while and table for-in — FIR compiler 0.36

[Русский](LOOPS.ru.md) · [Commented tutorial](../examples/tables/loops.b) ·
[Table ownership](TABLE_LEASES.md) · [CFG ownership checks](OWNERSHIP_FLOW.md)

Our Coco/R Limbo big/table profile now accepts statement loops before its terminal
return/decision. The same source grammar and lowering serve CLI and C99/IDE users.

```limbo
i := big 0;
sum := big 0;
while (i < 7) {
    sum += i;
    i += 1;
}

for (key, value in entries) {
    sum += value;
    entries[key] = value * 2;
}
```

## While and local state

A while condition is evaluated before every iteration, including the first.
Zero iterations leave outer state unchanged. Initialized outer `big` locals
are promoted to private function-frame cells before the loop test. Reads and
assignments then load/store the current cell, so counters and accumulators keep
their runtime values across backedges and subsequent loops. Nested loops work;
body-local declarations are lexical and cannot escape the body. Normal straight
line source before the first loop keeps its existing SSA lowering.

Conditions use the existing typed comparison/membership and lazy boolean rules.
Bodies contain assignments, declarations, deletion and nested loops. Compiler
0.37 adds [if/else, break/continue and return](LOOP_CONTROL.md) inside them.
Compiler 0.38 adds [classic three-clause for and postfix updates](COUNTED_FOR.md).
Loop-carried int/bool/string/table reference rebindings remain separate work.
An outer uninitialized declaration cannot become definitely initialized merely
because an assignment appeared in a possibly zero-iteration body. Such cases
are rejected rather than silently using a body-only SSA definition after exit.
Mutating table elements is supported; rebinding the outer table handle is not.

## Snapshot table traversal

`for (key, value in table_expression)` evaluates the receiver once and captures
one independent `keysof` snapshot. The first binding is a snapshot-owned key
capability, supporting numeric/text namespaces and existing `typeof`, `big` and
`string` key conversions. The second is a copied `big` payload read from the
live captured table at the start of that iteration. The names must be distinct,
shadow only inside the body and do not escape it. `value` is now admitted as a
local name despite also naming the tutorial entry function.

Every captured key is visited in snapshot order. New keys added by the body are
not visited. Existing values may be updated, and deleting the current key after
its value was read is supported. If a not-yet-read key is removed, its later
payload read raises the ordinary missing-key error. This is a snapshot of keys,
not an atomic snapshot of all values. The traversal does not hold one table
lease for its entire lifetime: each source write retains the existing assignment
scope, leaving scheduling/diagnostics available between operations. The initial
extension handles `table of big`; array/string/tuple traversal is separate work.

## FIR, Dis and bounded reference execution

FIR appends `i64.slot` and `big.local`, `big.load`, `big.store`. A local cell starts
at zero, cannot be copied or escape through a cast, and contains a scalar rather
than a heap/native pointer. Loads read memory; local initialization and stores
write it. Optimizers preserve these effects and do not replace loop reads with
stale constants. The NC64 backend maps them to plain frame cells and existing
MOVL operations. Backedges use existing Dis jumps; no Dis opcode or container
extension is introduced. Old FIR enum values retain their numeric identities.
The explicit `fir_acyclic` API still rejects cycles for callers that require a
DAG; table execution/emission no longer imposes that optional restriction.

The reference runner now executes cyclic CFG with a one-million-instruction
budget. Because it owns allocations until invocation end rather than collecting
them, each table/key/text owner registry is separately bounded by FIR_LIMIT.
Failures free registered owners and discard staged trace output. The native VM
uses its existing scheduler, GC and explicit `--max-instructions` budget.
Interpreter and JIT tests exercise both quantum 1 and 2048, including repeated
native table access and collection across loop backedges.

## Build and run

From the repository root with matching built tools:

```sh
export TMPDIR="$PWD/build/tmp"
mkdir -p "$TMPDIR"
build/fir/fir1_source limbo --input fir/v1/examples/tables/loops.b loops.fir -g loops.json
build/fir/fir1 check loops.fir
build/fir/fir1 opt loops.fir loops.opt.fir
build/fir/fir1_table_run loops.opt.fir
build/fir/fir1 dis loops.opt.fir loops.dis
build/runtime/disvm --jit --quantum 1 --stats --inspect-words 0 4 loops.dis
build/runtime/disvm --no-jit --quantum 2048 --stats --inspect-words 0 4 loops.dis
```

The reference prints 42. Native runs show `DATA 0 42`, `DATA 8 0`, `DATA 16 1`,
`DATA 24 0`: the second terminal arm was not executed. Final table/key counts
are zero. Installed SDK checks rebuild a C99 compiler client and run this same
shipped source. This FlashVM language extension is not copied into Inferno.
