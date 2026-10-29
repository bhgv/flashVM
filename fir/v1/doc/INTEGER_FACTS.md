# Known int32 bits — FIR 0.63

FIR0.64: [big and checked-shift facts / big и проверяемые сдвиги](SHIFT_FACTS.md) extend this original int32-only contract.


[Русский](INTEGER_FACTS.ru.md) · [Analysis implementation](../optimiser/integer_facts.c) ·
[Public C99 API](../include/fir.h) · [Commented Limbo example](../examples/dis/integer-facts.b) ·
[Executable regression](../tests/integer_facts.py) · [C99 oracle](../tests/integer_facts_api.c)

FIR now proves some checked operations safe even when their operands are dynamic.
The same analysis serves SWO target admission and unused-operation elimination.
CLI, `fir_object`, `fir_run(FIR_OBJECT)` and the Dis-owned IDE compiler worker
use these libraries. No grammar, FIR opcode, extension flag or wire ABI changes.

| Expression | Proven successful result | Use |
| --- | --- | --- |
| `n & 31` | 0…31 | int32 checked left/right shift |
| `(n & 15) \| 16` | 16…31 | int32 checked shift |
| `(n & 31) ^ 31` | 0…31 | int32 checked shift |
| `n & 63` | 0…63 | big shift DCE, not an int32 shift proof |
| `d \| 1` | Nonzero; sign can vary | signed int32 div/mod |
| `d \| -2147483648` | Negative, nonzero | signed int32 div/mod |
| `(d & 7) ^ 8` | 8…15 | signed int32 div/mod |
| `d & 7` | May be zero | No nonzero proof |

## Analysis and effects

`FirIntegerFacts` stores known-zero and known-one bit masks per immutable int32
SSA value. `fir_integer_facts` clears caller-owned scratch and follows only exact
int32 literals, copies, AND, OR and XOR. Monotone bounded scans handle definitions
whose dominating block appears later in the file. Every other producer remains
unknown: no memory-cell contents, dynamic AS conversions, reference identity,
float values, path conditions or whole-program assumptions are inferred.

`fir_integer_nonzero` requires at least one known set bit. The count query
requires every bit outside 0…31 (or 0…63 for big) to be known zero, including the
sign bit. Copies of literal values are now admitted without requesting optimization.
An incomplete proof rejects SWO before writing any bytes. Existing CLI output and
C99 stream position/content are preserved on target rejection; final host I/O
failure remains outside this staging guarantee.

Facts concern **successful evaluation**. Their producers still execute in the
original order and may throw. DCE can discard an unused proven-total div/mod/shift,
but it checks each producer's own effects separately. For example, an unused
`x / (bad | 1)` is total only after evaluating `bad`; a divide-by-zero that computes
`bad` remains observable. Big division keeps the previous nonzero-big-literal
proof; this int32 analysis does not claim big known-bit arithmetic. Public
`fir_effects`, CSE, memory barriers and ownership contracts remain conservative.

Scratch is allocation-free, synchronous and owner-local. Input must be verified
IR and query indices must identify int32 SSA values. Recompute after changing IR;
never retain facts across optimization, mutation or another module. Analysis does
not mutate the module, invoke callbacks or wait for a Flash frame.

## Compile and run

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/dis/integer-facts.b build/facts.fir
build/fir/fir1 object build/facts.fir build/facts.swo
build/linker/swl link build/facts.swf build/facts.swo
build/fir/fir1 dis build/facts.fir build/facts.dis
build/runtime/disvm --jit --stats --inspect-words 0 2 build/facts.dis
build/runtime/disvm --no-jit --stats --inspect-words 0 2 build/facts.dis
```

The example visits i=0…3. `1 << (i & 31)` contributes 15; `i / (i | 1)`
contributes 2; adding 25 returns **42**. Both raw and optimized FIR work. The
expressions keep the bit proof in SSA: naming a mutable local and reloading it
can hide that proof until existing same-block load forwarding runs during opt.
The SDK ships this source, guide and matching public libraries.

AVM1 masks shift counts and uses binary64 Divide/Modulo. Proven legal counts make
masking identical to FIR checks; nonzero int32 division follows the existing
[exact rounding argument](SWF_INT_DIVMOD.md). SWL's ordinary existing action
profile is sufficient; no private player opcode is introduced.

## Evidence and limits

The matrix compares 896 raw/optimized trace results against independent integer
oracles and, when explicit tools are supplied, actual SWF and Dis interpreter/JIT.
It includes MIN/MAX, both signs, full-width deterministic inputs, copies, composed
masks, rejection and unchanged outputs. The C99 analysis test checks 110592 sampled
results with non-topological block order and unchanged module bytes. Separate
cases retain producer traps, distinguish width32/64, reject memory inference and
check optimizer fixed points. The own Coco/R source and IDE worker exercise the
same implementation, alongside relocated C99 compiler acceptance.

Unknown dynamic divisors/counts still reject SWO. This is a conservative bit
analysis, not general interval propagation or runtime exception lowering in SWF.
New Dis64 containers and full ABI execution are separate from these established
SWF8/NC64 targets.
