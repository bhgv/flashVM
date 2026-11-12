# Fast retry without waiting for unrelated retirement

Copyright (c) 2026 bhgv

## English

A native guest whose frame-retirement publication encounters backpressure now
waits for one subsequent Fast scheduling attempt, rather than for the whole Fast
queue to become empty. An unrelated object with an unsupported destructor can
remain rooted in that queue while other native guests continue executing.
Previously that object could prevent the publishing guest from returning until
all peers left heap execution.

`dvm_gc_workers_step_fast` records a ticket under the service mutex and coalesces
a Fast request. If a callback is already running, its completion does not satisfy
the ticket: the caller waits for a subsequent attempt. Attempts include a
retained-admission refusal, since that refusal must not turn this retry into a
wait for other guests. Existing pass statistics continue to count callbacks only.
The attempt counter saturates; further tickets are refused before overflow.
Service failure/shutdown and collector self-waits return failure.

The caller publishes its roots before leaving shared admission and retains its
frame owners until publication succeeds. Reentry keeps the same native context,
permanent Prog and live registers. This is a scheduling opportunity, not a
promise of allocation success, queue exhaustion, or a real-time latency bound.
The old wait-idle APIs remain available for explicit cleanup and shutdown.

The host regression orders an in-flight callback and a second request explicitly,
checks a subsequent callback while backlog remains, rejects collector self-wait,
and checks an idle ticket. The actual VM regression keeps two resident peers
admitted beside an unsupported finalizer, retries shared publication, verifies
context/admission and exact roots/counts, and requires eventual retained cleanup.
It covers C/JIT and both poll paths. A private control restoring the old whole-queue
wait must fail with the dedicated backlog diagnostic; timeout is not success.

This removes a dependency of guest progress on unrelated retained work. General
retained tracing and unsupported finalizers still use quiescent admission. This
change does not establish fully concurrent tracing or complete the 64-bit JIT
ports; no ARM64/RV64 hardware validation is implied.

## Русский

При переполнении публикации освобождаемого кадра нативный гость теперь ждёт одну
следующую попытку Fast вместо опустошения всей очереди. Посторонний объект с
неподдержанным деструктором остаётся под корнем до резервной очистки, но больше
не заставляет публикующий поток ждать выхода всех соседей из исполнения.

Запрос получает отметку под мьютексом службы. Уже начавшийся проход не считается
следующей попыткой. Отказ в резервном допуске тоже завершает попытку, иначе снова
возникло бы ожидание остальных гостей. Счётчик насыщается без переполнения;
ошибка службы, остановка и самоожидание сборщика возвращают отказ.

Перед выходом публикуются корни; владельцы кадра сохраняются до успешной передачи.
Возврат использует тот же контекст, постоянный Prog и живые регистры. Проверка на
настоящей VM держит двух соседей активными, сохраняет отложенный финализатор,
проверяет возврат, корни/счётчики и последующее освобождение. Возврат старого
ожидания в контрольной сборке должен дать конкретную диагностику блокировки.

Полная конкурентная трассировка ещё не завершена: общий резервный обход и
неподдержанные финализаторы пока требуют покоя кучи. Проверки на ARM64/RV64
этим изменением не заявляются.

## Validation on Linux x64

- 20 host tests passed, including the new ticket regression. Its extended
  callback-failure path also passed separately in normal and TSan builds.
- 40 selected runtime tests passed after revalidating three tests affected by
  an accidental overlapping rebuild. The packet-fairness CTest passed separately:
  16 positive VM profiles plus three semantic negative controls.
- The 16 positive packet profiles passed with TSan; the native C/host code and
  fixtures are instrumented. Emitted machine code is not C-TSan instrumented.
- Changed handwritten source files contain at most 400 lines. Upstream sources
  remain unchanged.
