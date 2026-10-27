# Hybrid table constructors — compiler 0.31

[Русский](TABLE_LITERALS.ru.md) · [Commented tutorial](../examples/tables/constructors.b) ·
[Examples README](../examples/tables/README.md) · [Table contract](TABLES.md) ·
[Mutation](TABLE_MUTATION.md) · [Text/key values](TEXT_VALUES.md)

The Coco/R Limbo big/table profile accepts an optional initializer after the
existing allocation expression:

```limbo
settings := table of big {
    40, 2,
    font_size = 11,
    ["gutter-width"] = 4,
    [4] = 99,
};
```

`table of big` and `table of big {}` both allocate an empty owner. Constructors
are expressions, so postfix indexing/slicing and use inside other expressions
remain available: `table of big {42}[0]` yields 42. This is Lua-inspired field
sugar on our typed hybrid tables, not a general Lua or complete Limbo frontend.

## Fields and collisions

| Field form | Meaning |
| --- | --- |
| `expression` | Store at the next positional numeric index, initially zero |
| `name = expression` | Store under the copied string key `"name"` |
| `[key_expression] = expression` | Store under an explicit big/string/snapshot key |

Only positional fields advance the positional counter. Explicit numeric keys
never move it and do not search for free positions. Named and numeric key
namespaces remain distinct. Duplicate keys are legal; the last executed store
wins, including collisions between explicit and positional numeric fields:

```limbo
a := table of big {[0] = 99, 42}; # a[0] is 42.
b := table of big {42, [0] = 99}; # b[0] is 99.
```

All stored values are big; cast int/bool explicitly with big where needed.
Nested tables/strings/any are not added as stored value types. A nested constructor
can still be consumed by a scalar expression such as `len (table of big {1, 2})`.
Bare field names follow the existing ASCII identifier rules. Use bracket text
keys for punctuation, Unicode or reserved keywords. An identifier without a
following assignment remains a positional expression reading its normal binding.
An initializer reads an outer shadowed binding; its new local is bound only
when initialization succeeds. No implicit self-reference or new binding is made
for a named key.

The existing array/dictionary rules are unchanged: len counts only the contiguous
numeric prefix, countof counts all entries, keysof captures independent keys,
aliases share an owner and numeric slices are independent copies. Explicit numeric
keys must be nonnegative; quoted negative text keys retain the string namespace.

## Evaluation and failures

The owner is allocated first, then fields execute once in source order. For each
field, its key is evaluated and its location is validated before the RHS. The
value is stored only after successful RHS evaluation. This preserves ordinary
table assignment ordering: `[-1] = 1/0` reports the invalid key first. Prior owner
operations and externally visible RHS effects are not rolled back on a later
failure; constructors are not remote/channel transactions.

One trailing comma is allowed after a nonempty field list. Empty/repeated fields,
missing separators, missing assignment/value, wrong key kinds and non-big values
are rejected. Comments and whitespace may appear around assignment or a trailing
comma. Flat fields use an iterative grammar rule; nesting shares the existing
64-level expression limit and all normal byte/name/SSA budgets remain enforced.
Source failures preserve old FIR/debug destinations. Stores retain the exact
named-key, opening-bracket or first positional-token origin in debug JSON.

## FIR, runtime and C99 services

The authoritative grammar is src1.atg. Small non-consuming lexical predicates
resolve `name =` versus an ordinary identifier expression, distinguish `=` from
`==`, and admit one trailing comma after trivia. Expressions remain parsed by
Coco/R. Generated parser files stay in build directories.

Lowering emits only existing table.new, exact const.i64/string keys, table.index
and table.store. The positional helper emits a fresh big constant without reading
stale token text. No FIR operation/type ID, Dis opcode, builtin helper ABI or
container representation is added. Existing effects preserve eager location/RHS
errors and stores through optimization. Both interpreter and JIT use existing
hosted table services and frame roots.

A native C99 service test repeatedly compiles valid constructors, syntax failures
and SSA-limit failures in one process, then checks recovery through parsing,
verification and Dis output. Caller-owned streams/debug destinations remain
staged; parser calls require external serialization. Relocated SDK tests relink
fresh source/FIR clients and run the tutorial before/after optimization in both
VM modes. These language extensions remain FlashVM-only.

Live hybrid-table operations remain unsupported by the SWF object backend; old
output is preserved on rejection. The separate homogeneous int and AS source
profiles are unchanged. General source loops/functions, any-valued table storage,
full debugger attachment and IDE Build/Run/Debug panels remain separate work.

## Run

From the repository root after building FIR and runtime:

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/tables/constructors.b build/constructors.fir -g build/constructors.json
build/fir/fir1 check build/constructors.fir
build/fir/fir1 opt build/constructors.fir build/constructors-opt.fir
build/fir/fir1 dis build/constructors-opt.fir build/constructors.dis
build/runtime/disvm --jit --quantum 1 --inspect-words 0 4 build/constructors.dis
build/runtime/disvm --no-jit --quantum 1 --inspect-words 0 4 build/constructors.dis
```

Expected trace values: 42, 0; execution flags: 1, 0. The example verifies six
hybrid keys, a two-element numeric prefix, an independent slice and unchanged
editor-style settings after mutation through an alias. Existing AMD64/ARM64 JIT
availability remains 166 guarded fast paths and 173 complete decoded entries;
this compiler feature does not add a native fast path.
