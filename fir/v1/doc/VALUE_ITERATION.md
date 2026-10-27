# One-name value iteration — compiler 0.47

[Русский](VALUE_ITERATION.ru.md) · [Commented tutorial](../examples/tables/value-iteration.b) ·
[Existing pair traversal](LOOPS.md) · [Typed iteration](TYPED_LOOPS.md)

The Coco/R Limbo big/table frontend now accepts `for (value in collection)`.
The one name receives the **value**, not the key. The existing two-name
`for (key, value in collection)` form is unchanged. This is a Limbo-profile
extension, not a change to ActionScript's for-in meaning.

| Receiver | One-name binding type/value |
| --- | --- |
| `table of big` | Copied big payload at the current captured key |
| `string` | Unicode rune `int`, not a UTF-8 byte |
| `keysof table` / captured keys | Snapshot key capability, usable as `table[key]` |

```limbo
total := big 0;
for (amount in table of big {first=20, second=22}) {
    total += amount;
}
return total;
```

To traverse keys, ask for them explicitly: `for (key in keysof entries)`.
It does not imply that an arbitrary array or tuple is admitted by the current
big/table frontend. Unsupported receivers still fail before FIR publication.

## Capture, scope and control

The receiver is evaluated once in the outer environment. Only afterward does
the loop's body scope bind the name. Thus `for (entries in entries)` captures
the outer table and shadows that name with each big payload inside the body.
The outer binding is restored afterward. A newly introduced loop name cannot
be read after the loop, and assigning the copied value does not update its table.

Tables capture an independent key snapshot but read live values. New keys added
in the body are not visited; a changed not-yet-read payload is observed. Deleting
a not-yet-read key retains the existing checked missing-key behavior. Rebinding
the source table/string/snapshot variable cannot redirect the captured receiver.
Strings and key sequences retain their immutable snapshot semantics.

Break, continue, early return, nested loops, initialization proofs and typed
assignment leases use the existing traversal lowering. No hidden source key is
created for the shortened form. The internal cursor/key values still exist when
needed for lookup, but they have no invented user binding or debug declaration.
The value declaration keeps the exact authored header token and body scope.

Coco/R parses both headers. The existing nonconsuming lookahead now recognizes
a comma or the complete keyword `in` after the first name, skipping scanner
trivia. It checks the identifier boundary so `inbox`, `in_foo` and `in1` do not
become a false `in` token. Ordinary counted-for parsing remains unchanged.
No independent source parser was introduced.

## FIR, ownership and API

Both spellings use the same typed FIR cursor, captured receiver and value loads.
There is no new FIR/Dis operation, public API or native ABI. Retained receiver and
snapshot roots remain live through one-instruction collection quanta, and the
existing optimizer keeps memory, bounds and ownership effects. Source definite
initialization remains conservative after a potentially empty loop.

The serial C99 compiler service and CLI share the generated parser. Failed
headers preserve old FIR/debug outputs; the C99 regression alternates successful
one-name requests with malformed headers to check recovery. Full ordinary Limbo
modules still use the separate retained bootstrap; this does not expand the
Coco/R profile to arbitrary module definitions or transparent `any` values.

## Run

From the repository root after building:

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/tables/value-iteration.b values.fir -g values.json
build/fir/fir1 opt values.fir values.opt.fir
build/fir/fir1_table_run values.opt.fir
build/fir/fir1 dis values.opt.fir values.dis
build/runtime/disvm --jit --quantum 1 --stats --inspect-words 0 2 values.dis
build/runtime/disvm --no-jit --quantum 2048 --stats --inspect-words 0 2 values.dis
```

The tutorial returns 42; native captures are `DATA 0 42`, `DATA 8 1`.
`fir1_value_iteration` and its two runtime variants cover all receivers, empty
inputs, shadowing, capture, live table changes, nested/pair/counted loops, control
transfers, comments and malformed/ambiguous headers. They verify source spans and
original/optimized execution at both quanta, with no live native table/key owners.
Both installed SDK paths compile and run the tutorial through freshly linked
C99 clients as well as the relocated command-line tools.
