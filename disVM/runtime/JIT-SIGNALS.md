# Built-in RX scalar fault capture

The native runtime installs its own Linux64 SIGILL/SIGSEGV/SIGBUS/SIGFPE handlers
before starting VM threads. GDB, Python, a debugger library and an external
supervisor are not runtime requirements. `--gdb` remains an optional launcher.
The normal execution model is still one permanent native owner per Prog,
exclusive=0, with no global mutator stop.

## Admission and state

Capture/recovery currently covers the selected **integer scalar RX fast leaves**
listed in `jit_fault/invoke.c`. It is not general recovery from arbitrary native
memory corruption. Function calls, stack/module transitions, allocation, managed
pointer/refcount operations, floating-point/SIMD and unsafe C/helper execution
are outside this admitted subset. Other operations retain their existing native
or interpreted execution; the handler never pretends they have a safe landing.

Before invocation the runtime checks original opcode, callback address inside
its admitted immutable RX extent, ACTIVE managed admission, zero object guards
and a valid owner-local native cleanup checkpoint. A different opcode or foreign
callback cannot borrow the RX landing. Mapping provenance is native scalar data,
not a managed root. A lazy owner-owned alternate signal stack is at least64KiB
and at least SIGSTKSZ; an existing enabled stack may be borrowed. Owned storage
is released on source retirement or worker teardown, including error shutdown.
Failure to prepare a stack declines the native invocation before effects and
uses the ordinary interpreter.

The handler checks positive kernel si_code and the **actual kernel PC** inside
the admitted RX mapping. It writes only lock-free scalar TLS snapshot fields,
publishes completion and transfers to the same OS thread's live landing.
It does not allocate, lock, log, inspect managed memory, update quarantine,
change GC admission or invoke guest/error handlers. The signal mask is saved and
restored by the native landing boundary. There is no cross-thread continuation.

After landing, ordinary source code rechecks ACTIVE admission, no object guards
and a clean native checkpoint. A changed admission/cleanup state refuses recovery
and aborts; it is not unwound through possibly discarded helper frames. A fault
outside RX, a non-admitted operation or a signal sent by raise/kill instead of a
synchronous RX fault follows the original OS disposition. Dispositions are restored
after all source threads stop; initialization is idempotent.

## Delivery, retention and reports

For an admitted fault the source emits `JITSIGNAL` with signal/code/fault address,
native PC/SP, RX offset and replay=0, plus a bounded `JITCPU` line containing
named integer CPU register values and source PID. The existing `JITFAULT` report
identifies the original module/function interval/Dis PC/opcode/backend and
quarantines the **whole function** for subsequent dispatch.

Then the operation raises `sys: JIT native signal` through the ordinary VM error
boundary. The current instruction is never returned as a declined fast path,
replayed, restored from a REG copy or restarted at function entry. Already
completed scalar writes remain completed. In normal mode a Limbo handler can
catch the new error; unhandled-error behavior stays ordinary. Other guests keep
running. In debug mode the existing first-chance stop holds the original managed
stack, roots and exception before handler/unwind; GC and other owners continue.

The original CPU snapshot is a diagnostic copy, not the resumed live CPU state.
`native-registers PID` returns that copied signal context, architecture, actual
native PC/SP, RX offset and a named integer-register object. `registers PID`
continues to mean VM REG. `backtrace`, `operation`, typed reads and `census` use
the retained original Dis continuation. Addresses in either register view do not
authorize arbitrary memory access. Continue delivers the original error handler;
step does not replay faulting instructions. The next admitted debug instruction
clears historical native-fault publication. Nonlocal VM error paths disarm stale
RX landing frames before a later operation can use them.

CPU projection currently includes x64 Linux gregset (23 entries), ARM64 x0..x30
and pstate, or RV64 Linux gregset (32 entries including PC). SP/PC are explicit.
Floating-point/vector register state and arbitrary native backtrace/frame retention
are not implemented by this projection. Dis backtrace is retained and tested.

## Validation and limits

The test-only probe publishes separate immutable RX trap pages; production module
bytes/mappings are never patched. The public guest remains a valid application
without injection. Actual SIGILL, null-read SIGSEGV, truncated-backing SIGBUS,
a scalar write followed by SIGILL, and sequential SIGILL/SIGSEGV in two different
functions of the **same owner** test capture and signal-mask reuse. Both default
and debug modes and both poll paths run. Kernel argument-register values are
checked against the actual native invocation. Original caller payload and GC-root
census are checked before release; completed Fast work is observed while that
source is still held. Correct later results, quarantine, normal catch and exact
source/collector retirement are required.

A missing CPU-copy omission requires precise DETECTED while application semantics
and retirement stay normal. Separate unsafe-path tests require fatal SIGILL for
a C helper and a real held object guard, and SIGABRT for an actual unclosed native
cleanup checkpoint. These are refusal tests, not application recovery evidence.
Core dumps are disabled only in this test driver and its children.

Local AMD64 and ThreadSanitizer runs do not prove new ARM64/RV64 execution; those
use the mandatory architecture workload `jit-signal` (list now63). Generated RX
bytes are not instrumented by C sanitizers. Native memory corrupted before the
fault, loss of control outside RX, arbitrary host faults and persistent quarantine
across process restart remain outside the recovery guarantee.

POSIX constraints informing the boundary: Linux man-pages [sigsetjmp](https://man7.org/linux/man-pages/man3/sigsetjmp.3.html)
and [signal-safety](https://man7.org/linux/man-pages/man7/signal-safety.7.html).
The restriction to a synchronous admitted RX PC avoids treating an interrupted
unsafe libc/helper call as a recoverable leaf.

Post-landing checks also compare original VM control/root/operand addresses.
A native test that corrupts REG.M before trapping must abort before managed
publication/handler; no saved REG is restored. / После landing сравниваются
исходные VM control/root/operand addresses. Реальная порча REG.M перед trap
должна дать abort до managed publication/handler; REG из снимка не восстанавливается.
