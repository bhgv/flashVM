# [GC:] Retry completed graph publication after metadata refusal

Copyright (c) 2026 bhgv

## EN: Publication ownership

`defer_record_group` previously classified failed publication metadata allocation
as a hard refusal. Duke abandoned its already completed discovery, retiring pins
and discovering the graph again. Small Cycles candidates could also leave the
shared retry path despite otherwise supported graph contents.

Failure of the entry, owner packet, copied snapshot or prepared root batch now
sets the existing transient retry output. Every successful temporary allocation
is freed and the leaf queue reservation cancelled before returning. No graph
reference is retained, dropped or transferred on this failure path, and no root
batch is published. Cycles keeps its source for another guarded scan. Duke uses
its existing publication-pending protocol: the complete discovery and every pin
remain intact while only publication preparation is retried.

The retirement epoch is requested only after all publication allocations succeed.
Failed preparation therefore consumes no epoch for this publication. A zero
epoch with successful allocations still represents hard epoch exhaustion;
arithmetic/geometry failures and unsupported graph cases are not converted into
allocation retries. Persistent allocation refusal does not promise eventual
reclamation or remove the existing snapshot size bound.

## RU: Владение при отказе публикации

Готовый обход Duke теперь сохраняется при нехватке памяти для передачи в Fast.
Успешные временные native-выделения освобождаются, резерв очереди отменяется,
ссылки и корни объектов остаются прежними. Повторяется подготовка публикации,
а не само обнаружение графа. Малый кандидат Cycles остаётся для нового обхода
под guards. Неудачная подготовка не продвигает эпоху своей публикации.

Нулевая эпоха при успешно выделенных метаданных по-прежнему означает жёсткий
отказ. Предел размера снимка, неизвестные финализаторы и общий остановленный
обход этой серией не устраняются.

## EN: Actual runtime regression

`test_duke_publication_retry.inc` extends the real resident fixture from
[Duke pin retirement](DUKE-PIN-RETIREMENT.md) and
[snapshot allocation retry](DUKE-ALLOCATION-RETRY.md). It observes the completed
final source graph after the detached ring freed and the independent source
owner left. Two real native resident peers remain admitted throughout.

Five positive scenarios refuse these actual allocation positions, one per case:

| Position | Real allocation |
| --- | --- |
| 1 | Leaf retirement entry, `malloc` |
| 2 | Owner packet, `malloc` |
| 3 | Frozen snapshot, `malloc` |
| 4 | Root batch descriptor, `calloc` |
| 5 | Root batch entries, `calloc` |

Each case rejects eight allocations until eight actual Duke pass responses occur.
Wrappers forward real allocator/free/cancel/epoch operations outside the selected
fault. They do not fake collector responses, queue results or managed allocations.
Every successful temporary buffer must have reached real `free` before the actual
reservation cancellation. After refusal, the real scan must return continuation;
all 2,302 remaining managed objects must still have exactly their previous root
and strong-reference counts. Complete discovery must occur once, not once per
retry. The final publication uses the real epoch and queue commit.

The original fixture still requires all 2,366 target allocations to physically
free exactly once by allocation generation, with admitted peers and no stopped
Fast admission while targets remain. Every positive case runs in C and JIT with
both poll modes. The combined driver now covers nine scenarios: 36 applications.

Five additional private omission controls remove metadata retry, publication-
pending state, reservation cancellation, temporary packet free, or allocation-
before-epoch ordering. Each must fail its specific semantic predicate. The
combined driver requires thirteen semantic failures; signals, compiler errors,
sanitizer reports and process timeouts never count as successful controls.

## RU: Проверки

Проверяются пять настоящих мест выделения метаданных, по восемь отказов в каждом
сценарии. Тест требует освобождения временных буферов до отмены резерва,
сохранения корней и ссылок, одного завершённого обхода и настоящей итоговой
публикации. Затем все объекты физически освобождаются при работающих соседях.
Пять новых намеренно сломанных вариантов проверяют каждую границу отката.

## Validation / Проверки

The final series passed 96 selected runtime CTest checks and three standalone
host collector checks. Full-runtime ThreadSanitizer runs passed 360 Duke
applications (10 repetitions of 36 profiles), 108 module-cycle applications and
84 ownership/retirement applications: 552 VM executions. Five metadata scenarios
account for 1,600 actual allocation refusals in those repeated runs. The separate
native readiness binary passed 100 instrumented executions, each covering both
poll modes and real retained cleanup after join. No sanitizer warnings appeared.
Changed handwritten source files remain at most 475 lines. These are scoped
checks, not a claim that the entire repository test suite passes.

## Remaining scope / Оставшиеся ограничения

General stopped tracing, unsupported native finalizers, hard snapshot limits and
stopped shutdown remain. Runtime validation targets Linux x86-64; generated JIT
instructions are outside C ThreadSanitizer instrumentation. ARM64/RV64 execution
is not established here. This fixture exercises completed Duke publication;
small Cycles allocation-refusal behavior is established by the shared retry path,
not by a separate small-graph allocation-fault scenario.

[Fast readiness retry](FAST-READINESS-RETRY.md) records the separate dispatch
race discovered during repeated validation and its deterministic native test.
