# Coverage and boundaries

[Русский](COVERAGE.ru.md) · [Exact numeric syntax](SOURCE_SYNTAX.md) ·
[Lexical scopes](LEXICAL_SCOPES.md) · [Diagnostics](SOURCE_DIAGNOSTICS.md)

| Component | Implemented | Not implemented |
| --- | --- | --- |
| FIR reader | Coco/R, UTF-8, comments, exact FIR 1.0 | Future versions, binary encoding |
| Verifier | SSA uniqueness, dominance, types, targets, reachability, terminators | Interprocedural analysis |
| AS1/AS2/AS3 numeric adapters | trace, initialized var, AS3 const, reads/assignments, AS2/3 Number annotations | Objects, strings, source branches/loops/functions, hoisting, general language semantics |
| Limbo numeric adapter | value(): int tutorial, initialized int/inferred names, lexical blocks/shadowing, reads/assignments, terminal if/else returns | General module ABI, functions/imports, source channels, alt/spawn, big/real |
| Expressions | Numeric literals, groups, bounded unary +/-, + - *, compound assignments; AS / and /= | general boolean expressions/joins, broader types |
| FIR channels | Create/send/receive/close through existing FlashVM extensions | Source Limbo channel lowering, typed module graphs |
| Source diagnostics | First-failure C99 struct and opt-in schema-1 JSON with original byte spans | Multiple diagnostics, general AST maps, reentrant parser |
| SWF backend | SWO1 AVM1 objects; separate SWL links SWF8 | ABC/AVM2, function imports/relocations, exceptions |

AS uses binary64, including division, NaN and signed-zero behavior. Limbo uses
checked signed i32 literals and wrapping + - *; unary minus on groups/variables
is supported. AS accepts hex and decimal/exponent spellings; Limbo accepts bases
2..36. ++/-- are rejected rather than read as two unary signs. Exact syntax and
precedence remain documented separately, with the Coco/R grammar authoritative.

AS bindings are flat and initialized left-to-right, without hoisting. Limbo
blocks retire local names and restore outer bindings while retaining assignments
to outer names. Limits: 64 KiB input, 255-byte tokens, 256 total admitted names,
1000 SSA values, shared depth 64 for blocks/groups/unary recursion. Rejected
translations do not overwrite existing output. The final I/O copy is not a
rollback transaction on arbitrary output failure.

The Limbo tutorial harness traces value() instead of implementing the Inferno
loader ABI. AS3 here is AVM1 arithmetic compatibility; full AS3 needs an ABC
backend. The independent [syntax frontends](../../../compiler/README.md) recognize
broader syntax but do not thereby lower or execute the full languages.

FIR writes objects only. The [linker](../../../linker/README.md) combines
self-contained objects/libraries/resources; optimization remains explicit.
Source positions and diagnostic checks do not establish embedded Dis/JIT,
complete debugging, cross-module function calls or native backend coverage.

[NC64 Dis target 0.9.0](DIS_BACKEND.md): scalar CFG, i32/bool/f64 const/copy, signed i32 comparison/wrap and binary64 arithmetic, value+executed captures/return.
Our C99 emitter writes Dis directly; full Limbo ABI, phi, channels, SBL and Dis linking remain unimplemented.

Numeric Limbo now admits six comparisons in terminal if/else; every arm
returns. SSA joins and mutable source loops are not implemented.

Our Coco/R Limbo adapter now additionally admits a homogeneous `value(): real`
profile: decimal real constants, typed/inferred real locals, lexical scopes,
assignments, + - * / and return. No mixed types/casts or real comparisons/CFG
are implied. [Exact grammar coverage](SOURCE_SYNTAX.md#homogeneous-limbo-real-090).

Наш Limbo real — ограниченный однородный профиль с явными real-литералами;
обычные int-функции и их терминальные ветвления сохраняют прежнюю семантику.
[Комментированный пример](../examples/dis/README.ru.md#вещественный-limbo).

[Dynamic any: implemented status and proposed ADT boundary / Состояние any и проект ADT](DYNAMIC_VALUES.md).

[Compiler 0.23: comparison values / Сравнения как значения](COMPARISON_VALUES.md).

Compiler 0.27 / Компилятор 0.27: [int + - * / %, += -= *= /= %=](INT_ARITHMETIC.md),
[русское руководство](INT_ARITHMETIC.ru.md). Int bitwise/shift operations remain unsupported.

Compiler 0.29 / Компилятор 0.29: [checked int shifts](INT_SHIFTS.md),
[проверяемые int-сдвиги](INT_SHIFTS.ru.md), big/table source profile and raw FIR.
Live checked shifts are Dis/reference-only; homogeneous int/AS source profiles are unchanged.

Compiler 0.30 / Компилятор 0.30: [ordered strings and checked int casts](TEXT_VALUES.md),
[порядок строк и проверяемые int-cast](TEXT_VALUES.ru.md), in the typed big/table source profile.

Compiler 0.31 / Компилятор 0.31: [hybrid constructors](TABLE_LITERALS.md),
[конструкторы гибридных таблиц](TABLE_LITERALS.ru.md), ordered named/bracket/positional fields in the big/table profile.
