# Checked int32 shifts in SWF — FIR 0.60

FIR0.63: [known-bit proofs / доказательства по битам](INTEGER_FACTS.md) additionally admit bounded dynamic counts/nonzero divisors and literal copies. The literal-only boundary below describes the original release.

[Русский](SWF_INT_SHIFTS.ru.md) · [General shift contract](INT_SHIFTS.md) ·
[Commented example](../examples/dis/int-shifts-swf.b) ·
[C99 API acceptance](../tests/swf_int_shifts_api.c)

The SWO1 backend accepts `i32.shl.checked` and `i32.shr.checked` when the count's
live SSA definition is `const.i32` in **0…31**. The value may remain dynamic.
CLI `fir1 object` and library `fir_object`/`fir_run(FIR_OBJECT)` share the proof.
No parser production, FIR opcode, SWO version or Dis ABI changes.

AVM1 shifts mask the count to five bits. FIR instead rejects negative counts and
counts above 31. Consequently the backend must reject unknown counts, even if
some executions would use a valid value. It also rejects invalid constants;
it does not silently mask them or turn a FIR exception into a SWF result.
The entire preflight happens before writing any object bytes. A rejected call
preserves the caller's previous output prefix; successful output still requires
normal I/O error handling.

| FIR | SWF admission |
| --- | --- |
| dynamic x shifted by literal 0…31 | Accepted, exact signed32 result |
| literal x shifted by an unknown count | Rejected, including x=0 |
| count -1, 32 or another out-of-range literal | Rejected |
| count defined by copy/add/another operation | Raw backend rejects; explicit optimization may expose a valid literal |
| big shifts | Existing target restrictions remain |

Left shift retains the low 32 bits. Right shift is arithmetic, extending the
sign bit. Count 31 and negative inputs use the same semantics as the Dis backend.
The emitter uses standard AVM1 ShiftLeft/ShiftRight after checking the count;
it reevaluates no source expression and introduces no hidden runtime dependency.

## Build the example

From the repository root, with the FIR tools and SWL linker already built:

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/dis/int-shifts-swf.b build/shifts.fir
build/fir/fir1 object build/shifts.fir build/shifts.swo
build/linker/swl link build/shifts.swf build/shifts.swo
build/fir/fir1 dis build/shifts.fir build/shifts.dis
build/runtime/disvm --jit --stats --inspect-words 0 2 build/shifts.dis
build/runtime/disvm --no-jit --stats --inspect-words 0 2 build/shifts.dis
```

The source loop adds -2, 2, 6 and 10, then adds 26 to return **42**. Dis inspection
reports payload 42 and executed flag 1. The linked SWF traces 42 in FlashVM.
Every source line is explained. For the optimized path, run `fir1 opt` first,
then feed its output to either backend. Installed SDKs include the same source
and libraries; a fresh C99 client can compile it without the build tree.

## Verification and limits

The regression uses eight signed inputs including MIN/MAX and every count
0…31, for left and right shifts: 512 expected values per raw/optimized variant.
It compares Python integer oracles against actual FlashVM SWF output and native
Dis interpreter/JIT captures, with dispatch counters. Invalid/dynamic counts
check diagnostic and existing-output preservation. Public C99 API tests repeat
success, rejection and optimizer-exposed-count cases to check parser reuse.

SWF execution is enabled by explicit `FIR_SWL` and `FIR_FLASHVM_RUNNER` inputs.
Compiler-only runs do not prove player execution. This does not add general
runtime checked-count exceptions to AVM1, dynamic ActionScript shift syntax,
AS3/ABC output or the proposed int32/ref64 Dis64 ABI.
