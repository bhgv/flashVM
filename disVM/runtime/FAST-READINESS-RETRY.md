# [GC:] Fast readiness after an empty shared sample

Copyright (c) 2026 bhgv

## EN: The race and dispatch hint

The shared callback can sample no work and return zero while a producer publishes
new work before the following readiness check. The generic dispatcher previously
treated the new ready result as a reason to try retained quiescent collection.
With a guest still admitted, that acquisition would fail: this is an unnecessary
retained attempt, not evidence that the world actually stopped.

For quiescent-only Fast, `gc_worker_pass.c` now checks for an active guest before
trying retained admission. If present, it returns MORE for another shared slice.
This preserves the existing policy: quiescent collection could not succeed with
that guest active anyway. The observation reserves nothing; a guest entering
after the hint is still handled by the authoritative quiescent acquisition.
Retained work remains reachable once no active guest prevents admission.

The condition is limited to Fast and the quiescent-only dispatch branch. Paired
concurrent backends keep their closed route, and other retained policies remain.
This is not removal of the general stopped collector or shutdown protocol.

## RU: Готовность и повтор

После пустого shared-замера производитель может опубликовать новую работу.
Повторная проверка готовности раньше вела к попытке quiescent-прохода, хотя
активный гость заведомо исключал его успех. Теперь Fast в режиме quiescent-only
возвращает MORE и повторяет shared-обработку. Наблюдение не резервирует допуск;
сам захват по-прежнему повторно проверяет состояние мира.

Остановленная очистка остаётся доступной после ухода активных гостей. Общий
остановленный сборщик этим изменением не удаляется.

## EN: Deterministic native regression

`test_fast_readiness.c` calls the actual host dispatcher on its bound collector
context while a separate OS thread holds an actual active native participant.
The backend first samples an empty native work slot. A handshake makes the
producer publish real backend work after that sample and before readiness.
The shared callback truthfully reports zero completed work, while ready sees
the actual new publication. No managed heap or successful collector result is
substituted; this is a host-dispatch fixture, not a Limbo program.

Fast must return MORE without calling the quiescent acquisition boundary. After
the producer leaves and joins, the next shared slice consumes the publication.
A separate retained work request must then acquire real stopped admission and
finish its callback. Both polling modes run, with complete native context/world
cleanup and joined thread lifetime.

The Python driver also compiles a private dispatcher without the guest hint.
That binary must fail the exact active-producer predicate with exit 12; build
errors, signals, timeouts and sanitizer reports do not count as evidence.

The intermittent stopped-attempt assertion found during the
[publication retry](DUKE-PUBLICATION-RETRY.md) TSan series prompted this test.
The test reproduces the underlying dispatch sequence without scheduler luck;
the existing managed fixture's stricter no-attempt predicate remains intact.

Three existing semantic controls now observe shared retry directly: an actual
incomplete leaf epoch, a real duplicate index lookup or an injected mandatory
FULL reservation must produce a nonzero shared response. Their omission builds
can no longer rely on the generic dispatcher attempting quiescent admission,
because the new hint correctly suppresses that attempt. These observers apply
to positive runs too and never manufacture work. The unsupported-module control
continues to require its exact internal failure to retire, rather than the now
unreachable stopped-attempt marker. Positive physical-free/no-stop predicates
and rejection of external process timeouts remain unchanged.

## RU: Проверки

Настоящий OS-поток публикует native-работу строго между пустым замером и
проверкой готовности. Диспетчер обязан повторить shared, затем обработать работу.
После join отдельно проверяется доступность настоящего остановленного прохода.
Оба polling-режима проходят; частная сборка без проверки гостя должна завершиться
конкретной семантической ошибкой. Прежняя проверка отсутствия попыток остановки
при работающих Limbo-соседях не ослаблена.

The final series passed 96 selected runtime checks, three standalone host GC
checks and 100 TSan executions of this native binary. The managed regressions
also passed 552 instrumented VM executions; see the publication-retry validation
for the exact breakdown.
