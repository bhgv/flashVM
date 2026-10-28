# Grouped numeric conditions — FIR 0.56 / source CLI 0.23

[Русский](GROUPED_CONDITIONS.ru.md) · [Lazy evaluation](NUMERIC_LOGIC.md) · [Coverage](COVERAGE.md) · [Real tutorial](../examples/dis/grouped-conditions.b) · [Int tutorial](../examples/dis/grouped-int.b)

The homogeneous Limbo int/real source profiles now use one Coco/R expression
hierarchy for arithmetic operands, relation values, parentheses and logical
operators. Ordinary groups such as `(a < b || c < d) && e < f` work without
synthetic negation. `!!(a < b)` uses ordinary unary precedence. Equality can
compare two normalized intermediate bools: `(a < b) == (c < d)` or `!=`.
The previous explicit `!(...)` syntax remains accepted.

## Precedence and strict types

The following table runs from weakest to strongest, matching the corresponding
operators in the owning Inferno Limbo grammar. Binary operators associate left;
unary operators associate right. Parentheses re-enter the same grammar; no
second character parser or heuristic scan decides whether a group is numeric.

| Level | Operators | Accepted operands |
| --- | --- | --- |
| 1 | `||` | bool, lazy RHS |
| 2 | `&&` | bool, lazy RHS |
| 3 | bitwise `\|` | int |
| 4 | `^` | int |
| 5 | `&` | int |
| 6 | `==`, `!=` | same homogeneous numeric type, or bool/bool |
| 7 | `<`, `>`, `<=`, `>=` | homogeneous numeric type |
| 8 | `<<`, `>>` | int and checked int count |
| 9 | `+`, `-` | homogeneous numeric type |
| 10 | `*`, `/`, `%` | homogeneous numeric type; remainder requires int |
| 11 | unary `+`, `-`, `~`, `!` | numeric signs, int complement, bool negation |

In Markdown the bitwise union spelling above is the single vertical-bar operator.
Use `((flags & mask) == expected)` to make bit/equality precedence explicit.
`1 | 2 == 3` parses as `1 | (2 == 3)` and rejects bool/int mixing. Ordered chains
such as `1 < 2 < 3` parse but reject the resulting ordered bool operand.
`(1 < 2) == 1` also rejects; no numeric truthiness or implicit bool/int conversion
is introduced. General Limbo comparison-to-int conversion remains future work.

Every numeric SSA producer now records its static kind in the existing bounded
catalog. Arithmetic, shifts, signs, complement, identity alias casts, local
initialization/reassignment and returns check that kind before publishing output.
Thus `return +(1 < 2)`, `x := (1 < 2)` and an int alias cast of a bool cannot leak
an intermediate condition into the homogeneous numeric API. Intermediate bools
remain available to conditions, equality and logical operations only. AS numeric
profiles retain their existing arithmetic subset and reject these Limbo relations.

## Evaluation, debug and runtime ownership

Lazy branch creation and private bool-slot merging are unchanged. Each operand
is evaluated once in source order. Grouping can change which checked RHS executes;
for example `(1 == 1 || (1 / 0 == 0)) && !!(2 < 3)` skips the failing division.
Taking that RHS still raises the existing checked error before and after
optimization. NaN ordering/equality and signed-zero behavior retain the IEEE
contract; logical negation never substitutes an inverse ordered relation.

FIRDBG1 retains exact UTF-8 spans for relation, logical and each unary `!` token,
including repeated negation. Parentheses create no artificial source event.
Comments containing unmatched parentheses are skipped by the generated scanner.
The 64-level group/unary/decision depth budget and existing SSA/block limits are
shared. After SSA-budget failure, condition validation does not index the exhausted type
catalog. Negative tests require one structured compiler diagnostic, so a sanitizer
abort cannot count as successful rejection. Located semantic rejection leaves
caller FIR/debug outputs unchanged.
The serialized blocking C99 API and worker/channel integration policy are unchanged.

Only static compiler metadata and existing scalar bool instructions are used.
There is no new FIR opcode, Dis opcode, object/container format, frame layout,
GC root or cross-heap pointer. The existing Dis and SWO/SWL backends execute the
same raw/optimized CFG. This does not complete the Limbo compiler, final debug
PC maps, general numeric for/calls or IDE Build/Run/Debug panels.

## Tutorials and reproducible commands

Both tutorials comment every nonblank source line. The real example tests
interval predicates for equality and rejects NaN with repeated negation; the int
example combines a masked bit result with grouped lazy conditions.

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/dis/grouped-conditions.b build/grouped.fir -g build/grouped.json
build/fir/fir1 opt build/grouped.fir build/grouped.opt.fir
build/fir/fir1 dis build/grouped.fir build/grouped.dis
build/runtime/disvm --jit --quantum 1 --stats --inspect-words 0 4 build/grouped.dis
build/runtime/disvm --no-jit --quantum 1 --stats --inspect-words 0 4 build/grouped.dis
build/fir/fir1 object build/grouped.fir build/grouped.swo
build/linker/swl link build/grouped.swf build/grouped.swo
```

The first real capture has binary64 bits 4631178160564600832 (42.5), the unused
capture is zero, and flags are 1/0. Substitute grouped-int.b for an exact first
capture 42 and flags 1/0. Repeat backend commands with grouped.opt.fir; count
surviving traces rather than assuming optimization keeps the same slots. A
linked SWF trace prints 42.5 or 42 respectively. C99 embedding uses the same
public source/FIR libraries; the relocated SDK compares its bytes with CLI output.

Acceptance includes 192 int/real truth-table/forms, grouped lazy checked errors,
NaN, masking/precedence, comments, every unary/logical origin, type/limit rejection
and unchanged output. The public C99 fixture interleaves 16 successful compilations
with 24 located transactional rejections. Runtime checks inspect actual interpreter/
native counters, exact captures and owners. Physical ARM64/RV64 remains separate.

FIR 0.57: [numeric cells/if/while EN](NUMERIC_FLOW.md) / [числовые ячейки/if/while RU](NUMERIC_FLOW.ru.md).
