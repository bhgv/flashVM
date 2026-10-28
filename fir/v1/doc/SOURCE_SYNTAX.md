# Working numeric source profile 0.9.0

This page retains baseline/versioned syntax notes. See [current coverage](COVERAGE.md), [RU](COVERAGE.ru.md) and [signed32 bits](INT_BITS.md) for implementation 0.52.

This profile actually lowers into FIR, separately from the broader syntax
frontends. [Grammar](../compiler/src1.atg), [C99 lowering](../compiler/lower.c).

| Syntax | AS1/AS2/AS3 | Limbo value() |
| --- | --- | --- |
| `+`, `-`, `*`, groups, unary `+/-` | f64 | i32 wrap |
| `.5`, `2.`, `1e2`, `0x2a` | yes | rejected |
| `2r101010`, `16r2a` | rejected | radix 2..36, checked digits/range |
| `var a=1,b=a+1;` | yes | rejected |
| `const x:Number=1;` | AS3 only, immutable | rejected |
| `x: int=1;`, `x:=1;` | rejected | yes |
| `x+=1; x-=1; x*=2;` | yes | yes |
| `# comment` | rejected | yes |
| `;` | empty statement | empty statement |
| Nested `{ ... }` | rejected | lexical scope and shadowing |

Precedence: unary, then */% (integer remainder only), then +/-. Binary operators associate left-to-right.
A signed literal admits i32 minimum; minus on groups/variables lowers as multiply
by -1, preserving AS signed zero. ++/-- are rejected, never interpreted as two
unary signs. This baseline describes i32; the separate big/table and real profiles are summarized in current coverage. Range,
radix digits, const assignment and AS1/AS2 const declarations are checked without
overwriting an existing destination on failure. Declarations bind left-to-right,
without hoisting or implicit globals.

[AS2 example](../examples/as2/syntax.as), [AS3 const](../examples/as3/syntax.as),
[Limbo example](../examples/limbo/syntax.b). Adjacent syntax.txt files give expected
traces. Use fir1_source, fir1_opt, fir1 object and swl link as in the
[complete pipeline](PIPELINE.md). Caller-owned streams and C APIs are retained.
AS2/AS3 allow only Number annotations. No strings, objects, AS if/loops/functions,
Limbo channels or general module ABI. // and /* */ in Limbo remain compatible
with old tutorials; # is the native comment. Limits: 64 KiB, 256 names, 1000 SSA
values, 64 block/group/unary levels, 255-byte tokens.

AS additionally supports / and /=, with multiplication precedence and left
associativity. Compiler 0.27 supports signed int / and % plus /= and %=; live integer division/remainder is currently Dis-only. Tests cover -0
divisors, NaN and preservation through optimization. Source compilation writes
matched source_language/source_semantics fields, retained across round trips and
optimization. Division requires SWL >=0.3.2.

[Limbo lexical blocks / Лексические блоки](LEXICAL_SCOPES.md).

## Structured Limbo control flow, source CLI 0.24

The int/real profiles accept initialized declarations, assignments, lexical blocks,
if/else (including omitted else and else-if), while, innermost break/continue and
return. Open branches rejoin through explicit private numeric cells; terminal
branches keep no fallthrough edge. A conservative loop exit requires a later
return even when its test is constant true. Numeric for, mixed casts and source
calls remain unsupported. [Exact flow/cell contract](NUMERIC_FLOW.md).

Conditions use the shared [grouped precedence hierarchy](GROUPED_CONDITIONS.md).
Coco/R FlowBody/FlowCondition and numeric/table repetition FIRST sets stay separate;
unsupported statement starters reject without non-consuming recovery loops.
Source limits remain 64 KiB, 1000 SSA values, depth 64 and 256 shared blocks.
Synthetic cells are compiler-owned, scalar and nonescaping. [Examples](../examples/dis/README.md).

## Homogeneous Limbo real, 0.9.0

The `value: fn(): real;` interface and `value(): real` definition must agree.
Coco/R ResultType/BindingType select the profile. All function values are real:
explicit `x: real = 2.5`, inferred `x := 2.5`, assignments and lexical blocks.
Supported operators are + - * /, unary signs and return; # remains a Limbo comment.
Decimal literals need a fraction or exponent. Integer/radix/hex literals, int
locals and mixed numeric casts are rejected. Grouped comparisons, if/else joins and while are supported by the [current real condition contract](REAL_CONDITIONS.md). This is a bounded
profile, not a general Limbo type system. int functions use the same structured flow with exact i32 arithmetic.
`real` remains a valid AS1/2/3 variable name, but is a Limbo type keyword.
The new `limbo_real` provenance survives print/opt and reaches SWO/SWF or Dis.
[Commented example](../examples/dis/README.md#limbo-real).

Nonzero finite subnormal literals are accepted even when host strtod reports
ERANGE. Overflow and underflow rounded to zero remain source errors.

## Typed table extension

The `limbo_tables` profile, exact `i64` constants and typed table/location/range/key operations are described in [TABLES.md](TABLES.md). They are accepted by the updated parser/verifier/optimizer; the [hosted Dis table backend](TABLES_DIS.md) emits checked module calls, while AVM1 SWO rejects them explicitly.

[Checked snapshot keys and exact extension status](EXTENSIONS.md).

[Reflection and standard Limbo big/string cast syntax](TYPE_REFLECTION.md).

[Compiler 0.25: captured compound updates / Составные обновления](COMPOUND_UPDATES.md).

[Compiler 0.26: Unicode strings / Строковые операции](UNICODE_STRINGS.md).

Compiler 0.27 / Компилятор 0.27: [int + - * / %, += -= *= /= %=](INT_ARITHMETIC.md),
[русское руководство](INT_ARITHMETIC.ru.md). [Current homogeneous int bits/shifts](INT_BITS.md) / [актуальные биты/сдвиги](INT_BITS.ru.md).

[Lazy numeric conditions / Ленивые числовые условия — FIR 0.55](NUMERIC_LOGIC.md) · [Русский](NUMERIC_LOGIC.ru.md).

[Grouped numeric conditions / Группировка числовых условий — FIR 0.56](GROUPED_CONDITIONS.md) · [Русский](GROUPED_CONDITIONS.ru.md).

FIR 0.57: [current numeric flow EN](NUMERIC_FLOW.md) / [текущий числовой CFG RU](NUMERIC_FLOW.ru.md).
