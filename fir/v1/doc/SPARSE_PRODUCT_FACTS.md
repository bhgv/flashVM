# Sparse constant multiplication facts — FIR 0.71

[Русский](SPARSE_PRODUCT_FACTS.ru.md) · [Transfer](../optimiser/integer_product_bits.h) ·
[Carry transfer](../optimiser/bit_arithmetic.h) · [Concrete oracle](../tests/integer_sparse_product_bits.c) ·
[Pipeline matrix](../tests/integer_sparse_facts.py) · [Commented Limbo source](../examples/dis/sparse-facts.b).

For 32/64-bit modular multiplication, a completely known factor containing one or
two set bits is the sum of one or two left-shifted copies of the other operand.
The analysis propagates known-zero/known-one masks through those shifts and one
modular carry transfer. The IR operation remains multiplication. No checked-shift
instruction, signed C overflow or floating-point rule is substituted.

| Expression | Useful fact |
| --- | --- |
| `(x & 7) * 3` | High bits above bit4 are zero: a raw int32 shift count is provably legal |
| `3 * (x & 7)` | Same fact for the other operand order |
| `(x & 15) * 3` | Cannot certify a count below32; raw SWF object admission still refuses |
| `((x & 7) * 9) & ~63` | Projection is zero |
| `x * (MIN + 1)` | Two-bit factor is handled modulo the declared width, including its sign bit |
| `x * 7` or unknown factor | Existing conservative divisibility/parity transfer remains |

Mathematically `(x&7)*3` lies in 0..21. This bit analysis proves the sufficient
upper bound 0..31; it does not introduce a general interval solver. Shifted copies
are correlated, but the carry transfer intentionally ignores correlations. It can
lose precision, never turn an unknown carry into a false proof. Work is bounded
by the 32/64-bit width; arbitrary factors are not expanded into unbounded sums.
Zero factors retain the old zero-product handling. Refused helper inputs preserve
both outputs. All arithmetic and masks use unsigned C99 types.

## Effects, target admission and debug origins

The facts describe successful results. A constant final projection does not allow
an earlier divide-by-zero or conversion error to disappear. Existing effect-aware
DCE retains such producers. The verifier/peephole/backend continue to own validity
and debug origins; this analysis neither executes user code nor mutates IR.
The raw SWF backend consumes the same facts before an optimization pass. Unsafe
admission preserves the previous output file. A dynamic count is not specialized
merely because one test seed happens to produce a valid count.

Public C99 layouts, Coco/R grammars, FIR1.0, SWO and Dis formats stay unchanged.
The new headers are private optimizer helpers, not a new public API or Dis64 ABI.

## Validation

The concrete oracle enumerates 3,338,240 unsigned modular products, every one/two-bit
factor position in both widths and several complete low-byte input sets, with
forced high/sign bits. It checks each emitted fact, disjointness, precision for
new checked-count admission and unchanged output on refusal. This is independent
concrete multiplication, not a second shift/add implementation.

The pipeline matrix checks 104 raw/optimized captures, both widths and operand orders,
fixed points, retained eager errors and transactional SWF count refusal. With actual
DisVM inputs it checks interpreter/JIT dispatch counters and retained-owner cleanup.
With explicit `FIR_SWL` and `FIR_FLASHVM_RUNNER`, 39 linked SWF movies execute in
FlashVM, including the admitted shift and the Limbo example before/after optimization.
Big SWF execution is not claimed. The partial strict-UBSan run instruments the changed
analysis/CLI and concrete oracle; unchanged parser/IR libraries are prebuilt.

[The example walkthrough](../examples/dis/README.md#sparse-factor-example-fir071)
contains source, compile, optimize and execution commands.
