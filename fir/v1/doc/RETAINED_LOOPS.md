# Retained loop variables — compiler 0.40

[Русский](RETAINED_LOOPS.ru.md) · [Commented tutorial](../examples/tables/retained-loops.b) ·
[Typed scalar loops](TYPED_LOOPS.md) · [Captured traversal](LOOPS.md)

The Coco/R Limbo big/table profile now permits initialized `string`, `table of
big`, key-sequence and snapshot-key locals to be rebound across loop backedges
and structured loop-if joins. CLI and the reusable C99 compiler API share the
same lowering. Assignments retain their exact source type.

```limbo
current := table of big {answer=40};
original := current;
for (index := int 0; index < int 2; index++) {
    current = table of big {answer=current.answer+1};
}
# current.answer is 42; original.answer is still 40.
```

An alias captures a payload value. Reassigning its source variable changes that
variable's private frame cell; it does not redirect existing aliases. Tables
still have mutable shared contents: writes through aliases to the same table
remain visible through each alias. Strings and key snapshots keep value/immutable
semantics. The existing retained table assignment lease covers captured table
receiver/key, RHS evaluation and final publication.

String scalar and slice assignment also works inside loops because its resulting
immutable string can be stored back to the source variable. Existing rune-based
bounds, negative indexes and slice resizing/self-overlap rules apply unchanged.

```limbo
text := "xy";
while (len text < int 5) {
    text[int 0:int 1] = text; # 2 → 3 → 5 runes; RHS is captured before replacement.
}
```

## Private FIR cell contract

FIR adds four append-only types and three operations. Existing type/opcode enum
values remain unchanged. Both FIR and source parsers are generated with Coco/R.
No Dis opcode, native pointer ABI, module import or SWF format is added.

| Cell type | Exact payload type | NC64 frame layout |
| --- | --- | --- |
| `string.cell` | `string` | One mapped owner word |
| `table.cell` | `table.big` | One mapped owner word |
| `keys.cell` | `keys` | One mapped snapshot-owner word |
| `key.cell` | `table.key` | Mapped snapshot owner + unmapped scalar position |

```fir
%text: string = const.string "40"
%cell: string.cell = ref.local %text
%before: string = ref.load %cell
%replacement: string = const.string "42"
ref.store %cell, %replacement
%after: string = ref.load %cell
```

`ref.local` always takes an initialized, exactly typed payload. `ref.load` returns
an independently retained ordinary SSA payload. `ref.store` accepts only the
cell's exact payload type. Cell handles cannot be copied, reflected, traced,
sent or returned; they stay private to the function frame. Operand dominance
ensures that every use follows initialization on all incoming paths.

The effect model marks local/store as memory read+write with possible failure,
and load as memory read with possible failure. The peephole optimizer therefore
cannot merge loads across a store or erase ownership publication as dead pure
arithmetic. Tests retain both a before-store and an after-store read in the
optimized module and compare their independent values.

## Ownership, loops and GC

Before a loop test or structured loop-if, the source compiler promotes active
locals into typed cells; unreadable declarations reserve zero/nil storage. A nested loop reuses a dominating cell.
Body-local cells initialize on every entry; sibling terminal arms restore their
incoming cell identities. Uninitialized outer declarations still cannot become
definitely initialized merely because a possibly empty body writes them.

The Dis backend maps each cell's actual owner word and every returned payload
owner as separate roots. It emits existing retaining `MOVP` instructions:
retain the new owner, publish it and release the previous destination owner.
A snapshot key also copies its scalar position; that second word is never marked
as a heap pointer. Repeated cell initialization releases the previous retained
value rather than clearing its pointer without disposal. Roots remain bounded
by the compiled frame; the existing backend conservatively retains SSA payload
slots until they are overwritten or the frame is released.

The independent reference runner captures retained string bytes on publication,
including slices, embedded U+0000 and strings produced in reusable numeric
format buffers. Its invocation-local owner registry keeps those snapshots alive
until cleanup and retains its existing bounded owner budget. This is independent
of native VM reference counts and does not model their allocation footprint.

These cells are local compiler storage. External projections and channel
capabilities continue to use their separate broker/ownership contracts; private
cells do not export frame addresses or introduce an object mutex API.

## Captured receivers stay stable

`for-in` evaluates its receiver once. Rebinding the source string, table or key
sequence in the body does not change the captured receiver. Direct table traversal
continues to use a key snapshot and live payloads from that captured table;
key-sequence traversal never reloads a newly assigned sequence variable.

```limbo
text := "A😀Я";
for (index, rune in text) {
    text = "changed"; # Still exactly three iterations over the original string.
}
```

Ordinary loop conditions, in contrast, read the current payload on every test.
A header step may rebind a reference, and continue reaches that step once. Break
and return skip it; discarded step FIR/debug origins keep their existing rules.

## Build, execute and verify

From the repository root after building:

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/tables/retained-loops.b retained.fir -g retained.json
build/fir/fir1 opt retained.fir retained.opt.fir
build/fir/fir1_table_run retained.opt.fir
build/fir/fir1 dis retained.opt.fir retained.dis
build/runtime/disvm --jit --quantum 1 --stats --inspect-words 0 4 retained.dis
build/runtime/disvm --no-jit --quantum 2048 --stats --inspect-words 0 4 retained.dis
```

The reference result is 42. Native captures are `42, 0, 1, 0`: two result sites
followed by executed flags. Table/key owner counts finish at zero. Both installed
SDK acceptance paths compile the same tutorial through a freshly linked C99 client.

`fir1_retained_loops` and `disvm_retained_loops_{jit,interpreter}` test raw and
optimized FIR, both scheduling quanta, aliases, self-assignment, empty owners,
Unicode/NUL text, numeric-buffer snapshots, reference rebinding in steps and
branches, captured receivers, key owner/position copies, nested scopes and early
returns. Independent raw FIR fixtures cover all four cell types, incompatible
stores/loads/initializers, cell escape/copy rejection, non-dominating initialization
and preservation of an old SWF object on unsupported lowering. Reused C99 source
requests alternate retained loops with syntax/semantic failures.

The source extension stays in FlashVM. General ADTs, arrays/tuples, dynamic `any`
payloads, escaping reference cells and general-purpose Limbo/AS compilation remain
outside this profile. The NC64 Dis backend supports these cells; SWF lowering
rejects them before replacing a caller's output.


[Compiler 0.41](FUNCTION_FLOW.md) extends structured conditions and blocks to function scope.


[Compiler 0.42](JOIN_INITIALIZATION.md) adds all-path initialization and independently verified `ref.nil` storage.

[Compiler 0.43](LOOP_INITIALIZATION.md) permits first writes inside loop bodies while preserving read checks and conservative exit facts.
