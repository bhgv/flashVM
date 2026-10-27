# Scoped function statements — compiler 0.41

[Русский](FUNCTION_FLOW.ru.md) · [Commented tutorial](../examples/tables/function-flow.b) ·
[Retained locals](RETAINED_LOOPS.md) · [Loop transfers](LOOP_CONTROL.md) ·
[C99 source API](../compiler/API.md)

The Coco/R Limbo big/table profile now uses one statement grammar at function,
block and loop scope. An ordinary `if` may update locals and continue afterward;
`else` is optional and `else if` chains are accepted. Braces introduce ordinary
lexical blocks anywhere a statement is allowed. Return may close any function
path; break/continue still require an enclosing loop.

```limbo
answer := big 40;
if (answer == 40) {
    answer += 2;
}
{
    answer := "local"; # Shadows only inside this block.
}
return answer;
```

The initialized-local types introduced in 0.39/0.40 remain available through
joins: big, int, inferred bool, string, table of big, key sequences and snapshot
keys. Assignments preserve their exact type. Existing lexical shadowing, table
assignment leases, immutable strings, captured for-in receivers, negative indexes
and slice replacement rules are unchanged.

## Paths, joins and initialization

Before a condition, initialized active locals are promoted into private typed
cells. Each arm has a separate lexical scope and an independent compiler binding
snapshot. Only reachable, open exits get an edge to a join. A join is omitted
when both arms return or transfer out; a missing else contributes an empty,
continuing false arm. Break/continue keep targeting the innermost loop through
nested blocks and conditions.

A previously declared uninitialized outer local may be initialized on the only
surviving arm when the other arm returns. Its SSA definition or cell then
dominates the join. First assignment keeps the declaration's original binding
identity and scope; it no longer creates an accidental inner shadow.

```limbo
answer: big;
if (failed) {
    return 0;
} else {
    answer = big 42;
}
return answer;
```

The example's `failed` is an already initialized bool. If both arms continue,
initialize outer variables before the condition. This profile still rejects
first initialization across two joining paths rather than inventing a phi or a
nil/untyped reference cell. Assignments on paths which return do not initialize
a surviving sibling. A possibly empty loop also cannot prove initialization.
The previous all-terminal decision-tree behavior remains accepted, including
independent arm-local declarations and first assignments before each return.

Every reachable function exit must return big. A function with only a conditional
return and no continuing return is rejected, even when the condition is a literal
true. Loop bodies alone do not prove total return. A statement after an all-path
return/break/continue is rejected as unreachable. The total-return check runs
after the function's closing brace, so a missing brace retains its syntax error
instead of being hidden by a later missing-return diagnostic. FIR/debug outputs
remain privately staged and unchanged on failure.

## Limits and constant folding

An if initially reserves two blocks. A third block is charged only if an open
exit actually needs a join. This preserves the existing 127-decision, 128-leaf,
255-block terminal-tree boundary. An independent sequence of 85 empty joining
conditions plus entry occupies the existing 256-block limit; an 86th is rejected.
Conditions, blocks, expression groups and nested loops share the existing depth
budget of 64. The source value/name/input limits remain unchanged.

Promoting readonly scalar locals must not hide their literals from the peephole
optimizer. A new bounded rewrite forwards big/bool cell loads only when there
is exactly one live store of a literal payload and that store shares the cell
definition's block. A read earlier in that block keeps zero-initialization
semantics. Cell dominance makes the store precede reads in other blocks; a
literal cannot change when a physical SSA slot is reused by a loop.

The rewrite does not forward multiple stores, conditional stores from another
block, nonliteral values or retained-reference loads. Normal verification runs
before and after optimization. Independent FIR fixtures check before-store zero,
after-store values, mutable stores, conditional stores and bool normalization.
The older signed-boundary comparison and full terminal-tree matrices continue
to check that literal comparisons and unreachable arms fold away.

## Ownership, metadata and targets

This is source/control-flow work using existing FIR and Dis instructions. The
private environment snapshots contain compiler IDs, never runtime heap pointers.
The existing mapped frame cells and retaining MOVP implementation preserve
reference payloads across joins. C99 API signatures and source-debug JSON format
are unchanged; both parsers remain generated with Coco/R.

Brace scopes keep their exact authored byte spans and unique IDs. The shared
initializer/body scope of a counted for is anchored at its `for` keyword.
Structured conditions retain the authored `if` span; uninitialized declarations
now have a `declare` event with `value=-1`, followed by assignments using the same
binding ID. Generated source globals remain serial request state. The in-process
API test alternates valid flow with header syntax/semantic and joined-path errors.

Unused local strings in the Limbo table profile now correctly select the hosted
Dis backend even before DCE. Numeric-profile unsupported string/dynamic operations
retain their existing preflight behavior. No new Dis opcode, import ABI, SWF
format or general Limbo/AS frontend is introduced. The int/real numeric Limbo
profile retains its documented terminal decision grammar.

## Build and run

From the repository root after building:

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/tables/function-flow.b flow.fir -g flow.json
build/fir/fir1 opt flow.fir flow.opt.fir
build/fir/fir1_table_run flow.opt.fir
build/fir/fir1 dis flow.opt.fir flow.dis
build/runtime/disvm --jit --quantum 1 --stats --inspect-words 0 10 flow.dis
build/runtime/disvm --no-jit --quantum 2048 --stats --inspect-words 0 10 flow.dis
```

The reference prints 42. The tutorial contains five return sites: exactly one
result cell contains 42 with its executed flag set; all other cells/flags are zero.
Table/key owner counts finish at zero. The installed SDK acceptance now derives
capture counts separately for original/optimized FIR and checks every result,
flag and owner count, including the direct in-process source compiler client.

`fir1_function_flow` and `disvm_function_flow_{jit,interpreter}` cover both FIR
states, two quanta, early guards, optional else, chains, all supported local
kinds, sole-survivor initialization, nested loops/blocks, transfer scope, exact
debug spans and rejected missing returns, escaping locals, incompatible writes,
joining initialization and depth/block overflow. The extension stays in FlashVM;
no language changes are copied into Inferno.
