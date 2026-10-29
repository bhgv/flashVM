# Fully known integer results — FIR 0.65

[Русский](KNOWN_RESULTS.ru.md) · [Optimizer](../optimiser/peephole.c) ·
[Bit analysis](../optimiser/integer_facts.c) · [Commented source](../examples/dis/known-results.b) ·
[Matrix](../tests/integer_known_results.py) · [Underlying bit facts](SHIFT_FACTS.md)

The peephole now replaces an int32 or big bit operation with a literal when
known-zero and known-one masks cover every bit of its result. Operands may be
dynamic. It handles AND, OR, XOR, big complement and checked shifts whose
successful result is fully determined by the admitted bit analysis.

| Dynamic expression | Exact result |
| --- | --- |
| `(x & MAX) >> (width-1)` | 0 |
| `(x \| MIN) >> (width-1)` | -1 |
| `(x \| 1) << (width-1)` | MIN |
| `(x & -2) << (width-1)` | 0 |
| `((x \| 1) << (width-1)) ^ -1` | MAX |
| `((x \| 1) << (width-1)) \| 123` | MIN+123 |

Here width is32 or64; right shift is arithmetic and left shift wraps. This is
not a new algebraic identity for floating-point or dynamic AS values. Unknown
bits do not become zero. Unknown/invalid checked counts yield no shift facts,
even when the left operand is zero, so their checks remain observable.

The shared literal helper now takes int64_t for this full-width call site.
Big MIN/MAX and MIN+123 never pass through host int; signed payload conversion
uses the existing unsigned-bit helpers. int32 materialization uses a proven
signed32 payload. Type, result identity, block and source line remain intact.

## Effects and iteration

Analysis runs after local cell forwarding and before bit materialization.
Equivalent rewrites preserve successful-result facts for retained SSA values;
CFG/DCE rebuild the analysis before their separate proof use. Folding clears
only the selected operator's operands, not its producers. A division by zero,
conversion error or other observable producer remains even if the final result
is a known constant. Public effects, memory barriers, conservative CSE, GC,
ownership and checked channel boundaries are unchanged.

CLI, fir_run and fir_object share this optimizer when optimization is requested.
Raw lowering retains the original operations; opt explicitly produces constants.
No grammar, FIR opcode, object/archive/SWF format or active NC64 ABI changes.
This does not enable new Dis64 execution or general big arithmetic in SWF.

## Source and target commands

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/dis/known-results.b build/known.fir
build/fir/fir1 opt build/known.fir build/known.opt.fir
build/fir/fir1 object build/known.opt.fir build/known.swo
build/linker/swl link build/known.swf build/known.swo
build/fir/fir1 dis build/known.opt.fir build/known.dis
build/runtime/disvm --jit --stats --inspect-words 0 2 build/known.dis
build/runtime/disvm --no-jit --stats --inspect-words 0 2 build/known.dis
```

The fully line-commented own Coco/R source visits eight loop values. A cleared
sign bit followed by shift31 gives zero; a forced sign gives minus one. Each
iteration adds one, then34 produces **42**. Raw and optimized Dis/SWF both work.
Installed C99 clients relink the same libraries; the Dis-owned IDE worker also
compiles these expressions through its ordinary channel-owned build service.
Source debug sidecars retain original spans, not new native-PC debugger support.

## Evidence

The dedicated matrix checks192 raw/optimized results from six expression shapes,
eight inputs and both widths. Inputs include MIN/MAX, both signs and dynamic
cross-block loads. The test requires exact literal spelling, absence of the
folded bit operators, optimizer fixed points, retained zero-divide producers,
and unknown/invalid count checks. Actual SWF requires explicit runner/linker
inputs; actual Dis execution requires the supplied runtime and checks dispatch
counters, capture flags and owner teardown. Compiler-only runs prove neither
player execution nor new-ABI VM acceptance.
