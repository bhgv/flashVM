# Fully known integer results — FIR 0.68

[Русский](KNOWN_RESULTS.ru.md) · [Optimizer](../optimiser/peephole.c) ·
[Bit analysis](../optimiser/integer_facts.c) · [Commented source](../examples/dis/known-results.b) ·
[Matrix](../tests/integer_known_results.py) · [Underlying bit facts](SHIFT_FACTS.md)

The peephole now replaces an int32 or big bit operation with a literal when
known-zero and known-one masks cover every bit of its result. Operands may be
dynamic. It handles modular addition/subtraction, AND, OR, XOR, big complement and checked shifts whose
successful result is fully determined by the admitted bit analysis.

| Dynamic expression | Exact result |
| --- | --- |
| `(x & MAX) >> (width-1)` | 0 |
| `(x \| MIN) >> (width-1)` | -1 |
| `(x \| 1) << (width-1)` | MIN |
| `(x & -2) << (width-1)` | 0 |
| `((x \| 1) << (width-1)) ^ -1` | MAX |
| `((x \| 1) << (width-1)) \| 123` | MIN+123 |
| `((x & -8) + 5) & 7` | 5 |
| `((x \| 7) + 1) & 7` | 0 |
| `((x & -8) - 1) & 7` | 7 |
| `((x \| 7) - 5) & 7` | 2 |
| `((x & -8) + (y & -8)) & 7` | 0 |

Here width is32 or64; right shift is arithmetic and left shift wraps. This is
not a new algebraic identity for floating-point or dynamic AS values. Unknown
bits do not become zero. Unknown/invalid checked counts yield no shift facts,
even when the left operand is zero, so their checks remain observable.

The shared literal helper now takes int64_t for this full-width call site.
Big MIN/MAX and MIN+123 never pass through host int; signed payload conversion
uses the existing unsigned-bit helpers. int32 materialization uses a proven
signed32 payload. Type, result identity, block and source line remain intact.

## Carry and borrow facts

FIR0.66 walks32 or64 bit positions using only unsigned masks. Addition starts
with carry zero. Subtraction uses `a + ~b + 1`, starting with carry one and
swapping the known-zero/known-one masks of b. A sum bit is known only when all
three input bits are known. The next carry is known zero if at least two inputs
are known zero, and known one if at least two are known one. Otherwise it stays
unknown. The carry after the last bit is discarded, matching modular FIR.

This is a conservative independent-bit analysis. It does not assume that two
unknown inputs are equal, or retain correlations between a carry and other bits.
For example `x-x` is not established by this analysis; existing separately checked
identity rewrites handle such relations. No interval, signed host overflow or
signed host shift is used. Analysis scratch, public C99 layouts and ownership
remain unchanged. Work per arithmetic instruction is bounded by the word width
within the existing monotone graph scans.

These facts also feed existing consumers without requiring opt: a raw SWF shift
by `(x & 15) + 16` is proven within0…31, while `(x & 31) + 32` is rejected as
an int32 count. `(x & -8) + 1` has a set low bit and therefore certifies a nonzero
signed divisor, including wraparound and negative values. `(x & 7) - 1` can be
zero, so no such proof is allowed. Big values retain the separate Dis backend;
this work does not introduce general big operations in SWF.

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
sign bit followed by shift31 gives zero; a forced sign gives minus one. Carry
and borrow terms add zero and seven, then subtract seven. Each
iteration adds one, then34 produces **42**. Raw and optimized Dis/SWF both work.
Installed C99 clients relink the same libraries; the Dis-owned IDE worker also
compiles these expressions through its ordinary channel-owned build service.
Source debug sidecars retain original spans, not new native-PC debugger support.

## Evidence

The dedicated matrix checks448 raw/optimized results from fourteen expression shapes,
eight inputs and both widths. Inputs include MIN/MAX, both signs and dynamic
cross-block loads. The test requires exact literal spelling, absence of the
folded bit operators, optimizer fixed points, retained zero-divide producers,
and unknown/invalid count checks. Actual SWF requires explicit runner/linker
inputs; actual Dis execution requires the supplied runtime and checks dispatch
counters, capture flags and owner teardown. Compiler-only runs prove neither
player execution nor new-ABI VM acceptance.

The C99 analysis fixture checks204800 mask/shift samples and192512 additional
carry/borrow samples across every low-bit boundary in32/64 bits. Facts must be
disjoint, retain proven low zeros, and hold for independent unsigned evaluations.
The SWF proof matrix also exercises arithmetic-derived counts and divisors,
rejected unsafe counts/possibly-zero divisors and retained inner zero division.

The independent exhaustive oracle additionally enumerates all81×81 pairs of
four-bit ternary domains and256 concrete input pairs for each, in both widths
and both operations: **6,718,464 evaluations**. It compares exact zero/one
intersections with analysis, including subtraction sign extension and carry
beyond the four-bit domain. The oracle does not reproduce the carry algorithm.

FIR0.68 also propagates big negation as unsigned modular zero-minus-value.
Known low bits/nonzero proofs survive negation, including MIN wraparound;
operand errors remain observable. [Commented own-source Dis example](../examples/dis/big-neg-facts.b)
uses a typed table fetch, forces an odd value, negates it and returns42.
The C99 fixture additionally exercises10240 negation samples; the target matrix
at0.67 covered384 raw/optimized results and tests the source example through Dis.

## Multiplication trailing-zero facts / Младшие нули произведения

For int32/big modular multiplication, if successful operands have at least a
and b trailing zero bits, the product has at least min(width,a+b). The analysis
stops at the first unknown bit and makes no claim about the remaining bits.
It saturates before a full-width shift; all payload arithmetic remains unsigned.
Thus `((x & -8) * (y & -8)) & 63` becomes0, and
`(x & MIN) * (y & -2)` becomes0 even across modular overflow. This does not
justify a floating-point or dynamic AS rewrite. Both operand producers retain
checked reads, conversions and eager exceptions.

[product-facts.b](../examples/dis/product-facts.b) is commented line by line.
Compile it with the same source/opt/dis commands above; its raw and optimized
big-capable Dis result is42. The installed C99 source-library client also emits
identical bytes to the CLI. No parser, SWF big support or Dis64 wire change is
introduced. The existing int32 target matrix also checks linked SWF execution.

The C API oracle covers every pair of low-zero counts0..32 and0..64,
including exact zero and saturation:1,360,384 unsigned product samples. It
requires useful low-zero masks, unknown remaining bits, immutable input IR and
propagation through a later textual dominating block. The raw/optimized target
matrix has448 executions and retains division-by-zero producers even when the
outer product is entirely zero.
