# Working numeric source profile 0.9.0

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

Precedence: unary, then */ (AS division only), then +/-. Binary operators associate left-to-right.
A signed literal admits i32 minimum; minus on groups/variables lowers as multiply
by -1, preserving AS signed zero. ++/-- are rejected, never interpreted as two
unary signs. The Limbo profile remains i32; big/real are not implemented. Range,
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
associativity. The Limbo i32 profile explicitly rejects them. Tests cover -0
divisors, NaN and preservation through optimization. Source compilation writes
matched source_language/source_semantics fields, retained across round trips and
optimization. Division requires SWL >=0.3.2.

[Limbo lexical blocks / Лексические блоки](LEXICAL_SCOPES.md).

## Limbo terminal decisions 0.7.0

After the prelude, accept return or `if (a relation b) { result } else { result }`,
where each result is another terminal result. Both numeric operands are evaluated
once; relations are `<`, `==`, `>`, `!=`, `<=`, `>=`. Assignments inside arms,
continuation after the decision and general bool expressions are not supported;
no phi or merged environment is invented. Limits are shared depth 64 and at most
127 decisions/255 blocks within the existing 1000 SSA values. Coco/R productions
LimboResult/LimboCondition/Relation in [src1.atg](../compiler/src1.atg) call
[lower.c](../compiler/lower.c). [Example and both backends](../examples/dis/README.md).

## Homogeneous Limbo real, 0.9.0

The `value: fn(): real;` interface and `value(): real` definition must agree.
Coco/R ResultType/BindingType select the profile. All function values are real:
explicit `x: real = 2.5`, inferred `x := 2.5`, assignments and lexical blocks.
Supported operators are + - * /, unary signs and return; # remains a Limbo comment.
Decimal literals need a fraction or exponent. Integer/radix/hex literals, int
locals, casts and real comparisons/decisions are rejected. This is a bounded
profile, not a general Limbo type system. int functions retain the old profile.
`real` remains a valid AS1/2/3 variable name, but is a Limbo type keyword.
The new `limbo_real` provenance survives print/opt and reaches SWO/SWF or Dis.
[Commented example](../examples/dis/README.md#limbo-real).

Literal range errors are rejected too, including underflow when host strtod sets
errno. This is a limitation of the current bounded numeric profile.

## Typed table extension

The `limbo_tables` profile, exact `i64` constants and typed table/location/range/key operations are described in [TABLES.md](TABLES.md). They are accepted by the updated parser/verifier/optimizer; the [hosted Dis table backend](TABLES_DIS.md) emits checked module calls, while AVM1 SWO rejects them explicitly.
