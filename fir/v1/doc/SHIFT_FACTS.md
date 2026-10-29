# Bit proofs across checked shifts and big — FIR 0.64

[Русский](SHIFT_FACTS.ru.md) · [Analysis](../optimiser/integer_facts.c) ·
[C99 API](../include/fir.h) · [Commented source](../examples/dis/shift-facts.b) ·
[Cross-target checks](../tests/integer_shift_facts.py) · [Original int32 contract](INTEGER_FACTS.md)

The shared known-bit analysis now covers immutable `big` as well as int32:
literals, copies, AND, OR, XOR and big complement. `FirIntegerFacts` retains
its int32 masks and adds `big_zero`/`big_one`; `fir_big_nonzero` queries a
known set bit. Scratch is caller-owned, allocation-free and synchronous.
Rebuild after changing live definitions/operands; facts concern successful
results and do not make operand producers effect-free.

For both widths, checked left/right shifts propagate bits only when their
int32 count is **exactly proven** and legal:0…31 or0…63. Counts can be literal,
copied or fully known through admitted bit operations. Left shift inserts
known zero low bits and discards high bits. Arithmetic right shift fills high
bits with a known sign bit; an unknown sign remains unknown. Zero counts copy
facts without a host shift by the word width. All compiler bit arithmetic is
unsigned; no signed host right shift, overflow or shift-by64 is used.

| Expression | Proof | Consumer |
| --- | --- | --- |
| `(n & 127) >> 2` | int32 range0…31 | Dynamic checked int32 SWF shift count |
| `(d \| 1) << 1` | Result bit1 is set, including negative results | Signed int32 SWF div/mod |
| `(big_value \| big(1)) << 63` | Exactly the sign bit is set | Nonzero big proof for DCE |
| `(big_value \| big(-2)) << 63` | Low source bit is unknown | No nonzero proof |
| big shift by64 or int32 shift by32 | Invalid count | No facts; preserve runtime error |

SWO preflight uses these facts without requiring opt. The existing ordinary
AVM1 instructions and SWL profile suffice. Big values remain a Dis target:
these facts do not add big SWF arithmetic. Unused big div/mod can now be removed
when the shared analysis proves their successful divisor nonzero. Checked
shifts retain their previous count-range requirements for DCE. Unknown count
values and potentially zero divisors remain conservative.

Every producer still evaluates in its original order. If `bad` throws, removing
an unused outer `x / (bad | 1)` does not remove `bad`. Reference cells, memory
loads, dynamic AS values, FP/NaN, conversions, safepoints and ownership are not
inferred or weakened. The public effects table and CSE remain unchanged. These
are local SSA bit facts, not general intervals, path predicates or new ABI rules.

## Source, CLI, C99 and IDE

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/dis/shift-facts.b build/shift-facts.fir
build/fir/fir1 object build/shift-facts.fir build/shift-facts.swo
build/linker/swl link build/shift-facts.swf build/shift-facts.swo
build/fir/fir1 dis build/shift-facts.fir build/shift-facts.dis
build/runtime/disvm --jit --stats --inspect-words 0 2 build/shift-facts.dis
build/runtime/disvm --no-jit --stats --inspect-words 0 2 build/shift-facts.dis
```

The fully commented own Coco/R source visits i=0…7. Its shifted mask yields
counts0,0,0,0,1,1,1,1. Four ones plus four twos sum to12; the quotients are zero;
adding30 returns **42**. Raw and optimized FIR both compile to established
SWF8 and NC64 paths. Public C99/CLI and the actual Dis-owned IDE compiler worker
use the same analysis; generated parser state remains serialized as before.
The installed SDK ships the example, guide and matching header/libraries.

## Evidence

The dedicated matrix checks272 raw/optimized trace results using integer oracles,
actual SWF when explicit runner/linker inputs are provided, and both Dis modes
when the native runtime is supplied. It covers MIN/MAX, shift31/63, discarded
bits, invalid counts, negative signs, big DCE and preserved producer traps.
The C99 API fixture totals120832 sampled checks, including unsigned 64-bit
shift/complement oracles and non-topological int32 CFGs. The previous int32
matrix and all compiler acceptance remain enabled. Tests for one execution
profile do not establish complete new Dis64 loader/calls/GC support.

Fully determined int32/big bit-operation results are materialized as exact
constants. Original operand producers retain their effects; big sign-bit
literals are preserved without narrowing through host int.

[FIR0.65: exact bit constants / точные битовые константы](KNOWN_RESULTS.md).
