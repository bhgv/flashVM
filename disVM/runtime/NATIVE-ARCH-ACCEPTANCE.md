# Linux64 native acceptance / Приёмка Linux64 native

Copyright (c) 2026 bhgv

**Current result:** [44/44 in all four completed configurations](NATIVE-STATUS.md).
The detailed sections below retain earlier architecture acceptance history.

`native_arch_suite.py` runs the same eleven whole-VM workloads directly, without
depending on x64-only CTest registration. It checks Linux, pointer width, running
userspace and the configured CMake CPU before starting guests. A CPU mismatch is
an error, never an interpreter substitution. Both C and JIT execution modes are
exercised by the existing drivers; emitted JIT code is not TSan-instrumented.

The workloads cover native continuations, process control, receiver cancellation,
late replies to a living parent, native I/O cancellation, two-table transfer,
managed/module retirement, cycle-owner rescue, Duke pins and packet cycles.
Graph drivers use their positive/refusal variants; deliberately broken omission
controls are excluded from architecture acceptance. Existing strict no-stop GC,
thread identity, payload, ownership and retirement checks remain in those drivers.

```sh
cmake -S disVM/runtime -B build/native -DDVM_RUNTIME_TESTS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build/native --parallel 4
python3 disVM/runtime/native_arch_suite.py build/native --expected amd64
```

Use `--expected arm64` or `--expected riscv64` on the corresponding userspace.
`--emulated` labels an emulated run; it changes neither tests nor runtime policy.
`--list` lists workloads without claiming any architectural execution.
Per-case logs and `report.json` are written under `build/native/native-acceptance/<profile>`.
Cases run with bounded concurrency and every failed driver makes the suite fail.

The workflow `.github/workflows/native-disvm.yml` builds on native AMD64 and ARM64
Ubuntu runners. Its separate RV64 job cross-builds with the runner's GNU RV64 toolchain, then
executes the actual RV64 binaries under QEMU with the matching target sysroot. The emulator job checks code generation/ABI and functional behavior;
it cannot certify RV64 hardware memory ordering, timing or parallel throughput.
The workflow has read-only repository permissions, no persisted checkout token,
pinned action revisions and no deployment. Native module tutorial/compiler/editor
services remain outside this console acceptance configuration.

Local validation before the first workflow run passed all eleven workloads on
Linux x86-64. Requesting ARM64 on this x64 workspace was rejected before guests
started. The five graph suites additionally passed 132 positive profiles under
TSan: managed 12, module 36, rescue 16, Duke 36 and packet cycles 32. No new
production race was found in that audit. ARM64/RV64 workflow results must be
recorded after actual execution; workflow source alone is not acceptance.

Скрипт выполняет одинаковые одиннадцать нагрузок настоящей VM независимо от
x64-specific регистрации CTest. До запуска проверяются Linux64, CPU userspace
и CMake. Несовпадение архитектуры даёт ошибку, а не подмену интерпретатором.
Проверяются C/JIT, native-продолжения, отмена, поздние ответы, I/O, таблицы,
владельцы модулей и строк, циклы и Duke. Политика без остановки потоков и
проверки точных владельцев/данных/завершений сохраняются в исходных драйверах.

В workflow x64 и ARM64 работают на нативных Ubuntu runner. Отдельный RV64 job
исполняет RV64 userspace под QEMU. Эмуляция проверяет ABI и функциональность,
но не заменяет проверку порядка памяти и настоящей параллельности на RV64 CPU.
Права репозитория — только чтение, checkout token не сохраняется, deployment нет.
Локально Linux x64 прошёл 11/11; несовместимый ARM64-запрос был отвергнут.
Пять графовых наборов прошли 132 положительных TSan-профиля. Результаты ARM64/RV64
следует фиксировать после настоящего workflow-run, не по наличию YAML.

## Wide workload profile / Расширенный набор

The default `core` profile retains these eleven workloads. `--profile wide`
selects forty-two explicit portable drivers, adding table edit/retirement and kill
paths, native services, shared JIT publication, mapped strings/accounting,
collector requests, service budgets, module cycles and exception retirement.
Architecture-specific inline-emitter oracles retain their existing CTest gates.
CI selects `wide` on all three targets and retains full per-driver logs and JSON
reports as artifacts for fourteen days, including failures. Local AMD64 passed
40/40 in the predecessor matrix; those results are recorded below. The owner-local
observer extension is documented in [OWNER-MONITORS.md](OWNER-MONITORS.md); the
forty-second registered-child seal regression is in [KILLWAIT-SEAL.md](KILLWAIT-SEAL.md).
The exception driver keeps its omission controls by default; its explicit
positive-only mode runs all four real C/JIT/poll release profiles.

Профиль `core` сохраняет 11 нагрузок; `--profile wide` выбирает 42 переносимых
драйверов, включая общую публикацию JIT, жизненный цикл таблиц, отмену, бюджеты
GC и освобождение исключений. CI запускает wide на трёх целях и сохраняет полные
журналы/JSON на 14 дней, также при отказе. Локально x64 прошёл 40/40; результаты
расширенного ARM64/RV64 набора фиксируются только после исполнения.

The first wide local TSan run passed 39/40 and exposed a real table wake race:
`append_ready` wrote retained `Prog.state` under the table scheduler mutex while
dispatch classified the same state under the unrelated channel mutex. Dispatch
now samples one acquire-loaded `observed_state` scalar and classifies all allowed
ready/wait states from that value. It never reads channel fields or reloads live
registers. The existing table scheduler and atomic publication remain unchanged;
no global admission lock is added. The wide native CI matrix now includes an
AMD64 TSan job, so successful unsanitized architecture runs cannot hide this race.
The ephemeral TSan runner uses 28 mmap randomization bits for shadow compatibility;
VM/driver failures and sanitizer diagnostics remain failures, without retries.
The same audit moved generic cancellation classification, owner mailbox request/
reply admission and shared-dispatch recognition onto the atomic scalar as well:
the channel mutex alone cannot exclude a table scheduler's readiness write.
The bounded follow-up audited every remaining host composition state query:
channel claims, detached/duplicate ALT close entries and child wait/exit checks
also use the published scalar. `host_runtime` no longer reads raw `Prog.state`.
Queue or scheduler pins still protect lifetimes; the scalar is not a claim on
channel fields or a permission to execute a foreign process.

Первый расширенный x64 TSan-запуск дал 39/40 и обнаружил гонку пробуждения таблицы:
запись `Prog.state` под mutex таблиц пересекалась с чтением под mutex каналов.
Конец dispatch теперь классифицирует один acquire-снимок `observed_state`;
поля каналов и сохранённые регистры не читаются. Общий эксклюзив не добавляется.
В CI добавлен отдельный AMD64 TSan job; его ошибки не заменяются повторным
успешным запуском. Для shadow-памяти временного runner задано mmap_rnd_bits=28.
Проверки общей отмены, mailbox-запросов/ответов владельца и shared-dispatch также
читают атомарный скаляр: mutex каналов не исключает запись готовности таблицей.
Ограниченный повторный проход охватил все оставшиеся state-запросы host-слоя:
захваты каналов, снятые/повторные ALT-узлы и ожидание/выход детей. В host_runtime
прямых чтений Prog.state больше нет. Очереди/pin по-прежнему защищают lifetime;
скаляр не даёт доступа к чужим полям канала или права исполнять чужой процесс.

The wider local optimized GCC 14 gate additionally exposed two observer defects.
The old constructor oracle initialized ten synthetic wait states directly;
those now use `dvm_runqueue_set_state`, like real transitions. Its 1385-line C
fixture was split into five ordered probe modules and a coordinator; token
comparison verified the split before the ten publication changes. The UTF-8
observer now tracks real `calloc` as well as `malloc`: GCC can lower allocation
followed by zeroing to `calloc`. It still requires exactly one allocation/free,
zero writes to a published cache and eight private cache grows. Both failing
suites passed targeted reruns on the unchanged optimized runtime.

At the forty-workload checkpoint the constructor fault-injection oracle was an
unsanitized x64 check.
An additional local TSan experiment on its reference profiles detected its
dynamic global `heapmonitor` replacement racing a collector's hook-presence
query. That experiment was not a successful TSan result and was outside those
forty strict-native drivers. The scalar-state correction did not fix the hook's
lifecycle, so the x64 oracle then targeted the plain build. The subsequent
[owner-local monitor change](OWNER-MONITORS.md) fixes that lifecycle and includes
the oracle in TSan CI; the independent strict-native matrix remains mandatory.

Оптимизированный GCC 14 выявил два дефекта оснастки: десять искусственных wait
состояний старого constructor-oracle теперь публикуются штатным setter, а UTF-8
наблюдатель учитывает настоящий calloc после оптимизации malloc с обнулением.
Требования «одно выделение/освобождение, ноль записей опубликованного кэша,
восемь private grow» сохранены. Файл из 1385 строк разбит на пять модулей и
координатор с проверкой токенов до изменений публикации. Оба отказа исправлены
и проверены отдельно на той же оптимизированной VM.

На checkpoint из 40 нагрузок дополнительный TSan-эксперимент со старым oracle
обнаружил гонку переключения глобального heapmonitor с запросом сборщика.
Это не было успешным TSan-результатом; переключение hook тогда требовало
отдельного исправления. Последующая owner-local адаптация описана в
OWNER-MONITORS.md и включает oracle в TSan CI. Сам checkpoint из 40 нагрузок
остаётся историческим результатом и не включает этот эксперимент в число успехов.

The first wide RV64 run passed 39/40. Its controlled table preflight observer
could lose an earlier park notification: all competing owners could park before
the paused head published its observer identity. The barrier now records the
actual parked table and tests that durable predicate under the fixture mutex,
including arrivals before the head. Every controlled C/JIT/poll run must report
`TABLE_PREFLIGHT parked_history=1`. A bounded thirty-second internal wait fails
with a specific diagnostic; expiration is never success. The outer sixty-second
VM timeout is unchanged. No production table/guard code was changed for this
observer fix. The updated architecture results are recorded after execution.

Первый широкий RV64 дал 39/40: барьер контролируемого preflight терял раннее
уведомление, если все партнёры успевали припарковаться до регистрации головы.
Теперь сохраняется факт настоящего ожидания таблицы и проверяется под mutex,
включая ранний приход. Каждый controlled C/JIT/poll обязан дать parked_history=1.
Внутренний предел 30 секунд приводит к точному отказу, а не к успеху; внешний
таймаут VM 60 секунд не увеличен. Production-код таблиц/guard не менялся.

## Guarded JIT profile on ARM64/RV64

The first native ARM64 CI run built successfully but passed only 1/11 workloads.
The native JIT startup path skipped guarded-operation defaults on non-x64 CPUs;
several tests then exposed old inline native handlers bypassing checked ownership
and channel paths. Explicit guarded JIT had also been rejected on these targets.

Native startup now selects guarded operations on all three targets. Hosted ARM64
and RV64 compilers emit native tail entries to the existing checked C executor
when guarded mode is selected. These tails preserve the original return address
and stack, pass a NULL unchecked callback and use the guarded C opcode handlers.
This is real emitted native code with C handlers, not a new optimizing inline
implementation of every opcode. Legacy inline emitters remain reference-only;
no global mutator exclusion or register reload is introduced.

RV64 compiler reuse/publication additionally uses the builder mutex and acquire/
release JIT-table protocol already used by AMD64/ARM64. The generated view has a
final `prepare_guarded_leaves.py` stage after `prepare.py`; CMake owns both stages
and generation-idempotence tests verify both, including the RV compiler view.
Upstream files remain unchanged. Local x64 execution passed 11/11 again, and
host-side C compilation of both ARM64 and RV64 compiler views passed. Actual
ARM64/RV64 results for the fix are recorded below.

Первый ARM64 CI собрал VM, но прошёл лишь 1/11: native JIT пропускал defaults
защищённых операций на CPU вне x64. Теперь native-профиль включает guards на
всех трёх целях. ARM64/RV64 выдают настоящие машинные tail-входы к проверенным
C-opcode, сохраняя адрес возврата/стек и не передавая unchecked callback.
Это не утверждение о новой оптимизированной inline-реализации каждой операции.
RV64 дополнительно получает существующий протокол публикации JIT-таблицы.
Обе стадии генерации проверяются на повторяемость; upstream не изменяется.

## Bounded RV64 build cost

The RV64 job uses a native host cross-compiler rather than emulating GCC itself.
`QEMU_LD_PREFIX` selects the matching `/usr/riscv64-linux-gnu` runtime sysroot;
registered binfmt executes the RV64 bootstrap compiler, VM and private probes.
The Python harness can run on the host in explicit emulation mode, but verifies
ELF64 little-endian `e_machine` for both VM and bootstrap compiler and the CMake
target before any workload. A host ELF labelled as RV64/ARM64 is rejected.
Reports distinguish the target backend, Python `host_machine` and emulation.
No workload or native ownership check is skipped by cross-building.

RV64 собирается host cross-компилятором, а не GCC под эмулятором. QEMU/binfmt
исполняет настоящие целевые bootstrap/VM/probe бинарники с согласованным sysroot.
Host Python в явном режиме эмуляции проверяет ELF64/CPU обоих executable и CMake
до нагрузки. Host-образ с чужой архитектурной меткой отвергается. Пропуска тестов
или ослабления проверок владения cross-сборка не добавляет.

## Verified execution, 2026-11-13 / Проверенное исполнение

The final [workflow run 43336377681](https://github.com/bhgv/flashVM/actions/runs/43336377681)
tested commit `37474c501299f0a30c53ced0668f2f2d4c0f3a1b` and completed successfully:

| Target | Execution | Whole-VM workloads |
| --- | --- | --- |
| AMD64 | Native Ubuntu x64 runner | 11/11 |
| ARM64 | Native Ubuntu ARM64 runner | 11/11 |
| RV64 | Cross-built RV64 executables, QEMU/binfmt and target libc | 11/11 |

Both C and JIT modes and atomic/locked polling are exercised inside these drivers.
The RV64 job executed the target bootstrap compiler, VM, private probes and
generated RV64 JIT tails. Its success does not establish RV64 hardware ordering
or throughput. ARM64 went from 1/11 before the guarded-JIT correction to 11/11
after it. The emitted ARM64/RV64 tails use checked C opcode implementations;
this does not claim optimized inline lowering for every operation.

The final local x64 broad native-spawn gate passed 80/80 registered suites in
one uninterrupted run. The same eleven-workload matrix also passed on the
rebuilt x64 TSan executable after the guarded-JIT correction; generated machine
instructions themselves remain outside TSan instrumentation. Native-generation
idempotence passed with both stages. Every changed handwritten source is at most
409 lines; vendor snapshots are unchanged. This final documentation-only update
does not alter the tested execution code.

Финальный CI указанного коммита завершился успешно: x64 и ARM64 — настоящие
нативные runner, по 11/11; RV64 — настоящие целевые executable/JIT под QEMU,
11/11. ARM64 до исправления guarded-JIT имел 1/11. Это не утверждение о проверке
RV64-железа или об оптимизированном inline lowering всех opcode. Общий набор
x64 прошёл 80/80 одним запуском; обновлённая x64 TSan-сборка прошла ту же матрицу
11/11. TSan не инструментирует машинные инструкции, выпущенные JIT. Исходники
ограничены 409 строками; upstream не изменяется.


## Verified wide execution, 2026-11-13 / Расширенная приёмка

[Workflow run 43345063586](https://github.com/bhgv/flashVM/actions/runs/43345063586)
completed successfully on `3c4fc1ca19f77f475a8ef7ca466d08e3e0000efd`:

| Target/profile | Execution | Whole-VM workloads |
| --- | --- | --- |
| AMD64 plain | Native Ubuntu x64 | 40/40 |
| ARM64 plain | Native Ubuntu ARM64 | 40/40 |
| AMD64 TSan | Native Ubuntu x64, instrumented C runtime/probes | 40/40 |
| RV64 | Actual RV64 executables and JIT tails under QEMU/binfmt | 40/40 |

The x64 plain constructor/channel oracle also passed its separate CI gate.
Full driver logs and JSON reports were uploaded for all four jobs. The first
wide x64 TSan audit found the real raw-state race (39/40); the corrected strict
matrix passed 40/40 locally and in CI. The first wide RV64 run exposed the
preflight observer's lost notification (39/40); the durable-history observer
passed all forty on RV64, including controlled C/JIT and both polling modes.
No VM timeout was raised and no failure was accepted as a successful profile.

The local optimized x64 broad gate passed 81/81: eighty registered native-spawn
suites plus the generation-idempotence gate. The updated preflight observer was
also checked separately with the final predicate under normal and TSan builds.
A final test-only guard rejects an absent captured table before comparing the
park history, preventing NULL/NULL from becoming a witness; that additional
guard passed the complete local table driver in both builds. It was added after
the architecture run above. This result/guard commit changes no production code
and does not rerun the full architecture matrix.

Every changed handwritten source is at most 437 lines. The constructor oracle's
largest fragment is 344 lines and its coordinator is 96. Vendor snapshots are
unchanged. TSan does not instrument emitted machine instructions. RV64 QEMU
success remains functional/ABI evidence, not RV64 hardware ordering/throughput
certification. The legacy reference oracle's dynamic heapmonitor race described
above was open at this forty-workload checkpoint. Its subsequent owner-local
fix and the forty-first workload are documented in OWNER-MONITORS.md.

Финальный workflow указанного коммита завершился успешно: native x64 и ARM64,
x64 TSan и настоящий RV64 userspace/JIT под QEMU — по 40/40. Отдельный x64
constructor/channel oracle также прошёл CI. Во всех четырёх job сохранены полные
журналы драйверов и JSON. Исправление настоящей гонки состояния дало TSan
39/40 -> 40/40; исправление потери раннего события в оснастке дало RV64
39/40 -> 40/40. Таймауты VM не увеличивались, отказы не считались успехом.

Локальная широкая проверка прошла 81/81: 80 native-spawn наборов и генерация.
Финальная дополнительная проверка непустой захваченной таблицы проверена после
CI отдельным полным table-драйвером в обычной и TSan-сборке; production-код
не меняется. Исходники ограничены 437 строками, upstream не изменён. TSan не
инструментирует выпущенные JIT-инструкции, QEMU не заменяет RV64-железо.
Гонка heapmonitor была открыта на этом checkpoint из 40 нагрузок; последующее
исправление owner-local hooks и 41-я нагрузка описаны в OWNER-MONITORS.md.


## Current 42-workload result / Текущий набор из 42 нагрузок

The owner-local monitor and registered-child seal changes are verified by
[workflow 43355415904](https://github.com/bhgv/flashVM/actions/runs/43355415904),
commit `1c8c7ead36370c5927490c02744d3ec0475d9ecd`: native AMD64, native ARM64,
AMD64 TSan and actual RV64 executables/JIT under QEMU each passed 42/42.
The constructor oracle also passed both x64 CI profiles. Local wide TSan passed
42/42; the broad native CTest gate passed 83/83 including both new regressions
and generation idempotence. Full results and observer semantics are in
[OWNER-MONITORS.md](OWNER-MONITORS.md); the deterministic old-result control is
in [KILLWAIT-SEAL.md](KILLWAIT-SEAL.md). The previously open hook race is fixed.
The source ceiling remains 437 lines and vendor snapshots remain unchanged.
RV64 hardware ordering/throughput and JIT machine-code instrumentation are not
claimed by these results.

Текущий итог: по 42/42 на native x64/ARM64, x64 TSan и RV64 под QEMU. Локально
42/42 TSan и 83/83 native CTest. Обе новые регрессии и генерация входят в проверку;
бывший отказ constructor-oracle закрыт и проверяется TSan CI. Полные данные и
контроли указаны в двух документах выше. Исходники ограничены 437 строками,
upstream не изменяется; QEMU не является проверкой настоящего RV64-железа.


2026-11-14: mandatory workload list now62 includes retained-debug and
jit-quarantine. Selected local ordinary/TSan regressions do not establish new
whole-matrix completion. Previous cf066b75 and47665a architecture CI runs
43605594611/43605884568 completed successfully. / Обязательный список теперь62;
новые retained-debug/jit-quarantine требуют своей полной CI-матрицы. Предыдущие
указанные CI завершились успешно.


2026-11-14: list63 adds jit-signal with actual RX traps, retained native CPU
context and unsafe-path refusals. Prior e123a54d CI43617004556 failed retained-debug
socket mode:0777 & ~0077 gives0700, not0600. Creation now uses0177 and a host-independent
mask/restoration probe; full AF_UNIX execution remains a CI check on this socket-denied
host. / RU: Список63; исправлена маска socket до0600, добавлена проверка umask
без AF_UNIX. Новую полную архитектурную матрицу нельзя считать пройденной заранее.
