# Strict retirement admission / Строгий допуск освобождения

Copyright (c) 2026 bhgv

## Live strict sessions

A declined RC transfer previously returned zero to `destroy()`, which then
continued synchronous destruction under the retained `gclock`. Invalid admission
or a closed retirement queue could therefore bypass the native concurrency policy.

`dvm_runtime_rc_defer()` now rejects that path with status 70 while the strict
RC world is live. The diagnostic distinguishes `retirement-not-admitted` from
`retirement-admission-closed`. Rejection occurs before RC reads the value or
changes its reference count, external root or type ownership. `destroy()` may
already have checked its allocation header before entering RC.

The single-threaded reference and cleanup after the RC world has been finished
retain their existing behavior. This does not permit callbacks during a live
strict session. Unknown finalizers remain explicitly unsupported.

Раньше отказ передачи RC возвращал ноль в `destroy()`, после чего выполнялось
старое синхронное разрушение под `gclock`. Неверная фаза, чужой мир или закрытая
очередь могли обходить запрет эксклюзива. Теперь живой строгий сеанс завершается
с кодом 70 до чтения объекта внутри RC и изменения его владения. Причины отказа
различают недопустимый допуск и закрытую очередь. Эталонный режим и очистка после
завершения RC сохраняют прежний контракт.

## Drain admission

Pre-close concurrent draining belongs only to the runtime controller after guest
joins. An admitted SHARED caller must not request work and wait on its own reader
epoch. A foreign controller must not enqueue work before discovering its error.

The drain now checks context identity, matching world, ACTIVE/IDLE phase, pending
progress, object guards and in-flight guest count before requesting any lane or
changing phase. Invalid calls return zero without changing admission. An ACTIVE
controller temporarily leaves its epoch and restores its original admission;
its live registers are never reloaded from `Prog.R`.

Drain до закрытия разрешён только контроллеру runtime после join гостей.
SHARED-вызов иначе мог ждать завершения собственной эпохи. Чужой контроллер
мог поставить работу в очереди до обнаружения ошибки. Теперь все предварительные
условия проверяются до запросов и смены фазы; недопустимый вызов возвращает ноль.

## Evidence and scope

`test_concurrent_guards.py` checks C dispatch and x64 JIT, each with both polling
modes: 36 exact production refusals, four real release roundtrips, four SHARED
controller drain refusals and 24 semantic omission controls. Refusal observations
require the actual source to retain reference count 1, root count 1 and type count
2. Omission controls require an exact fixture failure; a crash, timeout or
sanitizer diagnostic never counts as detection.

The positive release frees a real managed object while two actual guest sleep
frames remain shared. The native return keeps the same context, Prog and live
PC/FP. Drain controls verify rejection before any worker request and preservation
of the caller phase. `--positive-only` selects eight normal release/drain profiles
for sanitizer builds; it does not run intentional process-termination probes.

Проверки используют настоящие объекты, корни, системные потоки и очереди.
Контрольные мутации удаляют именно защиту production и обязаны дать конкретный
отказ теста. Положительный сценарий освобождает объект при двух допущенных гостях;
возврат native сохраняет контекст, Prog и живые регистры. Это проверено на Linux
x86-64. Проверки C под ThreadSanitizer не инструментируют машинный код JIT;
ARM64/RV64 и полная реализация их JIT этим проходом не подтверждаются.

Validation on Linux x86-64: the selected native audit passed 29/29, host checks
21/21, and a final rebuild passed the guard test plus RC roles, managed-owner
roles and the large-record cycle test (4/4). TSan passed all eight new positive
profiles, 12 managed-retirement profiles, 36 module-retirement profiles and the
GC-request suite; the resident-file suite also passed all 48 profiles under
TSan. A broad TSan CTest selection mistakenly included intentional
negative controls: managed control 3 and module control 5 deliberately exited
before joining fixture threads and produced thread-leak diagnostics. Those
failures are not counted as passing sanitizer evidence; the positive-only
runs above completed separately without sanitizer diagnostics.
