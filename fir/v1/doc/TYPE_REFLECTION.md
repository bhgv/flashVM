# Limbo typeof and explicit big/string casts

[Русский](TYPE_REFLECTION.ru.md) · [Extension matrix](EXTENSIONS.md) ·
[Complete commented module](../examples/tables/main.b) ·
[AS projection helpers](../../../ide/as/projection-values.as)

Our Coco/R big-result Limbo profile supports `typeof expression`, `big expression`
and `string expression`. Groups may delimit their operand. These prefix operations
bind recursively in the current straight-line subset. `tobig` and `tostring` are
AS helpers; they are not new Limbo keywords. This is a documented supported
conversion subset, not a claim of implementing every standard Limbo cast.

| Operand | typeof result |
| --- | --- |
| big | `"big"` |
| string | `"string"` |
| table of big | `"table of big"` |
| keysof snapshot | `"keys"` |
| snapshot key | Actual `"big"` or `"string"` at execution time |

The snapshot-key result comes from its retained immutable owner's key kind. It
never reports the internal `table.key` representation. Reflection neither coerces
a value nor invokes a user method. The operand is evaluated first: `typeof t.bad`
still fails on a missing key. Reflection does not bypass a failing cast either.

```limbo
# Inside the value(): big body; the complete module is linked above.
t := table of big;
t[7] = big 10;      # Numeric key namespace.
t["7"] = big 20;    # Independent text key namespace.
keys := keysof t;
k := keys[0];       # Numeric key 7, retaining the immutable snapshot.
kind := typeof k;   # Runtime result "big".
text := string k;   # Explicit copied text "7", deliberately changing namespace.
value := t[text];   # Read text key "7", giving 20.
return big string value; # Exact decimal roundtrip with Limbo cast syntax.
```

## Exact copied conversions

`big` accepts big, text and snapshot-key operands. Big is unchanged. Numeric keys
copy their exact index. Text, including text keys, must be a complete decimal
integer: optional `+` or `-`, at least one ASCII digit, and no whitespace, radix
prefix, fraction, exponent or embedded zero. Leading zeroes are decimal. Overflow
outside [-9223372036854775808, 9223372036854775807] raises `invalid big conversion`.
No partial value, silent zero, narrowing or floating-point intermediate is used.

`string` accepts the same operand types. Text keeps its exact bytes/Unicode and
embedded zeros. Big and numeric keys format canonically in decimal; signed zero
becomes `"0"`. These are copied value operations; a cast never writes through the
original table/key or performs automatic channel/proxy access.

Tables, snapshots and other profile types are rejected as cast operands. The
language-level any/proxy adapter, heterogeneous tables, real/float/int conversions
in this profile and table CFG/loops remain separate work.

## FIR, Dis and ownership

FIR appends `value.type`, `value.big` and `value.text`; existing operation IDs stay
unchanged. Their result types are string, i64 and string. Verification checks
operand/result types, arity and dominance. Conservative effects preserve failing
unused conversions through optimization. FIRDBG1 records the exact prefix token
rather than a nested operand's position; full source-to-Dis SBL remains pending.

The NC64 backend uses existing Dis instructions and checked Bigtable imports.
Matching runtime 0.18 and compiler 0.15 are required. String results occupy genuine
mapped frame roots. Snapshot owners remain rooted across String allocation; no
native pointer crosses heaps/channels. Scalar adapters in common/table parse and
format exact int64 values without allocation. The reference executor tracks byte
lengths explicitly, including copied text, instead of truncating at embedded zero.

Tests cover signed extrema and overflow, copied keys, namespace changes, Unicode,
binary text at the native module boundary, failing unused operations, precise
source occurrences, forged owners and collection between each Dis instruction.

[Exact big arithmetic and unary precedence](BIG_ARITHMETIC.md).
