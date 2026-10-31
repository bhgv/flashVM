# Exact power-of-two multiplication facts — FIR 0.70

[Русский](POWER_FACTS.ru.md) · [Analysis](../optimiser/integer_facts.c) ·
[Matrix](../tests/integer_power_facts.py) · [Commented example](../examples/dis/power-facts.b)

For int32 and big modular multiplication, a fully known nonzero single-bit
factor is an exact left shift. The analysis now preserves every known-zero and
known-one bit of the other operand after that shift, including upper bits.
Both operand positions work. Previous trailing-zero/parity facts remain the
fallback for other products. No interval guess or signed host shift is used.

| Dynamic expression | Proven fact or outcome |
| --- | --- |
| `((x | 5) * 8) & 32` | Always 32 |
| `(8 * (x | 5)) & 32` | Same proof with reversed operands |
| `(x & 7) * 4` | int32 count within 0…28; raw SWF checked-shift lowering can admit it |
| `(x & 7) * 8` | Cannot certify an int32 shift count; transactional rejection remains |
| `(x | 4) * 2` | Bit 3 remains set, so a successful result cannot be zero |
| `x * MIN` | Modular shift by width−1, including signed sign-bit literal |
| `x * -2` or `x * 3` | Not a single-bit factor; no shift substitution |

Here width is 32 or 64. A factor with unknown bits must not be treated as zero-filled.
The source/IR operations remain multiplication; these are successful-result facts,
not a new opcode or an unconditional rewrite to a checked shift. Factor 0 stays
on the existing zero-product path. Floating point and dynamic AS addition are
outside this integer analysis.

The implementation proves that all bits of one factor are known and exactly one
is set. Its shift count is strictly below width. Unsigned shifts/masks discard
high bits and fill low bits with zero; host signed overflow and shifting 64 never
occur. The sign-bit constant is accepted because FIR multiplication wraps modulo
the declared width. This does not justify treating arbitrary negative powers as
positive shifts or changing signed division rounding.

## Effects and consumers

The shared facts feed the peephole and raw checked backend admission. If a final
projection becomes fully known, the ordinary optimizer materializes its literal.
It does not delete eager division/conversion failures in producer operations.
Unknown/unsafe counts remain rejected before replacing an existing object file.
C99 public layouts, grammar, FIR1.0, SWO/Dis formats and shared Dis64 ABI stay intact.

The matrix covers all 32/64 single-bit factors, both operand orders and a dynamic
partially known input across a CFG boundary. It checks 396 raw/optimized captures,
fixed points, nonpower/negative/dynamic-factor counterexamples, actual divide
errors, safe raw SWF object admission and unchanged output on unsafe admission.
The same matrix executes through the real Dis interpreter/JIT with dispatch and
retained-owner counters. Changed analysis/API/CLI units also pass O3/NDEBUG strict
UBSan using prebuilt unchanged parsing/IR dependencies; this is not whole-library
sanitizer instrumentation.

## Build the example

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/dis/power-facts.b build/power.fir
build/fir/fir1 opt build/power.fir build/power.opt.fir
build/fir/fir1 dis build/power.opt.fir build/power.dis
build/runtime/disvm --jit --inspect-words 0 2 build/power.dis
build/fir/fir1 object build/power.opt.fir build/power.swo
build/linker/swl link build/power.swf build/power.swo
```

Use your actual explicit tool paths. Eight loop inputs, including both signs,
force product bit 5 and contribute 32 each; subtract 214 to return 42. Both raw and
optimized Dis execute. Source parsing/emission uses our Coco/R Limbo→FIR pipeline,
not the temporary Inferno bootstrap. Object creation/linking and SWF execution
are distinct checks; a successful link alone is not an execution claim.
