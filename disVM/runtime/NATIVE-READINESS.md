# Native concurrency acceptance / Приёмка native-многопоточности

Copyright (c) 2026 bhgv

Actual three-target C/JIT results: [Linux64 architecture acceptance](NATIVE-ARCH-ACCEPTANCE.md).
Фактические C/JIT-прогоны трёх целей: [архитектурная приёмка](NATIVE-ARCH-ACCEPTANCE.md).

## What works now

The supported DisVM/Limbo runtime already executes guests concurrently on native
OS threads. A resident keeps its Prog and live register storage. ACTIVE is an
admission/lifetime phase, not a mutex excluding other guests. Returning to an
ordinary C opcode handler therefore does not by itself imply serialization.

Native worlds reject both stopped collector acquisition APIs. Fast, Cycles and
Duke operate concurrently; retained tracing and RC fallback are forbidden.
The application regressions verify real owner/reference transfers, channel and
I/O waits, module continuation, cancellation, child exit, cyclic graphs and
collector/thread joins. This describes tested runtime behavior, not lock-free
implementation or absence of local metadata/channel/allocator mutexes.

DisVM/Limbo уже исполняет гостей параллельно на системных потоках. Prog и живые
регистры закреплены за своим потоком. ACTIVE не исключает других гостей; старое
имя обработчика C само по себе не означает однопоточность. Остановленные проходы
GC запрещены. При этом локальные mutex метаданных, каналов и аллокатора остаются.

## Broader acceptance, rather than another isolated edge case

The earlier selected collector/resident audit covered 33 CTest suites. The
broader gate selects every registered Python driver containing `--native-spawn`:
79 suites in the configured Linux x86-64 console build. This is a mechanical
selection of suites, not a claim that every subprocess in them uses native mode.
Some suites retain reference profiles and deliberate rejection controls.

Reproduce after configuring/building the console:

```sh
python3 disVM/runtime/native_suite.py build/disvm --list
python3 disVM/runtime/native_suite.py build/disvm --jobs 4
```

The initial broader run passed 73/79. All six failures were traced to outdated
acceptance instrumentation; the actual applications were not repaired or replaced:

| Suites | Cause | Updated acceptance |
| --- | --- | --- |
| native_cycle_gc, dedicated_cycle_gc | Required nonzero stopped tracing passes | Require strict policy, zero retained passes/snapshots/epochs and real shared graph reclamation; preserve reference-mode tracing checks |
| dedicated_spawn_portable | Required stopped GC after forty successful native guests | Require strict concurrent GC and retain exact thread identities, joins, payload/copy/frame and queue accounting |
| module_continuation, module_continuation_profiles | Fixture installed a global heap profiling callback, inhibiting shared reclamation and hanging its final drain | Observe the actual process-rooted returned message in ACTIVE on its own thread; preserve allocation faults, TLS emergency strings, Unicode, exception and frame checks; no global profiling hook |
| dedicated_channel_buffer | Suspended-root probe depended on a stopped root visitor, which strict workers never install | Execute a real bounded Fast attempt during native suspension, perturb/restore only this thread's decoded destination and verify saved continuation stays byte-identical; preserve separate reference visitor probe |

The channel C fixture was split by admission, movers, I/O, cancellation, native
waits and ALT. Before the probe change, expansion preserved native tokens and
preprocessor directive tokens exactly. Every changed handwritten source stays
below 500 lines. Those six changes adjust acceptance instrumentation. A separate broader TSan
run subsequently found a real production process-control race, described below.

The cycle workload completes 12000 unreachable-cycle iterations, guarded graph
publication and deep frame return/unwind, with real shared reclamation and zero
retained tracing. The portable module workload starts and joins forty distinct
guest OS threads. Its C path also completes under ThreadSanitizer; that run does
not instrument emitted JIT machine code.

Расширенный набор нашёл шесть устаревших проверок: ожидания остановленного GC,
недостижимого root visitor и включение старого глобального profiling-hook внутри
конкурентного теста. Нагрузки сохранены. Новые проверки требуют конкурентной
сборки, точной передачи владельцев, неизменности продолжений и настоящего join.
Это исправление приёмки, а не заявление о шести исправленных ошибках production.

## A production race found by broadening TSan

The portable forty-guest run completed cleanly under TSan. Extending the run to
module killwait found a race: the parent read `child->state` while that child
withdrew itself from Pready to native I/O. The parent's channel registry mutex
did not synchronize with the child's owner-local state write. Sleeping in the
fixture made the schedule reproducible but provided no happens-before edge.

Dedicated kill/killwait now read the existing atomic lifetime seal instead of
foreign `Prog.state`. Registration/lifetime is protected by the channel registry
mutex; a live child's exit is posted to its mailbox in any execution/wait phase.
The child handles it on its own OS thread. Sealing precedes Pexiting, so an
already retiring child requires only its existing exit notification. No global
exclusion, register reload or execution of the child by its parent is added.

Расширение TSan с обычного возврата на killwait выявило настоящую гонку:
родитель читал чужой Prog.state одновременно с уходом ребёнка в native I/O.
Mutex реестра не защищал запись владельца. Теперь kill/killwait читают атомарный
lifetime seal; выход передаётся через mailbox и обрабатывается самим ребёнком.
Уже закрытый retirement требует только уведомления родителя. Глобальный
эксклюзив и чужое исполнение при исправлении не вводятся.

## Concurrent channel continuation publication

Continuing the same TSan regression then found four reports with one cause:
foreign mapped-channel validation read the parked peer's FP/SP/EX/TS from Prog.R
while the owner still copied its live registers at dispatch end. A process
lifetime pin keeps the peer alive; it does not make register publication atomic.

Dedicated channel/ALT delrun now publishes the complete owner image while the
channel registry mutex still hides the parked waiter. Dispatch-end publication
uses acquire readiness and skips an off-queue guest. A completion publishes
readiness after its foreign frame readers have finished, permitting the owner
to publish again. Both generated dispatch publication sites use the common
helper; the single-threaded reference keeps its normal copy. Live TLS registers
are never restored from Prog.R. Generator idempotence is checked as well.

A repeated ordinary audit also exposed an arithmetic observer comparing native
context addresses after their owners could terminate. Sequential context storage
reuse made two genuine residents look identical. It now compares immutable
resident PIDs. ACTIVE is accepted only in a world that forbids stops; ordinary
serialized reference ACTIVE cannot satisfy the parallel evidence.

Следующий TSan-прогон выявил гонку чтения геометрии Prog.R партнёром с её поздней
записью владельцем. Lifetime pin не защищает сам снимок регистров. Теперь снимок
публикуется под mutex до видимости канального ожидания; завершение диспетчера не
перезаписывает его, пока готовность не опубликована после чтений партнёром.
Живые TLS-регистры из Prog.R не восстанавливаются. Отдельно исправлен наблюдатель
арифметики: сравниваются PID, а не повторно используемые адреса завершённых
контекстов; ACTIVE считается конкурентным только в мире с запрещёнными stop.

## Broader cancellation audit: three further race causes

TSan stream-kill and receiver-cancel workloads first found unguarded ring-size
reads in SEND/RECV after peer lookup. A lookup does not reserve the channel:
another resident may change its buffer before the caller claims that peer.
Receive preflight and both diagnostics now run inside the existing channel match
reservation, before dequeue. Movers remain outside the channel registry mutex.

Continuing receiver cancellation then found `prepare_cancelwait` reading the
child's owner-local `Prog.state` during a ready withdrawal. Runtime transitions
now additionally publish an atomic scalar `observed_state`. Parent/host request
admission reads that value and the atomic table-retry flag. This is only an
eligibility observation: the permanent owner resolves the mailbox request against
its actual current wait. It grants no access to registers or other wait fields.
Cancellation and retirement replies instead use correlated `exit_wait_pid` and
`cancel_waiting`, protected by the registry mutex, without reading foreign state.
The ordinary reference execution state and live TLS registers keep their roles.

The wider seven-profile TSan run next exposed a concurrent graph-capture read
against legacy `initmem` clearing a buffered `ref Packet` slot. Built-in `movp`
pointer rings now use the same guarded mapped-buffer transaction as record rings,
including source-owner consumption, displaced-owner retirement and commit-based
geometry publication. A channel reservation alone does not protect cells against
the concurrent collector. Custom movers are not silently treated as built-ins.

The three streaming process-control profiles additionally require actual guarded
buffered receive commits. The previous pointer path completes its real workload
but fails this new evidence check. Source generation was split into constructor,
match, queue, buffer, ALT-result and ALT-wait modules; extraction preserved Python
ASTs and embedded C constants before the two reviewed channel semantic changes.

Расширенные TSan-нагрузки выявили ещё три причины гонок: чтение геометрии кольца
после поиска пары без захвата канала; чтение чужого `Prog.state` при `cancelwait`;
очистку указателя буфера старым `initmem` одновременно с обходом графа GC.
Проверки геометрии перенесены под существующий захват передачи. Допуск запросов
читает отдельный атомарный скаляр, а ответ проверяется по идентификатору ожидания
под mutex. Pointer-кольцо использует защищённую транзакцию mapped-передачи.
Регистры остаются живыми в TLS своего владельца, глобальный эксклюзив не добавлен.

Final TSan validation passed all seven process-control profiles: default,
parent-exit, audit-query, stream-kill, receiver-kill, receiver-cancel and
table-handoff. Each runs C/JIT with atomic/locked GC polling: 28 VM executions
with 2568 exact guest starts/retirements in total. The pointer-ring observer
forwards the real transaction unchanged and reports only a committed Tptr
consume. The old path completes the receiver workload but fails this observer.
ALT destinations can use guarded transfers without qualifying for the optional
shared-channel optimization counter. Acceptance therefore observes the actual
consume rather than requiring that optimization counter.

The final tree also passed the 12 dedicated module-continuation executions under
TSan and the standalone runqueue ownership regression in normal and TSan builds.
The latter now checks foreign observations of runnable/native/table waits and
their completed transitions. These sanitizer runs instrument C and host code,
not emitted JIT machine code. No previously reported race remained in these runs.

Финальные TSan-прогоны: семь process-control сценариев, по четыре C/JIT/poll
профиля, всего 28 запусков VM и 2568 точных стартов/завершений гостей. Ещё 12
запусков проверили native-продолжения. Runqueue-проверка прошла в обычной и TSan
сборках. Наблюдатель фиксирует настоящую committed consume Tptr, не счётчик
необязательной shared-оптимизации; старый путь эту проверку не проходит.
TSan покрывает C/host, но не машинный код, выпущенный JIT.

The broad final production run passed 78/79; the remaining array-length suite
exposed shared diagnostic state in its native receive-error fixture. Its 50-site
PC replay table belongs to the separate reference capture workload and is now
bypassed only for the native commit-error workload. The dynamic address-publication
marker is TLS, matching the owner-local call it observes. All 50 reference replay
checks, omission controls and before/after-commit failures remain enabled in
their respective profiles. The 1254-line driver was split into immutable guest
oracles and five private C fragments; the coordinator is below 500 lines.
Extraction retained the guest strings, driver function AST and native/directive
tokens except the two scoped instrumentation corrections.

After these fixture corrections, the complete array-length suite passed its
targeted recheck. Its positive-only matrix also passed under TSan: two native
commit-error runs and eight reference C/JIT/poll runs. Generation idempotence
passed separately after adapter extraction. All 79 broad suites are therefore
covered by passing results on this production tree, with the corrected fixture
rechecked separately; this is not a single uninterrupted final 79/79 run.
Every changed handwritten source is at most 484 lines. Vendor snapshots remain
byte-for-byte unchanged.

Общий прогон production-дерева дал 78/79. Последний отказ оказался оснасткой
array-length: общая таблица PC использовалась в native-нагрузке ошибок commit,
а признак публикации адреса был глобальным. Таблица сохранена для всех 50
reference-проверок и исключена лишь из отдельного native-профиля; признак теперь
TLS. После этого полный array-length сценарий прошёл отдельно, положительная
матрица прошла под TSan. Все 79 сценариев покрыты успешными результатами с этой
целевой перепроверкой; единый непрерывный финальный 79/79 здесь не заявляется.

## Live parent after cancellation of a reply wait

The next lifetime audit expanded TSan to late parent retirement, queued/admitted
table termination, SEND/RECV/ALT close, native kill/cancel, two-table transfer,
FIFO edits and repeated file-slot I/O. These existing workloads passed; this
audit did not find another production defect in those paths.

A new whole-VM regression covers a different lifetime: the parent abandons its
prepared cancelwait but remains registered and starts new native work before the
child's old reply finishes. A private observer holds that actual child callback,
lets the grandparent cancel the parent's wait, then releases the callback against
256 real native leave/reentry pairs on the still-live parent's permanent thread.
It verifies unchanged Prog identity, REG storage and register values, retained
parent/child graphs, both exact completions and full guest/collector retirement.

The observer repeats the same still-owned stale lookup 4096 times to expose the
read/write overlap. These are repeated observations of one request, not 4096
independently delivered messages. The callback must remain a no-op after its
correlation is withdrawn. Restoring only the former foreign `parent->state` read
in a private main composition produces an actual TSan race against runqueue state
publication. The control checks the specific read/write diagnostic and rejects
unrelated sanitizer warnings; compiler/link failures never count as detection.
Production sources and objects are not replaced by the omission control.

`resident_evidence` now delegates to the common strict-GC evidence checker. All
of its application callers additionally require zero retained reclamation,
`gc_runs`, `gc_epochs`, idle retained RC passes and RC fallback, with joined
collectors. The earlier partial policy/snapshot checks did not cover all of these
counters. The new case is registered for native Linux64 without an x64-only hook;
TSan builds additionally register its old-reader control.

Повторный аудит позднего завершения, таблиц, каналов и I/O не обнаружил новой
production-ошибки в проверенных путях. Добавлена отдельная проверка живого
родителя: ожидание отменено, начались 256 новых native-переходов, а старый ответ
ребёнка ещё задержан. Проверяются тот же Prog, та же память REG и значения,
сохранённые графы, точные ответы и полное завершение потоков/GC. Повторяются
наблюдения одного старого запроса, а не доставка тысяч новых сообщений.
Частный возврат прежнего чтения `parent->state` воспроизводит настоящую гонку
TSan; production-код этот возврат не получает. Общая приёмка дополнительно
требует нулевых retained-GC эпох, проходов и RC fallback. Регистрация теста
не ограничена x64, но фактические прогоны здесь выполнены на Linux x64.

Reproduce the ownership case and its sanitizer control after matching builds:

```sh
ctest --test-dir build/disvm -R '^disvm_live_parent_late_reply$' --output-on-failure
ctest --test-dir build/disvm-tsan -R '^disvm_live_parent_late_reply' --output-on-failure
```

The registered normal and old-reader TSan tests passed three repetitions each:
24 positive C/JIT/poll VM executions and six expected race controls. A final
stricter diagnostic check also passed four positives and two controls. Generated
JIT instructions remain outside TSan instrumentation; host C and metadata are
instrumented. This adds a regression for the earlier fix, not another claimed
production race fix or complete certification of all architectures.

The stronger common gate initially passed 39/40 registered callers. Its remaining
file-overlap fixture waited for a transient global peak of exactly three native
leases. Both file writers could already be outside the heap while the unrelated
parent had finished formatted output; then the valid peak was only two. The
application had made the required independent progress, but the harness timed out.

The file observer now holds the first genuine native port write and observes the
real condition-wait inside `dvm_file_gate_enter` on that same gate before releasing
the head. It forwards every real admission/wait/write, preserves the unrelated
actor marker, exact two bytes, gate contention and complete-retirement checks,
and does not use global lease peaks as per-FD evidence. The private relinker
selects the matching console/editor target metadata; no LD_PRELOAD library is
needed. The changed console probe passed ten TSan repetitions (40 C/JIT/poll
executions). Editor execution was not run in this console workspace.

Seven lifecycle suites were also rerun successfully under TSan after the common
strict-GC check was strengthened: parent exit, table-scope kill, native kill,
native cancel, two-table transfer, FIFO edit and resident files. Together these
execute 128 positive VM profiles. These runs plus 28 late-reply positives and
40 file-overlap positives provide 196 positive TSan VM executions, with eight
separate expected old-reader controls; emitted JIT code is still not instrumented.

Усиленная общая проверка сначала дала 39/40: файловый тест требовал случайного
пика трёх native lease, хотя два писателя уже ожидали вне кучи, а независимый
родитель успел закончить вывод. Вместо пика теперь наблюдается настоящий
condition-wait FIFO того же FD при задержанной голове. Настоящие вызовы не
подменяются, проверки прогресса, байтов и очистки сохранены. Эта матрица прошла
40 TSan-запусков; ещё семь lifecycle-наборов — 128 запусков. Вместе с новым
поздним ответом проверены 196 положительных TSan-профилей и восемь ожидаемых
контролей старого чтения. GUI/editor в этом console-окружении не запускался.

The final post-correction common gate passed 40/40 registered suites in one run,
including the new late-reply case, file overlap, ownership/retirement and large
graphs. Changed handwritten sources are at most 341 lines; upstream snapshots
and production execution code are unchanged by this acceptance-only commit.

Финальный общий прогон после исправления оснастки: 40/40 за один запуск.
Изменённые исходники — максимум 341 строка; upstream и production-исполнение
этим коммитом приёмки не изменяются.

## What remains before claiming the entire requested target

- Finish reviewing cross-owner Prog.state/register reads and their publication
  contracts. Further real races were found by extending TSan after ordinary tests
  were green; this pass does not certify every remaining reader. Keep the broad
  application gate stable and test actual ownership paths rather than infer
  completeness from a counter or a handler name.
- The common eleven-workload C/JIT suite now passes on native AMD64 and ARM64
  runners, and on actual RV64 executables under QEMU. This is whole-VM execution,
  not isolated emission checks. RV64 hardware memory ordering/performance remains
  unverified, and the broader 80-suite gate has so far run on x64. See the linked
  architecture report for exact coverage and guarded C-tail JIT limitations.
- Arbitrary C finalizers do not become thread-safe by declaring the VM complete.
  Unknown callbacks remain explicit unsupported work. Supported runtime-built
  FD/table/key finalizers and normal managed types have dedicated concurrent
  paths; extension callback support needs an explicit ownership/thread contract.
- FlashVM is a separate later target, as requested; these results are DisVM/Limbo.

На Linux x64 и ARM64 есть успешные native C/JIT-прогоны общей матрицы; целевой
RV64-код прошёл её под QEMU. Остаются непроверенные пути поддержанного набора,
широкая приёмка вне x64 и RV64-железо. Число зелёных тестов не заменяет эту
проверку; точную дату общей готовности эти данные не обосновывают.

## Validation of the previous continuation change

The final production tree passed 78 suites in the broad run; its sole failing
suite was the unstable arithmetic identity observer. After replacing the stale
context-address comparison, that suite passed separately and its actual parallel
workload passed ten repetitions (40 C/JIT/poll profiles). Thus all 79 registered
suites are covered by passing results, including targeted revalidation of that
observer; this is not presented as a single uninterrupted 79/79 final run.

Final TSan validation passed 12 dedicated module-continuation VM executions
(including four forty-guest workloads) and all four C/JIT/poll process-control
profiles with 128 children/eight parents. No previous race diagnostics remained.
The initial unwrapped forty-guest portable workload also completed under TSan.
A final post-build targeted run passed 6/6 (arithmetic, process control, both
module continuation suites, native channel buffer and generation idempotence).
The native suspended-publication regression also verifies that an attempted
publication cannot change a parked image or reload live registers. Production snapshots upstream remain unchanged. All changed
handwritten source files are at most 446 lines.

Финальный общий прогон дал 78 успешных сценариев и отказ нестабильного
наблюдателя арифметики. После исправления он отдельно прошёл, включая 40
реальных повторных профилей. Все 79 сценариев имеют успешные результаты с
этой целевой перепроверкой; это не выдается за один непрерывный финальный
прогон 79/79. Финальные TSan-проверки продолжений и process-control прошли.

## Repeated cancellation of the same child

An interrupted `cancelwait` can be followed by another attempt on the same live
child before its old reply arrives. Each attempt now owns a unique operation ID
and a native message allocation, so an old result cannot complete the new call
or overwrite another queued reply. Queue teardown and resident stop dispose
owned nodes, including replies already staged outside admission. See
[CANCEL-GENERATIONS.md](CANCEL-GENERATIONS.md) for ownership, the deterministic
real-owner regression, omission control and exact validation scope.

Повторная отмена одного ребёнка теперь различает операции по ID, а не только PID.
Старый ответ не завершает новое ожидание; его память освобождается владельцем.
Локально пройдены 43/43 обычных и TSan-нагрузок и 99/99 расширенных CTests.
Это исправление DisVM/Limbo без глобальной остановки; оно не объявляет все пути
VM или аппаратную многопоточность RV64 полностью проверенными.

## Independent retirement replies

A child now reserves its own retirement reply before `killwait` parks the parent.
Two retiring children never reuse one parent-embedded queue node. Publication
transfers only native scalar storage after the child's process has been freed;
withdrawn interest, consumed/stale replies, queued/staged shutdown replies and
unused reserves each have an explicit disposal path. No allocation occurs during
retirement publication. See [RETIREMENT-STORAGE.md](RETIREMENT-STORAGE.md) for the
original aliasing failure, explicit stale mailbox debt and allocation-refusal retry.

Уведомления завершения разных детей теперь имеют разную память. Регрессия
проверяет настоящие завершения A/B при явно добавленных старых native-ответах;
она не выдаёт тестовый долг mailbox за операции Limbo. Ошибка выделения до парковки
оставляет родителя runnable и допускает успешный повтор без глобальной остановки.
