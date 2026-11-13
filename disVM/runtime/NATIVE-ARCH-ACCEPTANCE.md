# Linux64 native acceptance / Приёмка Linux64 native

Copyright (c) 2026 bhgv

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

## Guarded JIT profile on ARM64/RV64

The default `core` profile retains these eleven workloads. `--profile wide`
selects forty explicit portable drivers, adding table edit/retirement and kill
paths, native services, shared JIT publication, mapped strings/accounting,
collector requests, service budgets, module cycles and exception retirement.
Architecture-specific inline-emitter oracles retain their existing CTest gates.
CI selects `wide` on all three targets and retains full per-driver logs and JSON
reports as artifacts for fourteen days, including failures. Local AMD64 passed
40/40; wider ARM64/RV64 acceptance awaits the corresponding workflow result.
The exception driver keeps its omission controls by default; its explicit
positive-only mode runs all four real C/JIT/poll release profiles.

Профиль `core` сохраняет 11 нагрузок; `--profile wide` выбирает 40 переносимых
драйверов, включая общую публикацию JIT, жизненный цикл таблиц, отмену, бюджеты
GC и освобождение исключений. CI запускает wide на трёх целях и сохраняет полные
журналы/JSON на 14 дней, также при отказе. Локально x64 прошёл 40/40; результаты
расширенного ARM64/RV64 набора фиксируются только после исполнения.

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
