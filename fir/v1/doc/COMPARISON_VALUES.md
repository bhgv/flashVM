# Comparison values and logical negation — compiler 0.23

[Русский](COMPARISON_VALUES.ru.md) · [Commented tutorial](../examples/tables/comparisons.b) ·
[Examples](../examples/tables/README.md) · [Terminal arms](TERMINAL_ARMS.md)

The Coco/R Limbo big/table profile accepts comparisons as expressions, not only
terminal conditions. Inferred locals retain the internal bool type:

```limbo
ascending := document.low < document.high;
descending := document.low >= document.high;
consistent := ascending == !descending;
return big consistent;
```

No runtime representation or container tag is added. CLI and C99 source APIs
share the same grammar and semantic actions. These are FlashVM compiler
extensions; this source change is not sent to Inferno.

## Types and evaluation

| Operands | Accepted relations | Result |
| --- | --- | --- |
| big / big | `<`, `>`, `<=`, `>=`, `==`, `!=` | bool |
| int / int | `<`, `>`, `<=`, `>=`, `==`, `!=` | bool |
| bool / bool | `==`, `!=` | bool |

Mixed kinds, ordered bools, string/table equality and implicit truthiness fail
statically. `!` accepts bool only. Use explicit casts for mixed numeric inputs.
There is still no bool declaration keyword in this bounded Limbo profile;
comparison/membership/deletion results may be inferred, copied or explicitly
converted with existing int/big/string casts. `typeof` reports bool as `"int"`
under the existing Limbo convention; FIR keeps its distinct bool type.

Each operand is evaluated once, left before right. For example,
`big (delete t.a) != big (delete t.a)` observes one successful removal followed
by one missing removal. A comparison does not make these owner operations atomic.
Potential errors remain eager even if its result is unused: an unused
`big "bad" < 1` still raises before and after optimization.

## Precedence checked against Inferno

The comparison levels follow the declarations in experiment's `limbo/limbo.y`:
stronger levels appear first.

| Level | Operators |
| --- | --- |
| Unary/postfix | existing casts, indexing and `!` |
| Product, sum | `* / %`, then `+ -` |
| Shift | `<< >>` |
| Ordering | `< > <= >=` |
| Equality | `== !=` |
| Bitwise | `&`, then `^`, then `|` |
| FlashVM membership extension | `in` |

Thus `1 < 2 == 3 < 4` compares two bools. `1 & 3 < 4` is a static error because
ordering binds before `&`; write an explicit numeric cast or parentheses for the
intended operation. Membership retains its documented low-precedence extension
level: negate it as `!(key in table)`. Equality associates left; chained ordered
comparisons fail once they attempt to compare bool with a numeric operand.
Compiler 0.24 adds [short-circuit `&&`/`||`](SHORT_CIRCUIT.md).
General mixed-type comparisons and source loops remain unimplemented. This is not full Limbo or AS expression coverage.

## FIR, Dis, optimization and diagnostics

The compiler uses existing signed `big.lt.signed`, `big.eq`, scalar conversions
and constants. Same-type int operands widen exactly to big. Bool equality widens
normalized values; inverse comparisons and `!` compare that value with zero.
There are no new FIR operation IDs, native opcodes or Bigtable imports. Existing
Dis BLTL/BEQL paths materialize normalized bool cells; no floating conversion is
used for adjacent integers above 2^53 or signed extrema.

Pure comparisons can fold or be reused. Checked conversions and memory mutations
retain their effects and order. Exact authored comparison bytes remain in source
sidecar events, including UTF-8 prefixes. Helper operations retain the operator's
origin instead of attributing failures to a later token. Resource errors are
sticky: an exhausted SSA identity cannot be used as a type-array index while
materializing inverse comparisons. Prior FIR/sidecar files survive all failures.

## Run

After independently building FIR and runtime, from the repository root:

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/tables/comparisons.b build/comparisons.fir -g build/comparisons.json
build/fir/fir1 check build/comparisons.fir
build/fir/fir1 opt build/comparisons.fir build/comparisons-opt.fir
build/fir/fir1 dis build/comparisons.fir build/comparisons.dis
build/runtime/disvm --jit --quantum 1 --inspect-words 0 4 build/comparisons.dis
build/runtime/disvm --no-jit --quantum 1 --inspect-words 0 4 build/comparisons.dis
```

The value cells are `42, 0`, followed by executed flags `1, 0`. Relocated C99
SDK clients compile both original/optimized FIR and run both VM modes. Tests
cover all six relations and outcomes, exact signed/binary64 boundaries, int/bool
kinds, Inferno precedence, mutation ordering, eager failures, resource limits and
UTF-8 source spans. ARM64 instruction-emulation evidence remains separate from
hardware execution and full IDE/debugger acceptance.
