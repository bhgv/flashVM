# [GC:] Wide mapped owners on shared Fast

Copyright (c) 2026 bhgv

## EN: Remove the record/list field ceiling

The last-owner record/list path used a fixed 512-key vector. A supported record
with more live fields, or a list whose tail plus data exceeded that vector, was
sent to `difficult` and needed retained stopped admission. Array transfer already
supported larger packets, but its temporary allocation failures could also be
classified as hard refusal.

Record/list capture now validates and counts every actual non-nil occurrence.
The first traversal stores the prefix fitting the local vector; small objects
need only that traversal. Larger objects allocate exactly the complete owner
count and repeat capture into that storage. Nil-rich maps do not allocate a
vector by mapped slot count. Repeated aliases remain repeated owners: 2,300
fields pointing at one allocation require 2,300 references/root occurrences,
followed by one physical free. A list tail is counted independently of its data.

The parent remains independently owned after its reader cutoff. Capture and
metadata setup do not consume that owner or change fields. Child roots publish
before the atomic last-reference claim; only a successful claim commits the
packet. Scratch is freed on success and refusal. A failed claim withdraws the
new roots, destroys their metadata and cancels the queue reservation. Existing
packet draining and child epochs govern later physical release. Size/charge
overflow and invalid maps still refuse without consuming managed ownership.

## RU: Широкая запись и список больше не требуют остановки из-за 512 полей

Прежний буфер из 512 ссылок отправлял широкие записи и списки в остановочный
резервный путь. Теперь учитываются все настоящие ненулевые вхождения, включая
хвост списка. Малые объекты обходятся один раз и используют стек; большим
выделяется точный вектор и выполняется повторное чтение. Nil не занимает место
в пакете, повторные ссылки сохраняют свою кратность.

До захвата последней ссылки исходный владелец и поля не меняются. Дочерние корни
публикуются раньше захвата. При отказе снимаются новые корни, резерв и рабочая
память; при успехе владельцы передаются пакету. Переполнение и неверная карта
остаются жёсткими отказами. Освобождение детей сохраняет прежние эпохи читателей.

## EN: Temporary failure stays shared

Key-vector malloc, child-packet malloc and child-root reservation failures now
request retry. The common packet helper checks count/storage arithmetic before
allocation. Arrays preserve root-allocation retry when no queue reservation was
attempted, rather than overwriting it with `INVALID`'s classification.

After published child roots, an actual competing retain can defeat the last
claim and disappear before the caller later samples the count. Record/list and
array packet rollback explicitly retain the retry signal instead of relying on
that later sample. The fixture exercises this exact order with a real independent
parent reference/root and the original atomic claim; no result is forged.

Leaf retry contributes to the callback result while guests are active even when
an unsupported retained owner is queued. At quiescence, retained cleanup can
still release a resource blocking shared allocation. Unconditional continuation
would starve that cleanup. No new world stop or global execution mutex is added.

## RU: Временный отказ не становится причиной остановки

Отказы памяти вектора, пакета и дочерних корней требуют повтора. Массив сохраняет
этот сигнал, даже если до резерва очереди дело не дошло. Проигранный захват после
публикации корней тоже требует повтора: конкурирующая ссылка может исчезнуть до
следующего чтения счётчика. Тест создаёт настоящую ссылку и проверяет этот порядок.

При работающих гостях повтор листа не теряется рядом с неизвестным финализатором.
После ухода гостей резервная очистка остаётся доступной для снятия нехватки
ресурса. Новая остановка мира или глобальный мьютекс исполнения не вводятся.

## Acceptance / Приёмка

`test_large_owner_release.py` covers 21 real scenarios, each in interpreted/JIT
execution and both polling modes: 84 positive applications. Two real Limbo peers
alternate epochs while another remains admitted. The parent cutoff must wait
for eight real Fast responses; all selected physical frees require zero roots,
zero references, the expected allocation generation and admitted peers. The
unsupported owner must remain rooted until guest release, except in the resource
assistance case, where its real stopped callback must first unblock allocation.

| Owners | Actual cases |
| --- | --- |
| Ordinary records | 0, 1, 512, 513 and 2,300 non-nil occurrences; sparse maps and 2,300 aliases |
| Lists | 512 data owners plus a real tail; root OOM; actual claim race |
| Arrays | 2,300 owners; packet/root OOM; FULL/BYTES; actual claim race |
| Temporary pressure | Eight real Fast responses per injected malloc/root/FULL/BYTES refusal |
| Quiescent resource assistance | Packet malloc remains unavailable until the real unknown callback runs |

Eleven private semantic omission controls cover the old ceiling, memory/refusal
classification, callback propagation, rollback and quiescent assistance. Build
errors, signals, sanitizer reports and process timeouts do not pass a control.
All diagnostic hooks only observe production boundaries; injected failures do
not fabricate successful allocations, roots, claims or collector responses.

The final tree passed 98 selected runtime CTest checks. Full-runtime TSan passed
five repetitions of the 84-profile matrix (420 applications), followed by 44
claim-storage, 108 module-cycle and 36 Duke applications: 608 positive VM runs
without sanitizer warnings. All eleven new semantic omission controls passed.
Changed handwritten sources remain at most 357 lines; upstream is unchanged.
These are scoped results, not a claim that the entire repository suite passes.

[Childless leaf claims](CHILDLESS-LEAF-CLAIMS.md) subsequently closes the three
empty-owner claim returns and preliminary count sampling. The current matrix
includes 124 positive applications and 21 semantic omissions, including ten
actual short-lived retain races.

## Remaining scope / Оставшиеся ограничения

This removes one concrete stopped fallback for supported last-owner objects.
General stopped tracing, unsupported callbacks, hard graph limits and stopped
shutdown remain. A large owner can require a correspondingly large native
packet/allocation; the change does not promise size-independent pause latency.
Validation executes Linux x86-64. C ThreadSanitizer does not instrument emitted
JIT instructions; ARM64/RV64 execution is not established here.
