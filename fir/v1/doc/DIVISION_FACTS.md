# Signed quotient/remainder facts — FIR 0.72

[Русский](DIVISION_FACTS.ru.md) · [Transfer](../optimiser/integer_division_bits.h) ·
[Concrete C oracle](../tests/integer_power_division_bits.c) ·
[Pipeline matrix](../tests/integer_division_facts.py) ·
[Commented Limbo example](../examples/dis/division-facts.b).

FIR derives known-zero/known-one bits for int32/big signed division and remainder
when the divisor is exactly known, positive and a power of two. It does not
rewrite arbitrary signed division into an arithmetic right shift: -9/8 is -1,
while a sign-filling right shift gives -2.

| Expression | Successful-result proof |
| --- | --- |
| `(x & 255) / 8` | Nonnegative quotient fits0..31; raw SWF checked-count admission succeeds |
| `(x & 255) % 8` | Nonnegative remainder fits0..7 |
| `(x & -8) % 8` | Zero for either sign, including MIN |
| `(x & 255) / 4` | Quotient may exceed31; int32 checked-count admission refuses |
| Division by0, negative/nonpower/unknown divisor | New transfer declines; existing behavior and errors remain |

## Algorithm and effects

The private helper constrains each possible input sign. Nonnegative inputs use
unsigned magnitude directly; negative inputs use modular unsigned negation.
Quotient magnitude shifts right, remainder magnitude keeps the low divisor bits,
and negative results restore the sign through modular negation. MIN is legal as
an unsigned magnitude; no C signed negation or division overflow occurs. Unknown
signs analyze both branches and retain only facts common to both. Correlations
may be lost; precision is not a general interval solver.

A proven complete result may become a literal only with an independent nonzero
bit proof for the divisor. Operand producers remain eager and keep their own
errors/effects. Thus an aligned zero remainder does not erase an earlier divide
by zero. Debug origins remain attached to the original folded instruction; copy
propagation/CSE may select another equivalent trace temporary.

The helper declines unsupported width/divisor without changing outputs. Work is
bounded by32/64 bits and allocates nothing. It changes no cells, user code or
thread state. Public C99 layouts, Coco/R grammar, FIR1.0, SWO and Dis/Dis64 wire
contracts are unchanged. Existing MIN/-1 wrapping and MIN%-1=0 evaluation remains.

## Validation

The independent concrete oracle compares70,170,624 signed C `/` and `%` results:
both widths, every positive divisor position, all ternary assignments of six low
bits, two upper patterns and three sign states. It also checks useful precision,
canonical masks and refusal canaries. Expected values use actual signed C
arithmetic, not another copy of the magnitude/known-bit transfer.

The pipeline checks184 arithmetic raw/optimized captures per real Dis mode, plus
the Limbo example, retained zero-divide errors, fixed points, debug origins and
transactional rejection of unsafe SWF counts. Optional explicit `FIR_SWL` and
`FIR_FLASHVM_RUNNER` inputs execute23 actual linked int32 SWF movies. Big SWF
execution is not claimed. The complete standalone FIR suite also runs with strict
UBSan across all built FIR source/generated-parser/library/test objects; the
external Coco/R executable and native runtime/FlashVM runner are separate tools.

The owning Inferno contribution `libinterp/tests/division-powers.py/.c` separately
checks actual divw/divl/modw/modl against Python arbitrary integers. It is not
Dis64 whole-module execution or a native-JIT proof.

[Example walkthrough](../examples/dis/README.md#signed-division-example-fir072).
