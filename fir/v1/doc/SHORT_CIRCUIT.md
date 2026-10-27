# Lazy logical expressions and scalar FIR cells — compiler 0.24

[Русский](SHORT_CIRCUIT.ru.md) · [Commented source](../examples/tables/short-circuit.b) ·
[Comparison values](COMPARISON_VALUES.md) · [Terminal arms](TERMINAL_ARMS.md)

The Coco/R Limbo big/table profile implements true short-circuit `&&` and `||`.
Both operands must have bool type. The left operand evaluates once; `&&` evaluates
the right only if left is true, and `||` only if left is false. The result is a
normalized bool, usable in locals, explicit casts or terminal conditions.

```limbo
present := "answer" in document;
valid := present && (document.answer == 42);
selected := valid || (delete document.answer);
```

A skipped RHS performs no read, conversion, deletion, exception or owner call.
A selected RHS retains its ordinary effects, even if the final logical result is
unused. Both operands are still parsed/type-checked before optimization: a skipped
string RHS is a static error. Numeric/string truthiness is not introduced.

## Precedence and CFG

The levels follow Inferno `limbo/limbo.y`: `&&` binds above `||`, and both bind
below the existing bitwise and comparison levels. FlashVM's low-precedence `in`
extension binds above these logical operators. Parentheses can override grouping.
`!` remains unary bool negation. These source extensions are FlashVM-only; this
series does not change Inferno's language/compiler implementation.

Each lazy operation emits three private blocks: skipped, RHS and join. The left
value initializes a local bool cell. The skipped arm preserves it; only the RHS
arm stores a new value. The join loads a fresh immutable bool SSA value. Nested
operations have distinct labels/cells; cells are never commoned by the optimizer.
No RHS instruction is emitted before its guarding branch.

This adds three append-only FIR op IDs and one append-only type:

| FIR form | Contract | Effects |
| --- | --- | --- |
| `%c: bool.slot = bool.local` | Initialize one frame-local scalar cell to false | writes memory |
| `bool.store %c, %b` | Store normalized bool in a dominating cell | writes memory |
| `%v: bool = bool.load %c` | Copy current cell contents to immutable SSA bool | reads memory |

Existing type/op IDs remain unchanged; textual format remains FIR 1.0. The C99
API exposes the same verified operations. Cells cannot be copied, traced, cast,
returned as values or sent through channels. Dominance verification checks every
use. They are not pointers, GC owners, remote projections or general Limbo arrays.
A cell may be read immediately after allocation; initialization is deterministic.

## Dis and reference execution

Scalar and hosted-table Dis backends lower these operations to existing MOVW.
A cell occupies a normal scalar frame slot and never receives a GC pointer bit.
There is no heap allocation or new VM opcode/helper ABI/container tag. A pure
numeric logical program needs no Bigtable import. Table programs retain their
existing checked owner/module imports. Private cells cannot mix interpreter heaps.
The C reference runner executes the same loads/stores by value.

Memory effects prevent local initialization from being CSE'd and loads from being
reused across stores. Known branch predicates can remove unreachable RHS blocks,
including throwing operations that are truly skipped. Selected calls/conversions
remain observable. This is not a general phi/environment-join implementation,
atomic remote update or source-loop implementation. The SWF object backend rejects cell operations without replacing caller output;
full AS/SWF short-circuit lowering and full IDE Build/Run/Debug panels remain separate work.

## Limits, origins and acceptance

The frontend uses one global budget for terminal and logical blocks, at most 256
including entry. An isolated flat chain can contain 85 logical operations; adding
a terminal decision consumes two more blocks. Source value/name/nesting limits
remain unchanged. Type or resource errors preserve previous FIR and debug files.
Logical branch/store sidecar events point to the exact authored `&&`/`||` bytes,
including UTF-8 prefixes. Full Dis SBL/debugger attachment is not claimed.

Tests check both skip/take outcomes, owner mutations, bad casts and zero divisors,
original/optimized FIR, nested precedence, independent cell initialization,
non-dominating/copy/escape/type errors, exact source spans, the 256-block boundary,
and the shared terminal/logical budget. Actual Dis runs use one-instruction
scheduler/GC quanta, both dispatch modes and exact value/executed captures.

## Run the tutorial

After building FIR and runtime, from the repository root:

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/tables/short-circuit.b build/lazy.fir -g build/lazy.json
build/fir/fir1 check build/lazy.fir
build/fir/fir1 opt build/lazy.fir build/lazy-opt.fir
build/fir/fir1 dis build/lazy.fir build/lazy.dis
build/runtime/disvm --jit --quantum 1 --inspect-words 0 4 build/lazy.dis
build/runtime/disvm --no-jit --quantum 1 --inspect-words 0 4 build/lazy.dis
```

Captures: values `42, 0`, then executed flags `1, 0`. Relocated SDK acceptance
relinks fresh C99 clients and runs both original/optimized tutorial variants on
both VM modes. This does not establish physical ARM64 execution or a full OS boot.
