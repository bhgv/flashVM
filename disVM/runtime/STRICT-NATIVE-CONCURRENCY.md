# Native execution with collector exclusion forbidden

Copyright (c) 2026 bhgv

## Policy

`--native-spawn` now selects a closed concurrent collector route. Fast and Cycles
receive `concurrent_fast`/`concurrent_cycles`; their retained callbacks and stopped
root visitor are unset. Duke continues using its shared graph discovery. There is
no automatic fallback to a collector-owned world, including during shutdown.
The single-worker reference retains its separate tracing implementation.

The native world is created with `dvm_world_create_concurrent`. This constructor
forbids both stop requests and acquisition of an already quiescent heap. A call to
either stop API prints `NATIVE_CONCURRENCY_ERROR` and terminates with status 70
before setting a collector, stop request or parked generation. A quiescent stop
would still block new guest admission, so it is forbidden too.

All runtime calls to `rungc` pass through a guard. The retained implementation is
compiled as `dvm_retained_rungc`; native execution cannot enter it. Direct retained
RC draining is guarded as well. Fatal concurrency diagnostics flush stderr and use
`_Exit(70)`, avoiding unsafe cleanup handlers while other OS threads remain live.

Native diagnostics report:

```
NATIVEGC_POLICY exclusive=0 trace=0 stop_forbidden=1
NATIVEGC collector=worker passes=0 snapshots=0 ...
```

The historical `serial_heap` field describes the hosted participant protocol; it
is not a mutator execution mutex. The host regression keeps two ordinary ACTIVE
owners admitted simultaneously and rejects both collector stop APIs.

## Shared shutdown and event requests

The last controller temporarily leaves execution while all collector lanes drain
**before** RC admission closes. Otherwise Cycles/Duke could retain accepted work
which a closed queue prevents them from transferring to Fast. The same controller
context and live registers reenter; no `Prog.R` image is restored. Guest joins,
sealing, close and service joins still preserve the published-root lifetime.

Native event requests inspect candidate metadata and coalesce empty work without
waiting for retained color rotations. The eight-owner request fixture validates
32000 real requests per VM, exact atomic request accounting, rooted draining and
zero retained passes/snapshots with both C/JIT and polling modes.

## Unsupported work is an error, not a leak or a hidden stop

Supported managed types continue shared reclamation. Lists remain eligible when
native Cycles is disabled; disabling cycle discovery does not disable ordinary
acyclic list finalization.

A last owner requiring an unreviewed finalizer terminates with
`component=Fast reason=unsupported-finalizer`. The object is still rooted and
owned when refusal is detected. Nonfinal references may be dropped without
executing a finalizer. Difficult finalization, finally refused cycle graphs and
exceptional Duke pin fallback produce their own explicit RC diagnostic. They are
not silently abandoned and cannot invoke the old stopped collector.

This intentionally exposes incomplete support. It does not turn arbitrary C
callbacks into thread-safe finalizers and does not remove Duke's graph-storage
limit. VM errors unrelated to this policy retain their normal handling.

## Validation and visible audit failures

Linux x64 normal build, selected native audit: **25 passed, 4 failed out of 29**.
The four failures remain visible; they were not disabled or marked expected:

| Test | Observed strict failure | Remaining work |
| --- | --- | --- |
| `disvm_packet_fairness` | Unknown finalizer in its deliberately retained packet | Rebase scheduling probes on supported transient pressure; unreviewed callback execution remains forbidden. |
| `disvm_fast_claim_storage` | Mixed-backlog profiles publish an unknown callback | Separate shared claim/replay probes from legacy callback scaffolding. |
| `disvm_duke_queue_progress` | `refused-cycle-graph` after its 70000-field graph exceeds the snapshot limit | Replace the stopped hard-limit route with a bounded concurrent representation. The independent 2300-field graph freed before the explicit refusal. |
| `disvm_module_retirement` | Deliberate omission control 4 substitutes an unknown finalizer | Update that negative control to require strict refusal; positive module cases completed before it. |

This is an audit of the strict runtime, not an all-green suite or a claim that all
native heap shapes are supported. The exact native audit command is:

```sh
ctest --test-dir build/disvm -j4 --output-on-failure -R 'disvm_(concurrent_guards|resident_.*|gc_requests|module_cycle_retirement|module_retirement|packet_cycle_retirement|cycle_owner_rescue|exception_retirement|managed_retirement|duke_pin_retirement|duke_queue_progress|packet_fairness|large_owner_release|fast_.*)$'
```

Additional evidence:

- All 21 host CTests passed, including genuine ACTIVE overlap and both stop guards.
- `disvm_concurrent_guards` checks 20 exact status-70 failures: unknown callback,
  direct trace, direct retained drain, ordinary stop and quiescent stop, each with
  C/JIT and both polling modes. Signals/timeouts do not count as success.
- The strict large-owner matrix passed 152 positive VM profiles and its applicable
  ownership/claim/epoch/pressure omission controls. `--concurrent` derives a private
  fixture without the old unknown-callback scaffolding. It preserves real target
  construction, child multiplicity, roots/counts, pool frees, claim races and
  allocation/FULL/BYTES probes. Case 16 and controls 106/110 specifically require
  retained assistance/backlog and are excluded from this concurrent matrix;
  explicit guard tests cover refusal instead.
- TSan passed all 152 strict large-owner positive profiles, plus the startup,
  published-root and request-burst suites. Fatal negative probes are tested in
  the normal build; emitted JIT code is not C-TSan instrumented.
- After final guard/comment changes, host tests and native guard/request/root/file
  checks were revalidated. Handwritten changed sources stay below 500 lines;
  upstream snapshots remain unchanged.

ARM64/RV64 hardware validation and complete JIT coverage are not established by
this Linux x64 series. Absence of a TSan report is not proof of race freedom.

## Русский

В `--native-spawn` эксклюзив сборщика отключён. Fast/Cycles работают только через
конкурентные обработчики. Домен запрещает и общую остановку, и захват уже спокойной
кучи. Прямой старый `rungc` и остановочный RC-drain также защищены. Попытка даёт
диагностику и код 70 до выполнения старого пути; возврата к нему при ошибке нет.

При завершении очереди обрабатываются до закрытия, без замены постоянного Prog
или живых регистров. Неизвестный финализатор и граф, требующий прежнего fallback,
теперь являются явными ошибками, а не скрытой остановкой либо утечкой.

Аудит: 25 из 29 тестов прошли; четыре оставлены видимыми. Они обнаружили старую
обвязку неизвестных финализаторов и предел графа Duke на 70000 полей. Независимый
граф на 2300 полей успел освободиться. Это конкретные оставшиеся ограничения;
полная поддержка всех объектов и произвольных native callbacks не заявляется.

152 профиля поддержанных широких владельцев прошли обычно и под TSan. Также
прошли 21 host-тест и 20 проверок запрета старых входов. Следующая работа должна
исправлять выявленные случаи при включённом запрете эксклюзива, а не возвращать
резервную остановку ради зелёного результата.

The single-worker reference additionally passed the real `gc-worker.dis` workload
in C and JIT, including 12000 unreachable cycles and preserved live roots, with
`exclusive=1 trace=1 stop_forbidden=0`. This is a separately selected reference,
not a fallback available to native execution. Largest changed source: 450 lines.
