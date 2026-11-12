# [GC:] Shared snapshot retry beside retained work

Copyright (c) 2026 bhgv

## EN: Preserve the observed retry result

`dvm_runtime_rc_shared_leaves` had a graph-local `retry` variable shadowing the
pass result. A BUSY confirmation or FULL/BYTES refusal to replay a stale snapshot
could rotate its unchanged entry in `record_retries` but leave the outer retry
flag clear. A final readiness heuristic recovered the signal only when no other
retained work was ready. With an unsupported mandatory owner queued, the function
could report zero despite observing a genuine shared retry.

The graph-local decision is now named `snapshot_retry` and explicitly contributes
to the pass retry result before queue movement or rotation while a guest is active.
This preserves shared continuation independently of other pending work in that
phase. At quiescence, retained cleanup remains eligible to release resources
blocking shared progress; an unconditional retry would starve that cleanup.
Hard replay refusal still
keeps its existing difficult/retained routing; queue/root/reference ownership is
unchanged. This fixes the callback result, not a demonstrated loss of heap owners.
The active-guest hint described in [Fast readiness](FAST-READINESS-RETRY.md) could
mask the old result by deferring quiescent dispatch; it is not the retry protocol.

## RU: Сигнал повтора не зависит от соседней работы

Локальная переменная графа скрывала итоговый retry прохода. При BUSY или отказе
FULL/BYTES запись оставалась в shared-очереди, но наличие резервной работы могло
обнулить ответ обработчика. Теперь при активном госте решение графа явно
добавляется к итоговому флагу до перемещения/поворота очереди. В покое резервная
очистка остаётся доступной: она может освободить ресурс для shared-повтора.
Ссылки, корни и маршруты жёсткого отказа не меняются.

## EN: Real mixed backlog regression

The [native claim storage](FAST-CLAIM-STORAGE.md) fixture gains four positive
profiles. Each publishes an actual last-owner managed object whose Type uses an
unrecognized destructor. Production RC puts it in the mandatory FIFO with its
real root. It remains queued while two real Limbo peers stay admitted; the test
does not fake a ready predicate, collector result or managed allocation.

| Shared graph work | Injected transient refusal | Retained backlog |
| --- | --- | --- |
| 1,040 nodes, 514 native Modules | Eight actual vector malloc failures | One actual unknown final owner |
| 64 two-slot cyclic records | Eight FULL responses at real replay reservation | Same unknown owner |
| 64 two-slot cyclic records | Eight BYTES responses at real replay reservation | Same unknown owner |
| 1,040 nodes, 514 Modules; peers leave after eight allocation refusals | Persistent malloc refusal until retained callback | Real retained cleanup must release the dependency |

Replay cases move a real strong edge between mapped slots under actual write
guards, without creating or dropping references. Real confirmation must reject
the stale fields. The native wrapper then refuses the production packet queue
reservation until eight actual nonzero Fast responses occur. Every refusal must
preserve all snapshot roots and strong-reference counts. The same stale snapshot
must await packet replay; after the real packet commit, fresh discovery and claim
must physically free the graph beside admitted peers. Queue accounting balances.

The callback observer directly requires nonzero work after an actual selected
BUSY or replay refusal, even with the retained FIFO ready while guests remain
active. Three private omission controls remove propagation and must fail that predicate for vector
pressure, FULL and BYTES. A fourth omission makes propagation unconditional and
must fail the quiescent retained-progress check. Existing allocation controls
remain. The combined driver covers nine positive scenarios in C/JIT and both
poll modes: 36 applications and eight semantic omission controls. Build errors,
signals, sanitizer reports and process timeouts never count as successful controls.

In the three active-peer profiles, after supported graph and native metadata
physically free, the unknown owner must still have its sole reference and root
and must not have run its callback.
Only then may the peers leave. With the parent inactive and peers blocked on
their real completion channel, the actual retained collector invokes the unknown
callback under stopped admission and physically frees its object. Its Type creator
must become the sole descriptor owner. This verifies that supported work makes
progress independently without silently authorizing arbitrary shared finalizers.

The quiescent-assistance profile deliberately keeps vector allocation unavailable
until the real unknown callback runs. After eight actual allocation refusals, the
parent releases the peers while keeping its own native frame inactive. Retained
cleanup must then run before the blocked graph completes. Its real callback
releases the injected resource dependency; the same snapshot then claims and
frees through shared Fast. This preserves retained liveness at quiescence instead
of replacing the original bug with indefinite shared retry.

## RU: Приёмка настоящей смешанной очереди

В FIFO находится настоящий последний владелец с неизвестным деструктором.
Рядом проверяются BUSY большого native-вектора и FULL/BYTES возврата снимка.
Устаревание создаётся настоящим переносом поля под guards; ответы подтверждения,
очереди и GC не подменяются. После восьми отказов поддержанный граф физически
освобождается при работающих соседях, а неизвестный владелец остаётся укоренённым.
Дополнительный сценарий удерживает нехватку памяти до настоящего callback:
после ухода соседей резервный путь обязан снять эту зависимость, затем завершится
shared-граф. Отдельно проверяются настоящий остановленный callback,
физический free и освобождение владельца Type. Неизвестные финализаторы не
объявляются поддержанными shared-путём.

## Validation / Проверки

The default acceptance set passed 97 selected runtime CTest checks and all eight
native-claim/retry omission controls. Full-runtime ThreadSanitizer runs passed
720 fixture applications (20 repetitions of 36 profiles), 108 module-cycle
applications and 36 Duke applications: 864 VM executions without sanitizer
warnings. These include 1,280 injected FULL/BYTES replay refusals and 320 real
unknown-callback cleanups after guest release. Changed handwritten sources remain
at most 352 lines. The separate large-replay probe below is not part of that
passing set. That failure was subsequently addressed by
[Duke rooted candidate consolidation](DUKE-ROOTED-CANDIDATES.md).

## Larger replay watchdog / Watchdog большого возврата

Before the subsequent Duke consolidation, the 1,040-node two-slot replay variant exceeded its internal ten-second
watchdog under TSan, including the retained manual probe on this version. Observed
states retained 40 or 200 roots and an active Duke candidate after thousands of
passes; allocation-root safety did not establish bounded completion. That large replay stress is not fixed by this signal patch.
The queue protocol cases use 64 nodes to isolate reservation/result behavior.
Existing 1,040-node ownership graphs remain in the default matrix. The larger
replay variant is retained explicitly as `--large-replay` on the driver and is
now also included in the default acceptance set after the Duke fix. A watchdog
failure remains a failing positive test, never an accepted omission result. For example:

```sh
python3 disVM/runtime/test_fast_claim_storage.py \
  build/disvm-tsan/disvm build/disvm-tsan/bootstrap/limbo-bootstrap \
  disVM/upstream/module --large-replay
```

Начальный двухполейный replay на 1,040 узлах превышал внутренний watchdog под
TSan при оставшихся живых корнях и кандидате Duke. Последующее
[исправление Duke](DUKE-ROOTED-CANDIDATES.md) устраняет эту волну кандидатов
и возвращает оба больших replay-сценария в основную приёмку.
Отдельный режим `--large-replay` сохраняет воспроизведение. Протокол очереди
изолирован на 64 узлах; прежние большие графы владения остаются в основном наборе.

## Remaining scope / Оставшиеся ограничения

General stopped tracing, unsupported native callbacks, hard snapshot limits and
stopped shutdown remain. The FULL/BYTES cases inject refusals at real reservation
calls; they are protocol fault tests, not queue-capacity benchmarks. Execution
targets Linux x86-64. Generated JIT instructions are outside C ThreadSanitizer
instrumentation; ARM64/RV64 execution is not established here.
