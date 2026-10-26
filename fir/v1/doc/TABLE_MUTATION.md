# Count, membership and deletion in Limbo tables

[Русский](TABLE_MUTATION.ru.md) · [Commented tutorial](../examples/tables/mutation.b) ·
[Hybrid tables](TABLES.md) · [Dis backend](TABLES_DIS.md).

Compiler 0.21.0 adds three typed operations to our bounded Coco/R Limbo table
profile. Runtime 0.24.0 implements the matching hosted Dis module helpers. This
is an experimental FlashVM/DisVM language extension, not an Inferno language ABI
change. Source grammar remains authoritative in compiler/src1.atg; FIR spelling
and parsing remain authoritative in compiler/fir1.atg. Generated parsers are build
outputs, not handwritten sources.

## Source rules

| Source | FIR result | Meaning |
| --- | --- | --- |
| `countof entries` | i64 | All numeric/string pairs, independently of `len`. |
| `key in entries` | bool | True for a present value, including zero; false when absent. |
| `delete entries.name` | bool | Remove literal string key; no value load precedes deletion. |
| `delete entries[key]` | bool | Remove numeric/text/snapshot key; false when already absent. |

Deletion can be a statement or an expression. It never compacts numeric indices.
The receiver and key are evaluated once, in source order. Membership evaluates
its left key then its right table. `in` binds below bitwise OR; use parentheses
when embedding membership in a numeric conversion. `countof` is a bounded prefix
operator. Dotted spelling remains a literal string key, not a variable lookup.

A bool can be inferred into a local and used directly in `if`. Explicit `int`,
`big` and `string` casts produce normalized 0/1 numeric/decimal values. `typeof`
reports `int` for this logical result, matching the Limbo convention; FIR retains
its distinct verifier bool type. A typed `int` declaration uses `int (key in t)`.
Table values still require big: `t.flag = big (key in t)`. There is no general
implicit dynamic coercion and no transparent proxy/any implementation here.

## Observable behavior

Numeric and string keys occupy separate namespaces (`0` differs from `"0"`).
Numeric keys use big and must remain nonnegative; convert an int explicitly with `big index`. Negative index construction and invalid
snapshot indices raise eagerly, even if the membership/deletion result is unused
or optimization runs. Absence itself does not raise for membership or deletion;
ordinary missing table loads still raise. A zero value remains present.

`len t` is the contiguous zero-based numeric prefix. `countof t` counts every
entry, including sparse indices and text keys. Deleting zero may reduce len to
zero while later indices remain readable. A copied `keysof` snapshot stays
immutable and independent of the mutable table. Its retained key capability can
be reused after deletion or reinsertion; membership and deletion consult the
current table entry with that original key identity, not the snapshot ordinal.
Aliased tables observe the same mutation. Snapshots do not expose native pointers.

## FIR and optimizer contract

Three IDs are appended, preserving old operation IDs and textual FIR 1.0:

```text
%all: i64 = table.count %table
%slot: table.slot = table.index %table, %key
%exists: bool = table.has %slot
%removed: bool = table.delete %slot
```

The verifier checks operand/result types, arity and dominance before any backend
output. Count/has carry READS_MEMORY | MAY_THROW. Delete additionally carries
WRITES_MEMORY. They cannot be discarded or moved through writes merely because
an SSA result is unused. The reference runner and optimized/unoptimized Dis paths
share the same missing/invalid distinctions. Source debug events preserve the
operator locations; location/key checks retain their own eager origins.

## Dis ownership and compatibility

Lowering uses existing checked LOAD/MFRAME/MCALL instructions and native scalar
cells. No new container tags, native ISA instructions or pointer wire formats are
introduced. Old programs retain their original 21 named imports. Modules using
new operations add count, delindex, deltext, delkey and present imports (26 total),
with signatures checked by the loader. Existing exported signatures stay unchanged.
New programs need the matching updated runtime; the loader rejects missing helpers.

The native present helper maps Ok to 1 and Missing to 0, raising for invalid/OOM
statuses. delkey resolves a checked snapshot key and deletes that logical key.
Table/string/snapshot owners are rooted across prepared calls, including GC at
every instruction. Status/big/bool return cells remain unmapped scalars. The new
operations are owner-local; remote object projections remain explicit channel
interfaces rather than hidden cross-heap pointers.

## Run the tutorial

From the repository root after building fir and disvm-runtime:

```sh
build/fir/fir1_source limbo --input fir/v1/examples/tables/mutation.b mutation.fir
build/fir/fir1 opt mutation.fir mutation.opt.fir
build/fir/fir1_table_run mutation.opt.fir
build/fir/fir1 dis mutation.opt.fir mutation.dis
build/runtime/disvm --jit --quantum 1 --inspect-words 0 4 mutation.dis
build/runtime/disvm --no-jit --quantum 1 --inspect-words 0 4 mutation.dis
```

Reference output is `42`. Native captures are `42, 0, 1, 0`: two arm values, then
executed flags. Only the selected arm executes. The tutorial uses zero presence,
a table alias, a snapshot-owned deletion, a repeated deletion and a sparse prefix.
Installed SDK acceptance relinks fresh C99 compiler/reference clients and tests
both original/optimized FIR and both VM modes. table_mutation.py also checks wrong
types and output rollback, unused eager errors and boundary-owner rejection.
