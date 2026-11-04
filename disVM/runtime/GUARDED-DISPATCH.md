# Guarded dispatch completion / runtime0.109

Runtime0.108 selected ADDF/SUBF/MULF/DIVF C tails in the x64 emitter but omitted
four cases in `bytes_fast.c`. The dispatcher declined, so the owner loop used
its interpreter fallback. Guest results and coherent arithmetic guards still
worked; selecting/reaching a tail did not establish completed dispatch. The
previous tests observed arrival/cache/error behavior, then accepted fallback.
Runtime0.109 adds all four cases and corrects the prior documentation claim.

The real tail oracle now requires return1 and exactly one call to the genuine
arithmetic helper before returning. Cache-unmap and protected error paths remain
covered. For EACH opcode, private actual dispatcher variants (1) decline,
(2) return1 without execution, or (3) execute twice. All twelve must fail at the
exact completion predicate. Compiler/crash/sanitizer/resource failures never
count. Bad-CALL/old-emitter controls remain independent. Both collector polls
are checked. TSan positives instrument C, not emitted guest machine code.

`test_guarded_dispatch.py` separately audits all85 selected operations against
actual dispatcher cases and one named generated OP entry each. Its deliberately
bounded source recognizer rejects missing/repeated selectors, case drift,
misrouted functions, early success and absent OP entries. Three exact source
omissions/mutations for each entry plus one selector duplication give256 controls.
[Source-identity receipt](reports/guarded-dispatch-runtime109.json).
This is source-closure evidence, NOT a C parser, execution of85 operations,
proof of exclusion or permission for parallel managed mutators. Other owner
entry mechanisms such as MOVP are outside this particular selector's scope.

Source checkout, matching x64 build:

```sh
python3 disVM/runtime/test_guarded_dispatch.py build/runtime/native
python3 disVM/runtime/test_real_arithmetic_tail.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/benchmark_real_dispatch.py RUNTIME LIMBO INCLUDE_DIR
```

The runtime/editor SDK copies matching generated audit inputs to source/dispatch
when present. From that SDK directory:

```sh
python3 source/threading/test_guarded_dispatch.py source/dispatch
```

The diagnostic does not grant permission to compile/load those copies as another
VM. Runtime lifetimes, generated headers/object flags and executable ownership
remain separate. No shared IDs/opcodes/layout or native-module ABI is changed.

## Timing correction

REALARITH-108's original timing is preserved byte-for-byte. Its JIT side includes
declining tails plus fallback; it must not be called a completed-tail timing.
REALDISPATCH-109 compares private old/fixed dispatcher views against the SAME
matching runtime objects, startup guarded profile, guest and build flags. The
old side removes only the four cases; fixed executes through the tail. Both
preserve guest results. Warmup then three alternating samples include process
startup; all raw samples and dispatcher source hashes are retained. The C mode
is a control because it never uses this tail. Host scheduling variability and
other guarded operations remain in the measurement; FP/locks/C-tail costs are
not isolated. The comparison is not default-vs-guarded or general IDE speed.
See [PERFORMANCE-COSTS.md](PERFORMANCE-COSTS.md).

This correction supplies no root/graph epochs, shared constructors, channel/
scheduler lifetime or compiler locality proofs. NativeWorld remains required.
The remaining FP conversions are not changed in this series.
