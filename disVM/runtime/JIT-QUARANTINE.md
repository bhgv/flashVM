# Function quarantine after a confirmed JIT defect

The unit of fallback is a **whole function**, not an opcode site or entire VM.
Subsequent dispatch in that function uses the ordinary interpreter. Other
functions and modules keep JIT; already admitted native instructions are not
asynchronously interrupted or replayed. There is no global exclusive phase.

The hosted leaf backend keeps original Dis instructions/PCs. Function intervals
are derived from module entry, export entries and patched local CALL/SPAWN
entries. This uses the loaded module's immutable code, without installed symbol
files, disassemblers or debugger libraries. Source functions inlined by the
compiler have no independent runtime frame; their enclosing emitted function is
the fallback unit. Ranges are original Dis PC intervals `[start,end)`.

A monotonic generation assigned before JIT publication distinguishes unload and
reload, even if allocation addresses are reused. Quarantine records contain only
native scalars and are release-published as an immutable prefix. Parallel readers
need no execution lock. Exceptional writers use a metadata mutex. A currently
running instruction may finish after another thread reports a defect; its next
checked JIT selection observes the published quarantine.

At present the automatic classifier handles an explicit `exCompile` raised from
a selected native callback. Ordinary Limbo nil/bounds/divide errors, declined
fast paths and all ordinary nonzero callback return values keep their existing
semantics and do not quarantine code. The internal `dvm_jit_report_failure`
interface can record a separately confirmed backend defect before unwinding.
It is not an arbitrary client command or an instruction-retry API.

One bounded `JITFAULT` record reports loaded-code generation, module path,
function interval, exact original Dis PC/opcode, native backend, cause,
`action=interpret-function` and `replay=0`. The failing operation follows ordinary
exception handling, or debug retention before that handling. It is not restarted
from the function entry: previous I/O/shared writes could already have happened.

The table holds128 exceptional function records for this VM lifetime. Reaching
the bound disables subsequent JIT selection globally in favor of interpretation
and logs `capacity_exhausted=1`; it never silently executes a known bad function.
Generation exhaustion likewise declines new JIT execution. Records do not hold
Module/Jit/heap references. Restart resets quarantine; there is no disk cache or
persistence policy yet. Missing function symbols are not fabricated.

The real-VM regression injects a side-effect-free explicit backend exception
inside a real selected native addition. It verifies a caught original exception,
correct later results, **no subsequent native entry anywhere in the whole failed
function interval**, continued genuine native work outside it and both poll
backends. Injection is confined to a test-linked probe, absent from production.

Synchronous admitted integer scalar RX faults now feed this same whole-function
quarantine: see [signal capture](JIT-SIGNALS.md). This remains a bounded recovery
contract, not an arbitrary native-corruption detector or instruction retry policy.
External GDB is optional; see [debugger](DEBUGGER.md).

The missing-classification control preserves original caught-error behavior and
correct results, but requires precise DETECTED for absent quarantine. Eight real
native owners exercise the shared fallback after publication. / Контроль пропуска
классификации сохраняет пойманную ошибку и верные результаты, но требует точного
DETECTED при отсутствии карантина. Общий fallback проверяют восемь native owners.
