# [GC:] Duke snapshot allocation retries

Copyright (c) 2026 bhgv

## EN: Failure classification

Previously, a failed initial Duke snapshot allocation returned a nonbusy scan
failure. Duke routed the candidate to the retained fallback. Failure to enlarge
an existing snapshot was marked `oversized`, so the same candidate moved back
to mandatory retirement. Temporary native allocation pressure could therefore
send an otherwise supported graph toward stopped cleanup.

`gc/duke/snapshot_growth.c` now distinguishes allocation failure (`busy=1`)
from a hard byte limit (`busy=0`). Both preserve the old buffer, indices, field
cursors and discovery pins. No managed reference or root moves in this helper.
The initial scan propagates busy without withdrawing its queued source.
`dvm_rc_reserve_record_fields` preserves allocation busy instead of marking
the graph oversized. Hard capacity/byte refusal still sets oversized and clears
stale busy state, including when a request exceeds the reservation byte guard.

The existing shared cleanup protocol may release connected pins under field
guards or retire the remaining pins with their roots. A failed growing capture
is then rediscovered; this change does not resume a partially failed field scan
from its cursor. Duke retries through its existing queue/pass policy, allowing
other candidates and admitted guests to run. Persistent allocation failure does
not guarantee eventual collection, and no new unbounded buffer is introduced.

## RU: Классификация отказа

Начальное выделение и рост снимка теперь отличают временную нехватку памяти
от жёсткого предела. Временный отказ оставляет исходного кандидата в shared-
обработке и не передаёт его остановленному fallback. Старый буфер, индексы,
курсоры и pin сохраняются на границе функции роста. Далее работает обычная
очистка неудачного обхода, после которой граф обнаруживается заново.

Превышение ёмкости или лимита по-прежнему означает oversized. Оно явно снимает
устаревший busy, поэтому настоящий предел не превращается в бесконечный повтор.

## EN: Actual failure injection and regression

`test_duke_allocation_retry.inc` extends the real resident fixture described in
[Duke pin retirement](DUKE-PIN-RETIREMENT.md). A linker wrapper refuses only the
actual `calloc` executed by the selected source's snapshot growth. It does not
replace collector results, queue operations or managed storage.

Two new positive cases cover initial allocation and enlargement of an existing
snapshot. Each refuses eight actual allocations until eight real positive Duke
pass responses occur. The wrapper checks unchanged buffer identities/cursors
and the busy classification, and the real scan must return busy without moving
its source to fallback. Probes on private native descriptor copies verify that
maximal hard-limit requests preserve storage while returning the distinct hard
failure. They neither copy managed ownership nor publish those descriptors.

After the refusals, the original fixture still detaches its real branch, drains
the genuine old-edge seed, frees the 64-record ring before releasing its outside
source owner, then physically frees all 2,366 allocations with two admitted peers.
All four positive scenarios run in C/JIT with both poll modes: 16 applications.

Four additional private omission controls remove allocation busy, initial busy
propagation, growing-allocation retry handling, or hard-limit busy reset. Each
must fail its exact semantic predicate. Together with the four existing pin
controls, the driver requires eight semantic failures; compiler errors, signals,
process timeouts and sanitizer reports are never accepted as evidence.

## RU: Проверки

Настоящий calloc снимка отказывает восемь раз в двух сценариях: первое выделение
и увеличение существующего буфера. Проверяются реальные ответы Duke, сохранение
буфера/курсоров, отказ от передачи кандидата в fallback и отличимость жёсткого
предела на личных копиях native-дескриптора. После восстановления памяти тест
требует прежнего физического освобождения всех объектов при работающих соседях.
Четыре новых и четыре прежних намеренно сломанных варианта должны завершиться
конкретными семантическими ошибками.

## Validation / Проверки

The final native behavior passed 95 selected CTest checks, including the large
record cycle and all eight semantic omission controls. A full-runtime TSan build
passed 320 positive Duke applications (20 repetitions of 16 profiles) and 108
positive module-cycle applications without sanitizer warnings. The new allocation
cases account for 160 applications and 1,280 injected snapshot allocation failures.
Changed handwritten source files remain below 500 lines. This is scoped runtime
validation, not a claim that the entire repository suite passes.

## Remaining scope / Оставшиеся ограничения

The 4 MiB snapshot bound remains, as do general stopped tracing, unsupported
native finalizers and stopped shutdown. Validation here targets Linux x86-64;
generated JIT instructions are outside C ThreadSanitizer instrumentation.
ARM64/RV64 execution is not established by these tests.

[Completed publication retry](DUKE-PUBLICATION-RETRY.md) subsequently extends the
fixture to 36 positive applications and thirteen semantic omission controls.
