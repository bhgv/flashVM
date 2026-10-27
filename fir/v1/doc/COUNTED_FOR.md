# Counted for and postfix updates — FIR compiler 0.38

[Русский](COUNTED_FOR.ru.md) · [Commented tutorial](../examples/tables/counted-for.b) ·
[Loop transfers](LOOP_CONTROL.md) · [Snapshot traversal](LOOPS.md)

The Coco/R Limbo big/table profile now accepts the three-clause form alongside
snapshot for-in. CLI and the in-process C99 source compiler share this lowering.

```limbo
sum := big 0;
for (index := big 0; index < 100; index++) {
    if (index % 2 != 0) { continue; }
    if (index == 14) { break; }
    sum += index;
}
return sum;
```

The initializer runs once. The condition is checked before every body pass. The
step runs after normal body completion and on innermost continue, exactly once;
break and function return bypass it. Any clause may be omitted: an empty middle
clause means a typed constant true. Existing while/for-in and nested transfers
retain their behavior. A header clause contains one assignment/update or delete;
comma expression lists and declarations in the step are not admitted.

## Binding and update rules

The initializer and body share a loop-local lexical scope. An initializer may
shadow an outer name; that inner name disappears after the loop and cannot be
redeclared in its body. An assignment to an already declared outer big variable
continues to update that variable. Body-local declarations may be reassigned
normally; loop-carried initialized big locals keep their existing frame cells.
Reference/int/bool rebindings across loop iterations still need future typed cells.

Statement-only postfix `name++`, `name--`, `table[index]++` and dotted/snapshot-key
forms now work for big payloads. Receiver and key are captured once. Table updates
retain the assignment lease across old-value read, exact modular arithmetic and
store. They do not return the old/new value into an expression. Prefix forms,
string/range updates and non-big operands are not introduced. Limbo casts retain
`big`/`int`/`string` syntax; AS `to...` helpers are separate extensions.

## Deferred step generation and debug data

The step is parsed and type-checked in its header scope before the body, but its
FIR is staged in an owned temporary stream. At loop closure the compiler places
it in a separate step block, then jumps back to the test. Body/cursor contexts
remain independent for nested classic and snapshot loops. If every body path
exits without continue, no step block is reachable: its staged FIR is discarded.
An unused step is still parsed/type-checked; it is not executed by compilation.

Step origins are also captured privately. Only origins of emitted FIR are
published, in final FIR order with consecutive site IDs. Discarded step values
have no dangling debug events. `value=N` identifies the emitted `%vN` name,
not an array index inside a later parsed/optimized module. Authored byte spans
remain those of the header even though the step executes after the body.
Compiler request cleanup releases all retained streams and origin nodes on
success or syntax/semantic/resource failure. C99 tests alternate valid requests
with errors while step capture is active and require old outputs to survive.

No new FIR type, native module import, Dis opcode or container format is needed.
This is a FlashVM source extension; it is not copied into Inferno. Full general
Limbo/AS compilation and final source-to-Dis SBL relocation remain separate work.

## Build and run

From the repository root:

```sh
export TMPDIR="$PWD/build/tmp"
mkdir -p "$TMPDIR"
build/fir/fir1_source limbo --input fir/v1/examples/tables/counted-for.b for.fir -g for.json
build/fir/fir1 opt for.fir for.opt.fir
build/fir/fir1_table_run for.opt.fir
build/fir/fir1 dis for.opt.fir for.dis
build/runtime/disvm --jit --quantum 1 --stats --inspect-words 0 4 for.dis
build/runtime/disvm --no-jit --quantum 2048 --stats --inspect-words 0 4 for.dis
```

The reference prints 42; the tutorial captures `42, 0, 1, 0`, and table/key owner
counts finish at zero. Tests cover both optimization states, both VM modes and
quanta, missing clauses, nested loop kinds, unused failing steps, high-bit/wrapped
big updates, single key evaluation, scope errors and complete debug-event IDs.
Installed SDK acceptance compiles the same source through a freshly linked C99 client.
