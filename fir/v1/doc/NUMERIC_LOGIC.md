# Lazy numeric conditions — FIR 0.55 / source CLI 0.22

[Русский](NUMERIC_LOGIC.ru.md) · [Ordered real contract](REAL_CONDITIONS.md) · [Coverage](COVERAGE.md) · [Line-commented Limbo](../examples/dis/numeric-logic.b) · [Line-commented FIR cells](../examples/dis/numeric-logic.fir)

Homogeneous Limbo int/real functions now compose terminal if/else conditions with
lazy `&&`, `||` and `!(...)`. Each arm must still return. Coco/R owns all parsing;
CLI and C99 libraries invoke the same lowering actions. The big/table profile
retains its existing richer expression grammar.

| Source | Meaning |
| --- | --- |
| `a < b && c < d` | Evaluate the RHS only when the left bool is true |
| `a == b || c == d` | Evaluate the RHS only when the left bool is false |
| `!(a <= b)` | Negate the normalized bool, retaining IEEE unordered behavior |
| `a < b || c < d && e < f` | Conjunction binds above disjunction |
| `(a + b) < c` | Parentheses in arithmetic operands keep their ordinary meaning |

FIR 0.56 also admits ordinary boolean groups, repeated unary negation and bool
predicate equality. Numeric bindings/results still reject intermediate bools;
AS profiles keep their arithmetic subset. See [grouped conditions](GROUPED_CONDITIONS.md)
for the complete precedence and strict typing contract.

## Evaluation and ownership

Each relation operand evaluates once in source order. The compiler emits the
conditional edge before RHS code, using private initialized bool.slot cells to
merge results. Thus `1 == 1 || 1 / 0 == 0` skips the checked integer division;
`0 == 1 || 1 / 0 == 0` executes and fails in reference/Dis execution, including
after optimization. This series does not add checked integer division to SWF.

Negating a NaN-ordered comparison is not equivalent to another ordered relation:
`!(NaN <= x)` is true while `NaN > x` is false. Scalar cells hold normalized bools,
are distinct, initialize false and cannot escape through copy/trace/channel APIs.
They hold no GC owners. SWF represents them with object-local variable names and
standard AVM1 actions, preserving immutable load snapshots and cell independence.
No Dis instruction, shared heap pointer, container tag or native frame ABI changes.

FIRDBG1 records exact original UTF-8 relation/logical operator spans. Synthetic
stores/loads retain the operator origin. These are unoptimized source/FIR origins,
not a final Dis/SWF/JIT PC map. The existing SSA/block budgets and shared 64-level
recursion budget remain enforced transactionally. Source library calls are blocking
and serialized by the embedding host; IDE should return copied results over its
service channels from its build worker.

## Reproduce the tutorials

From the repository root after building the matching tools:

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/dis/numeric-logic.b build/logic.fir -g build/logic.json
build/fir/fir1 opt build/logic.fir build/logic.opt.fir
build/fir/fir1 dis build/logic.fir build/logic.dis
build/runtime/disvm --jit --quantum 1 --stats --inspect-words 0 4 build/logic.dis
build/runtime/disvm --no-jit --quantum 1 --stats --inspect-words 0 4 build/logic.dis
build/fir/fir1 object build/logic.fir build/logic.swo
build/linker/swl link build/logic.swf build/logic.swo
```

The source tutorial captures exactly 42.5 in its first result slot, zero in the
unexecuted second slot, then flags 1/0. Binary64 42.5 has signed word bits
4631178160564600832. Count surviving traces after optimization. For numeric-logic.fir,
skip the source stage: it emits false/true/false with validity flags 1/1/1. SWF trace
may render bools as words; numeric bit inspection belongs to Dis. Every nonblank
source line in both tutorials has a comment explaining its purpose.

## Acceptance

numeric_logic.py checks 64 truth-table/forms across int/real, precedence,
negation, skipped/taken errors, NaN, arithmetic groups, exact origins, type/depth/
block rejection and unchanged prior outputs. Each accepted case is emitted raw
and optimized. Explicit runtime/mode arguments additionally check actual native/
interpreter counters, capture bits/flags and owner retirement. Explicit FIR_SWL
and FIR_FLASHVM_RUNNER enable actual linked SWF execution. A C99 client compiles
16 times through public libraries. Relocated SDK acceptance compares CLI/library
Dis and runs the source tutorial in both modes. Physical ARM64, complete Limbo
and IDE Build/Run/Debug panels remain separate milestones.
