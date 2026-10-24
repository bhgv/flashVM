# Working numeric source profile 0.4.1

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
values, 64 group/unary levels, 255-byte tokens.

AS additionally supports / and /=, with multiplication precedence and left
associativity. The Limbo i32 profile explicitly rejects them. Tests cover -0
divisors, NaN and preservation through optimization. Source compilation writes
matched source_language/source_semantics fields, retained across round trips and
optimization. Division requires SWL >=0.3.2.

[Limbo lexical blocks / Лексические блоки](LEXICAL_SCOPES.md).
