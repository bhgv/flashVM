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
Per-case logs and `report.json` are written under `build/native/native-acceptance`.
Cases run with bounded concurrency and every failed driver makes the suite fail.

The workflow `.github/workflows/native-disvm.yml` builds on native AMD64 and ARM64
Ubuntu runners. Its separate RV64 job builds and runs in an RV64 Debian userspace
under QEMU. The emulator job checks code generation/ABI and functional behavior;
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
