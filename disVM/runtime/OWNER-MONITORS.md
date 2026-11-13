# Owner-local heap observers / Наблюдатели кучи владельца

Copyright (c) 2026 bhgv

The hosted `heapmonitor` callback is local to its OS thread. One permanent
native Prog keeps its own callback through waits and native calls; new resident
and collector threads begin with NULL. Installing, replacing or clearing the
callback does not affect a parent, peer or Fast/Cycles/Duke service. There is no
callback registry mutex, global mutator admission or implicit inheritance.

The callback ABI is unchanged: event, borrowed object/header and byte extent.
Existing invocation sites and constructor error unwinding remain in place.
The code pointer is borrowed, not a managed root: the installing owner must keep
its code/state valid through local invocation. This is an owner's diagnostic
observer, not a whole-VM profiler; a mutator's hook does not receive another
thread's allocation/free/trace events. Callback code that explicitly shares
other state still owns that state's synchronization.

CMake changes the generated interpreter declaration to GNU C99 `__thread`.
`prepare_owner_monitor.py` makes the matching definition after the regular
source/JIT stages. Generation tests compare the actual compiled heap source,
check the TLS header and regenerate every stage twice. The pinned upstream
snapshot and its global profiler interface are unchanged.

The previous constructor fault-injection TSan experiment reported a real race
between `imported_case` changing the global hook and Fast querying its presence.
That same driver now passes under TSan. Merely making the pointer atomic would
still let a collector/peer execute another owner's callback, and would retain
the global hook's suppression of shared Fast work. Thread-local storage removes
both cross-owner accesses; it does not disable the observers.

`test_owner_monitor.py` proves this with eight real resident threads:

- The parent keeps a distinct installed observer while children start empty.
- Even/odd residents install distinct callbacks, then overlap actual String
  allocations. Every callback checks its original context, Prog and identity.
- All eight first allocations are physically freed by a collector while parent
  and resident hooks remain installed. Completion is counted after real poolfree.
- Half the residents clear their hook; peers still receive callbacks. All then
  restore and exercise their own hooks before clearing them.
- A control atomically shares the callback selector; wrong callbacks must fail
  with exit 12 and the exact owner diagnostic. Timeout/sanitizer failure cannot
  pass as that control. Positive C/JIT and both polling modes remain mandatory.

The monitor extension introduced a 41-workload architecture profile. The subsequent
registered-child seal regression in KILLWAIT-SEAL.md extends it to 42. The x64 constructor oracle
also runs in both plain and TSan CI jobs; the former reference-hook race is no
longer excluded from the sanitizer gate. Architecture results must be recorded
after the new workflow actually executes.

В hosted runtime `heapmonitor` теперь локален OS-потоку. Постоянный native Prog
сохраняет собственный callback при ожиданиях; новые потоки гостей и сборщиков
начинают с NULL. Установка/смена/снятие не меняет hook родителя, партнёра либо
Fast/Cycles/Duke. Общий mutex callbacks и эксклюзив исполнения не добавлены.

ABI event/object/extent и существующие места вызова сохранены. Указатель кода
заимствован и не является GC-корнем; его состояние должно жить до завершения
локальных вызовов. Это диагностика владельца, а не глобальный profiler: события
другого потока ему не пересылаются. Явно общие данные callback защищает сам.

TLS-декларация создаётся CMake, matching definition — отдельной стадией после
генерации runtime/JIT. Проверяется повторяемость всех стадий, фактический heap.c
сборки и TLS-header. Upstream не изменяется. Старый constructor/fault-injection
тест, обнаруживший гонку глобального hook с Fast, теперь проходит под TSan.

Новая проба проверяет настоящие callbacks восьми потоков, отсутствие наследования,
локальное отключение/возврат и восемь завершённых физических frees сборщиком при
установленных hooks. Намеренно общий atomic-selector должен дать ровно код 12 с
диагностикой чужого callback. Таймаут и sanitizer-ошибка не засчитываются.
Монитор расширил wide до 41 нагрузки; регрессия KILLWAIT-SEAL.md — до 42.
x64 constructor-oracle включён
также в TSan CI. Результат новых архитектур фиксируется только после исполнения.


## Verified execution / Проверенное исполнение — 2026-11-13

[Workflow 43355415904](https://github.com/bhgv/flashVM/actions/runs/43355415904)
completed successfully on `1c8c7ead36370c5927490c02744d3ec0475d9ecd`:

| Target/profile | Execution | Wide workloads |
| --- | --- | --- |
| AMD64 plain | Native Ubuntu x64 | 42/42 |
| ARM64 plain | Native Ubuntu ARM64 | 42/42 |
| AMD64 TSan | Native Ubuntu x64, instrumented C runtime/probes | 42/42 |
| RV64 | Actual RV64 executables/JIT under QEMU/binfmt | 42/42 |

The owner-monitor and forced killwait-seal probes passed in every job. The
previously failing resident-errors workload also passed on RV64. The independent
x64 constructor oracle passed in both plain and TSan jobs, closing its previously
recorded global-monitor race rather than retaining a sanitizer exclusion.
All four jobs uploaded full driver logs and JSON reports with fourteen-day
retention. No timeout or diagnostic failure was accepted as a positive profile.

Local x64 validation passed 42/42 in the wide TSan matrix and 83/83 in the broad
CTest gate: eighty earlier native-spawn suites, owner monitors, killwait sealing
and source-generation idempotence. The new tests' semantic omission controls
also passed locally in normal and TSan builds: a shared callback selector gave
the exact foreign-owner failure, and the restored old killwait admission gave
real NotWaiting while its sealed child remained registered. `readelf` confirmed
the runtime's heapmonitor symbol is TLS storage of one pointer, not a global
OBJECT. Every changed handwritten source is at most 437 lines; upstream is
unchanged. This final result update changes documentation only.

TSan covers compiled C runtime/probes, not emitted JIT instructions. The ARM64
job is native hardware; the RV64 QEMU job remains ABI/functional execution,
not certification of RV64 hardware memory ordering or parallel throughput.

Указанный workflow завершился успешно на указанном коммите: native x64 и ARM64,
x64 TSan и настоящий RV64 userspace/JIT под QEMU — по 42/42. Обе новые регрессии
прошли на каждой цели; resident-errors на RV64 теперь тоже прошёл. Старый
constructor-oracle прошёл в plain и TSan CI, без прежнего исключения санитайзера.
Полные журналы/JSON сохранены для четырёх job на 14 дней.

Локально wide TSan прошёл 42/42; широкий CTest — 83/83, включая генерацию и обе
новые регрессии. Контроли в обычной/TSan-сборке подтвердили настоящие отказы:
чужой callback при общем selector и NotWaiting старого killwait для sealed-child
в реестре. В ELF heapmonitor является TLS-указателем. Изменённые исходники не
длиннее 437 строк; upstream сохранён. Итоговый commit меняет лишь документацию.
TSan не инструментирует JIT-инструкции; QEMU не заменяет проверку RV64-железа.
