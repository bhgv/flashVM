# Typed Limbo tables → FIR

[Русский](TABLES.ru.md) · [Commented source](../examples/tables/main.b)

`keys[i]` now returns a checked snapshot key, usable as `t[keys[i]]` for reads or writes.
Bounds failures remain eager even for unused keys. Keys retain their immutable snapshot
when locals or the original table are replaced; numeric/text namespaces stay distinct.
See [Extension status](EXTENSIONS.md).

`fir1_source` 0.19 provides an explicit `value(): big` source profile using our Coco/R
parser and C99 semantic lowering. This is not the complete Limbo language or the
Inferno bootstrap. `source_semantics limbo_tables` records the selected profile.

Supported: `table of big` declarations/creation, inferred locals, `big` and string
locals, plain/exact decimal big literals, numeric/string indices, literal dotted
members, table aliases, explicit `[first:end]` slices, `keysof`, `len`, and a big
return and terminal if/else decisions. Source loops,
omitted slice bounds, other table element types and general modules remain outside
this subset. A declaration without initialization cannot be read.

`t.k` and `t["k"]` emit the same string constant and `table.index`/`table.load`
operations. A local named k does not affect dotted lookup. A write captures its
receiver/key once before evaluating the RHS. Static typing rejects non-big table
values, non-table receivers, invalid local assignments and non-big returns.

## FIR contract

| Operation | Inputs | Result |
| --- | --- | --- |
| `const.i64` | exact signed decimal | `i64` |
| `table.new` | none | `table.big` |
| `table.index` | `table.big`, `i64`, `string` or `table.key` | `table.slot` |
| `table.load` | `table.slot` | `i64` |
| `table.store` | `table.slot`, `i64` | void |
| `table.len` | `table.big` | `i64` |
| `range.big` | two `i64` bounds | `big.range` |
| `table.slice` | `table.big`, `big.range` | `table.big` |
| `table.keys` | `table.big` | `keys` |
| `keys.at` | `keys`, `i64` position | `table.key` |
| `keys.len` | `keys` | `i64` |

A slot is a typed owner/key location, not a native address. Missing reads fail;
stored zero remains a successful value. Slice bounds are half-open; slices copy
the numeric sequence and omit dictionary keys. Table assignment preserves owner
identity, while slices and key snapshots are independent. `len table` counts the
contiguous numeric prefix; `len keys` counts every key in that snapshot.

New operations verify operand dominance, exact arity and types. Table operations
retain conservative effects: the optimizer neither merges reads across writes
nor removes unused potentially failing loads. Source debug records cover every
SSA definition and the exact written member/key occurrence independently of RHS.
Strings retain FIR's existing UTF-8/escape limits: embedded NUL, Unicode escape
spelling and unsupported escapes fail rather than truncating a key.

## Explicit hosted reference execution

The optional `fir1_tables` C99 library and `fir1_table_run` CLI execute the verified
acyclic table subset against the same scalar C core as the native Bigtable module.
They own temporary tables/snapshots until invocation retirement and preserve
aliases without double-free. Errors retire every owner and publish no partial
traces. This is reference execution, not a Dis interpreter or JIT.

Standalone configuration adds an explicit dependency:

```sh
cmake -S fir/v1 -B build/fir \
  -DCOCOR_ROOT=/absolute/coco-r-C \
  -DCOCOR_EXECUTABLE=/absolute/cocor \
  -DFIR_TABLE_CORE_ROOT=/absolute/flashVM/common/table
cmake --build build/fir
build/fir/fir1_source limbo --input fir/v1/examples/tables/main.b tables.fir -g tables.debug.json
build/fir/fir1 check tables.fir
build/fir/fir1 opt tables.fir tables.opt.fir
build/fir/fir1_table_run tables.fir
build/fir/fir1_table_run tables.opt.fir
```

Both executions print 42. Set TMPDIR to an existing writable directory on
restricted hosts. Without FIR_TABLE_CORE_ROOT the source compiler/verifier still
build independently; the optional runner/library do not build. Workspace builds
provide the owned core explicitly and ship the matching CLI/library/example.

The [Dis table backend](TABLES_DIS.md) now emits checked native Bigtable calls and
precise frame/data GC maps. It builds without the optional reference core and
executes the image on the matching hosted Dis runtime. SWO/AS table lowering
remains pending. No new Dis opcode or Inferno extension is used.

[Reflection and standard Limbo big/string cast syntax](TYPE_REFLECTION.md).

[Exact big arithmetic and unary precedence](BIG_ARITHMETIC.md).

[FIR 0.19: acyclic table CFG / Ациклический CFG таблиц](TABLE_CFG.md) · [Русский](TABLE_CFG.ru.md).

Compiler 0.31 / Компилятор 0.31: [hybrid constructors](TABLE_LITERALS.md), [гибридные конструкторы](TABLE_LITERALS.ru.md).
