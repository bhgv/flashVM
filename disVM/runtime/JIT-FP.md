# FP faults and base register diagnostics

The built-in RX guard now admits basic REAL scalar operations: MOVF, ADDF, SUBF,
MULF, DIVF and NEGF, alongside the existing integer subset. Root/control identity,
ACTIVE admission, no-object-guard and native-cleanup checks remain mandatory.
There is no global stop, exclusive profile or external debugger dependency.

Before an admitted invocation, the source saves its native floating environment.
If saving fails, native execution declines before effects. After a synchronous RX
fault and successful safety checks, the source restores that pre-invocation
environment **before** logging/VM error delivery. Restore failure refuses recovery.
Normal native returns keep existing FP semantics; this does not reset every
successful operation's flags. Existing managed-error cleanup contracts remain.
The failing operation is never replayed, and its whole emitted function enters
the existing interpreter quarantine. VM REG snapshots are not restored.

## Copied base state

Kernel-context capture performs integer reads and lock-free stores only. No FP
arithmetic, allocation, locking, libc formatting or fenv call occurs in the signal
handler. It copies:

| Target | Copied base fields |
| --- | --- |
| x64 | 16 XMM128 registers; eight x87 significant80-bit slots; x87 control/status/compressed tags; MXCSR |
| ARM64 | 32 V128 registers, FPCR and FPSR from the bounded FPSIMD record |
| RV64 D | 32 F64 register bit patterns and FCSR |

`native-fp PID` returns copied fields as exact hexadecimal integers, including
architecture, format, width and `coverage:"base"`. It has its own bounded response,
separate from `native-registers` and VM `registers`. x87 slot validity/TOP must be
interpreted with tag/status; empty slots are not claimed live values. No floating
conversion of NaN, extended precision or payload bits is performed.

`JITFP pid=...` logs the same named bit patterns outside the handler, correlated
with JITSIGNAL/JITCPU/JITFAULT. These describe fault-time kernel state; the live
source has already restored its pre-invocation FP policy. They are diagnostics,
not a CPU continuation for resumption or arbitrary native-memory read capability.
The next debug instruction clears historical publication as before.

ARM64 decoding walks at most the4096-byte reserved region, checks16-byte record
geometry and complete528-byte FPSIMD payload, skips unknown bounded records and
never follows extra-context pointers. Absent/malformed base state is explicit.
RV64 decoding is scoped to the current D target ABI. Extended XSAVE/AVX upper
lanes, SVE/SME, RVV and Q-extension state are not captured by this base view.
SIMD guest operations, arbitrary host/helper faults, native corruption and native
frame retention remain outside the admitted scalar-recovery contract.

## Evidence

The public REAL guest executes real floating additions and checks exact results
and rooted caller data. Test-only RX pages load independent low/high canaries,
change the hardware rounding policy/flags, then fault. On x64 an additional real
SSE divide by zero generates SIGFPE. Other targets run illegal-instruction FP
capture without assuming supported hardware FP trap enables.

Default/debug and fast/locked-poll runs require original Dis frames/payload/GC
census, actual Fast progress while retained, correct subsequent interpretation,
genuine peer native work and complete retirement. The kernel argument GPR and
original REAL opcode remain independently checked. Copied base bits include the
last vector register, and x64 x87 ST0=1.0 is checked as its80-bit encoding.

Pre-handler source evidence requires the original downward rounding mode and
inexact flag, despite the different policy installed by RX code. Two safe
omissions independently detect missing copied FP publication and omitted fenv
restoration while normal application catch/results/retirement still succeed.
The handler's copied flags retain fault-time values rather than the restored ones.

A separate record unit checks560 truncations, invalid lengths, unknown-record
skipping, invalid external-pointer non-following, final-vector bits and response
bounds. This validates decoding, not actual ARM64 signal delivery on another CPU.
Its host ASan/UBSan run uses detect_leaks=0 because this sandbox's ptrace prevents
LeakSanitizer; memory/UB instrumentation remains enabled. No allocation-leak
coverage is inferred. Generated RX bytes are not C-sanitizer-instrumented.

Prior architecture CI43622919835 passed all six jobs. The new workload `jit-fp`
(list now64) requires its own AMD64/ARM64/RV64 matrix; local AMD64 results alone do
not establish new cross-architecture execution.

ABI references: [glibc x64](https://github.com/bminor/glibc/blob/master/sysdeps/unix/sysv/linux/x86/sys/ucontext.h),
[Linux ARM64 signal context](https://github.com/torvalds/linux/blob/v6.12/arch/arm64/include/uapi/asm/sigcontext.h),
[glibc RV64](https://github.com/bminor/glibc/blob/master/sysdeps/unix/sysv/linux/riscv/sys/ucontext.h).
No upstream/vendor source was modified or imported for these projections.

The test probe resets its own policy to defaults only AFTER observing the
pre-handler result, including omission runs; this keeps the negative control
safe and is not production repair. / Test probe возвращает default policy
только ПОСЛЕ наблюдения pre-handler результата, включая omission runs.
Это граница безопасного контроля, не production repair.
