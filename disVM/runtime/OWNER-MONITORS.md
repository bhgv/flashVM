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

The wide architecture profile now has 41 workloads. The x64 constructor oracle
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
В архитектурном wide-наборе теперь 41 нагрузка; x64 constructor-oracle включён
также в TSan CI. Результат новых архитектур фиксируется только после исполнения.
