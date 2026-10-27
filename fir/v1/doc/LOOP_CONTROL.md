# Structured loop control — FIR compiler 0.37

[Русский](LOOP_CONTROL.ru.md) · [Commented tutorial](../examples/tables/loop-control.b) ·
[While/for-in](LOOPS.md) · [Assignment ownership](TABLE_LEASES.md)

The Coco/R Limbo big/table profile now accepts statement `if`, optional `else`,
`else if`, `break`, `continue` and early function `return` inside while/for-in
bodies. CLI and reusable C99 compiler users use the same grammar and lowering.

```limbo
sum := big 0;
for (key, value in entries) {
    if (typeof key == "string") {
        continue;
    } else if (value < 0) {
        break;
    }
    sum += value;
    if (sum == 42) {
        return sum;
    }
}
return sum;
```

## Execution and joins

Conditions retain typed comparison/membership and short-circuit semantics. Each
arm has its own lexical scope. Initialized big locals visible before an if are
promoted to dominating frame cells; whichever arm runs updates those cells, and
reads after the join observe current values. Body-local declarations in one arm
do not escape it. Uninitialized locals or reference/int/bool rebindings across
a shared join remain rejected until their typed-cell/definite-initialization
contracts are implemented. Table-element mutation uses the existing assignment
leases and is supported in either arm.

Break exits the innermost loop. Continue restarts the innermost while test, or
passes through the innermost for-in cursor step before testing the next key.
Nested loops keep independent target/cursor contexts. Return validates a big
result and exits the function, without cursor advancement. Each source return
has its own trace/executed capture in the current executable tutorial harness.

A transfer terminates its CFG path. A conditional whose arms both terminate
emits no unreachable join; one open arm alone determines continuation. For-in
omits an unused cursor block when every body path exits without continue.
Unreachable statements after unconditional transfers, including after an if
whose arms both transfer, are explicit source errors. Normal source writes
finish their complete lock/RHS/store/unlock sequence before a later transfer.

Control nesting shares the parser's 64-level scope/expression budget; conditions
are parsed before consuming the body's control level. This prevents mixed
while/for/if nesting from overflowing lexical debug arrays or recursive descent.
Debug JSON events for `break`, `continue`, `loop-if` and `loop-return` preserve
the authored keyword occurrence. Full source-to-Dis SBL generation is separate.

## Compiler environment correction

Terminal function decisions already saved/restored SSA bindings between sibling
arms. They now also save/restore promoted big-cell identities. Without that,
a while in the first arm could make the other arm use a cell defined only on
the first path, producing a non-dominating FIR operand. The parent regression
was reproduced; both arm selections now verify and run. This correction belongs
to FlashVM's source extension and is not copied into Inferno.

## Build and run

From the repository root:

```sh
export TMPDIR="$PWD/build/tmp"
mkdir -p "$TMPDIR"
build/fir/fir1_source limbo --input fir/v1/examples/tables/loop-control.b control.fir -g control.json
build/fir/fir1 opt control.fir control.opt.fir
build/fir/fir1_table_run control.opt.fir
build/fir/fir1 dis control.opt.fir control.dis
build/runtime/disvm --jit --quantum 1 --stats --inspect-words 0 4 control.dis
build/runtime/disvm --no-jit --quantum 2048 --stats --inspect-words 0 4 control.dis
```

The reference prints 42. The tutorial captures `42, 0, 1, 0`, with zero remaining
table/key owners. Tests cover raw/optimized output, both VM modes and quanta,
all-closed branches, cursor advancement, nearest-loop targeting, early returns,
scopes, keyword positions, bounded parsing and preservation of existing outputs
on source errors. Installed SDK acceptance recompiles the same example through
a newly linked C99 client. No new FIR type, Dis opcode or container tag is needed.
Outside loops the existing terminal function-decision grammar remains in effect;
general nonterminal function if and labeled transfers remain separate work.
[Compiler 0.38](COUNTED_FOR.md) adds the classic three-clause for.
