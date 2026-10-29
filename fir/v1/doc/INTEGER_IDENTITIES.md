# Total integer identities — FIR 0.59

[Русский](INTEGER_IDENTITIES.ru.md) · [Optimizer](../optimiser/peephole.c) ·
[Effects](../optimiser/effects.c) · [Tutorial](../examples/integer-identities/README.md) ·
[C99 API regression](../tests/integer_identities_api.c)

The peephole simplifies verified i32/big expressions when the replacement has
exactly the same value and cannot remove an operand producer's effects. No FIR
opcode, parser production, source syntax, Dis ABI or SWF action is added.
CLI `fir1 opt` and library `fir_optimise`/`fir_run` share the implementation.

| Rule | Exact integer result |
| --- | --- |
| x+0, 0+x, x-0, x*1, 1*x | x |
| x-x, x*0, 0*x | 0 |
| x/1 | x |
| x/(-1) | Wrapping multiply by -1; MIN remains MIN |
| x%1, x%(-1) | 0, including MIN%-1 |
| x AND -1, -1 AND x, x AND x | x |
| x AND 0, 0 AND x | 0 |
| x OR 0, 0 OR x, x OR x | x |
| x OR -1, -1 OR x | -1 |
| x XOR 0, 0 XOR x | x |
| x XOR x | 0 |
| x left/right shifted by proven int32 zero | x |

These are SSA-value identities, not source rewrites. For `(a/b)*0`, the multiply
may become zero while the division remains: its possible zero-divide error is
observable even when its result is unused. Dynamic big conversion, trace,
channel, ownership and memory producers retain their effect-table treatment.
No operand is reevaluated and no producer is moved across a branch.

Do not simplify x/x, x%x or 0/x with an unknown divisor. Do not simplify a zero
left operand of a checked shift with an unknown/invalid count. Their errors
remain. Checked operations lose their check only after proving divisor ±1 or
count zero. Static `fir_effects` for general divide/mod/shift still includes
MAY_THROW. The MIN/-1 rule uses modulo arithmetic rather than host signed UB.

No integer identity is applied to binary64, dynamic AS coercions or reference
values. In particular, replacing `f64.add(-0,+0)` with its first operand would
change +0 to -0. Existing constant folding and ordered floating CSE keep their
previous contracts.

## Local reuse and target admission

Ordered same-block CSE now includes i32 AND/OR/XOR. Identical operands in the
same order can share a calculation. Reversed operands are not canonicalized,
and the proof does not cross basic blocks. Checked arithmetic with unknown
operands is not added to the pure CSE set.

FIR 0.61 admits both raw and optimized SWO for x/1, x%1 and x<<0: their
literal inputs satisfy the [checked SWF arithmetic proof](SWF_INT_DIVMOD.md).
Optimization still removes the identity calculations. Unknown/invalid divisors
and counts retain transactional rejection; general dynamic checks and big SWF
operations remain outside the target profile.

The [commented example](../examples/integer-identities/main.b) returns 42 through
raw/optimized NC64 Dis and optimized FIR → SWO → SWL → SWF → FlashVM. Its loop
keeps i dynamic across block edges, so this is not merely literal folding.

## Ownership, debug and evidence

The pass operates synchronously on a borrowed verified FirModule, verifies again
on completion and retains the established fixed-point loop. Removed instructions
remain marked for ordinary compaction/printing. Source FIRDBG1 remains a mapping
to pre-optimization SSA; this feature does not create a final Dis/SWF PC map.
Callers rebuild matching headers/static libraries, then relink compiler services.
The deployed Dis IDE uses that same in-process compiler library.

[Matrix regression](../tests/integer_identities.py) compares raw/optimized Dis
captures, signed MIN cases, producer/unknown-divisor/shift traps, same/reversed/
cross-block bit CSE and binary64 signed zero. It checks a stable second optimize,
raw/optimized target admission and optional actual linked SWF execution.
The C99 regression repeats source/debug compilation, before/after snapshots,
proven raw admission and optimized object/Dis emission in one process. Runtime counters
verify actual interpreter/JIT dispatch; native owner counts must return to zero.
Linux execution and UBSan do not certify physical ARM64/RV64 or other platforms.

In-memory optimization retains original SSA identities. Printing/reparsing an
optimized FIR stream removes dead definitions and can therefore change backend
slot numbers and bytes while preserving behavior. The installed C99 tutorial
explicitly performs FIR_OPT → canonical text → backend, matching the CLI stages;
byte comparisons apply to that matched pipeline. Direct fir_run with optimise=1
is independently tested but is not promised byte-identical to a text roundtrip.

## Integer comparisons and branch pruning

For the same evaluated i32/big SSA value, `x == x` becomes true and `x < x`
becomes false. Signed `x < MIN` and `MAX < x` become false, using the exact
width of the operand. Reversed endpoint comparisons remain dynamic. The
ordinary CFG pass can then remove impossible branches. Throwing producers
remain observable even when their comparison is constant. No range inference
or floating reflexivity is introduced: `NaN == NaN` remains false at runtime.

[comparisons.b](../examples/integer-identities/comparisons.b) is a fully
commented source example returning 42. The regression checks raw and optimized
Dis in interpreter/JIT modes and, with the host/linker test configuration, raw
and optimized SWF execution. It covers MIN/MAX, negative and zero inputs,
producer exceptions and IEEE NaN separately. This is the existing NC64 backend,
not the proposed Dis64 format.

For this comparison example, the measured native run falls from 256 Dis
instructions (255 native) to 160 (159 native), preserving the result 42.
These are instruction counters, not a wall-clock speed claim.
