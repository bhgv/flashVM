# Typed scalar loops and immutable traversal — compiler 0.39

[Русский](TYPED_LOOPS.ru.md) · [Commented tutorial](../examples/tables/typed-loops.b) ·
[Counted for](COUNTED_FOR.md) · [Loop transfers](LOOP_CONTROL.md)

The Coco/R Limbo big/table profile now carries initialized `int` and inferred
`bool` locals across loop backedges and structured loop-if joins. The CLI and
reusable C99 compiler library use the same implementation. Source types remain
static: an int assignment requires int, a bool assignment requires bool, and
mixed int/big arithmetic still requires an explicit cast.

```limbo
running := 1 == 1;
counter := int 0;
while (running) {
    counter++;
    if (counter == int 3) {
        running = 1 == 0;
    }
}
```

`int` postfix `++/--` is statement-only, just like the existing big forms.
Arithmetic wraps modulo 2^32, including INT_MAX → INT_MIN; no signed C overflow
is introduced by compilation. Bool postfix updates remain invalid. Bool values
are inferred from comparisons or logical operators; this profile does not add a
new bool declaration keyword or literal spelling. As in the existing Limbo
reflection contract, `typeof` reports a normalized bool as `"int"`.

## Storage and joins

Before a loop test or a structured loop-if, initialized active scalar locals are
promoted into private dominating FIR cells. Big uses `big.local/load/store`;
bool uses `bool.local/load/store`. Int uses an existing big cell with explicit
signed widening on store and modulo-32 narrowing on load. This is an internal
storage choice: `typeof`, comparisons, arithmetic and debug read values continue
to expose int, not big. No public FIR type/opcode, Dis opcode or native import
is added. Cells neither escape a function nor represent an external proxy lock.

Nested loops reuse an already dominating cell. Sibling terminal arms restore
incoming cell identities before compiling the other arm. A body-local scalar may
be promoted within its iteration; its initializer still runs on each entry.
Uninitialized outer declarations are not made definitely initialized by a body
which might execute zero times. [Compiler 0.40](RETAINED_LOOPS.md) adds retained string/table/snapshot/key cells
for handle rebindings across joins.

## for-in types and lifetime

The receiver expression and immutable sequence length are evaluated once before traversal. The two names are
local to the body, must differ, and may shadow outer names. Continue advances
exactly once; break and return bypass the cursor step.

| Receiver | First binding | Second binding | Observed data |
| --- | --- | --- | --- |
| `table of big` | snapshot key capability | big payload | Key snapshot, live checked payload |
| `string` | int rune index | int Unicode scalar | Captured immutable string |
| `keysof table` or a key snapshot local | big position | snapshot key capability | Captured immutable key sequence |

String indexes start at zero and count Unicode scalars, not UTF-8 bytes. A
supplementary-plane scalar is one iteration. The string is immutable and retained
by normal VM roots for the traversal. The string index and payload remain int.

```limbo
for (index, rune in "A😀Я") {
    # index = 0, 1, 2; rune = U+0041, U+1F600, U+042F.
}
keys := keysof values;
for (position, key in keys) {
    delete values[key]; # Removing entries cannot invalidate this key snapshot.
}
```

A key capability preserves numeric/string identity: `values[key]` addresses the
original key space. `typeof key` reports that key's dynamic kind, rather than its
internal capability carrier. Traversing a snapshot alone never reads the table's
payload. In contrast, direct table traversal reads each live payload before the
body; deletion of a later key can therefore raise a missing-key error. Empty
strings and snapshots execute no body iterations.

## Run and validate

From the repository root, using the built tools:

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/tables/typed-loops.b typed.fir -g typed.json
build/fir/fir1 opt typed.fir typed.opt.fir
build/fir/fir1_table_run typed.opt.fir
build/fir/fir1 dis typed.opt.fir typed.dis
build/runtime/disvm --jit --quantum 1 --stats --inspect-words 0 4 typed.dis
build/runtime/disvm --no-jit --quantum 2048 --stats --inspect-words 0 4 typed.dis
```

The reference result is 42. The VM captures `42, 0, 1, 0` (two result sites and
their executed flags); table/key owner counts finish at zero. The tutorial shows
all three features and explains each operation. Both installed SDK acceptance
paths compile it with a freshly linked C99 client.

`fir1_typed_loops` and `disvm_typed_loops_{jit,interpreter}` cover raw/optimized
FIR, two scheduling quanta, int wraparound, bool exit/update, nested scopes,
terminal sibling arms, empty and Unicode strings, nested traversal, key lifetime,
type reflection, source/debug IDs, and rejected mismatched/uninitialized writes.
The C99 loop API test alternates valid typed traversal with malformed and
semantically invalid staged for headers to check request isolation.

This source-profile extension stays in FlashVM; it is not copied into Inferno.
General reference rebinding, arbitrary arrays/tuples, a full general Limbo
frontend and final source-to-Dis SBL relocation remain separate work.


[Compiler 0.41](FUNCTION_FLOW.md) extends structured conditions and blocks to function scope.
