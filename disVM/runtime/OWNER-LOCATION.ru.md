# Снимки location владельца

Copyright (c) 2026 bhgv

[English](OWNER-LOCATION.md) · [Запускаемый пример](../examples/resident-debug/README.ru.md)

Resident копирует имя/путь текущего модуля и **следующий original Dis PC**, пока
permanent owner ещё удерживает active managed claim. Ticket хранит только копию,
без удержания Module, Modlink, stack либо managed pointer. Готовый reply не меняется
при дальнейшей работе, возврате из imported module и окончательном освобождении модуля.

Это диагностическая подпись завершённой execution boundary. Она не является stack
trace, глобальным ID Module, native instruction pointer, source line, breakpoint
либо разрешением читать память. Next PC может равняться числу инструкций на границе
конца. Для source mapping нужны соответствующие module/debug metadata; имя и путь
сами по себе не устанавливают уникальность экземпляра модуля.

## Capture и lifetime

`diagnostics/location.c` читает только rooted Modlink/Module своего active owner
и его live TLS PC. После normal xec и установки exception handler публикуется
native copy до освобождения claim. Начало нового claimed turn очищает старую
рабочую запись. Idle diagnostic service копирует последнюю публикацию, не читая
чужие registers и не обходя heap.

Integer spans отказывают посторонним, невыровненным и native-PC координатам.
Original Dis offsets допустимы от нуля до nprog включительно. Текущий leaf JIT
сохраняет эти координаты. Legacy compiled-модули с native PC дают unavailable.
Code extent, имя и путь должны быть immutable при удерживаемом lifetime. Функция
не валидирует произвольные C pointers либо изменяемые module data.

Native storage: 64 bytes для имени и 256 для пути, включая NUL. Копируется максимум
63/255 bytes. Truncation заканчивается целым UTF-8 prefix. Неверный снимаемый UTF-8
очищает всю location. Capture не выделяет память, не вызывает callbacks и не
увеличивает Module references. Работа ограничена на каждую завершённую границу;
нулевой overhead этим не обещается.

Owner-local sequence насыщается на signed-big max и относится к resident, а не
Module. Неизменяемую запись отличают original requester и exact target/ticket
в текущем VM service. Cross-VM generation authentication и внешний transport
отладчика здесь не реализованы.

## API

Ticket создаёт прежний residentrequest. Прежний scalar residentreply сохраняет
формат и поглощает complete reply. Новые projections **не поглощают** credits
ни в Pending, ни в Ready, ни в Closed:

| Processctl method | Результат |
| --- | --- |
| residentlocation(id, ticket) | status, sequence, next PC, число инструкций, длины имени/пути в bytes, flags |
| residentlocationbyte(id, ticket, part, index) | status, unsigned UTF-8 byte; part 0=имя, 1=путь |

Любой non-Ready output нулевой. Неверные part/index либо copied geometry дают
DiagInvalid. Чужой requester получает DiagDenied без bytes. LocationValid означает
доступную geometry и copied labels; truncation имени/пути имеет отдельные bits.
Ready с LocationValid=0 означает недоступную location при действительном scalar
progress reply. Освободить credit после peek нужно residentreply/residentforget.

Target может быть reaped между projections. При любом non-Ready нужно отбросить
частичный результат: ticket не удерживает target. Блокирующий native callback
может оставлять query Pending до servicing point своего owner. Эти API не
прерывают callback и не ставят VM на паузу.

## Limbo reader

`disVM/debug/residentdebug.b` с интерфейсом `module/residentdebug.m` предоставляет
`inspect(id, attempts) -> (status, ref Location)`. Он отправляет query, делает poll,
копирует обе подписи в строки caller и пытается забыть ticket на normal/timeout/
exception выходах. Искусственная managed error проверяет rethrow и полное повторное
использование quota. Pending timeout проверяет сохранение обычного native sleep.

Attempts — от 1 до 1000. Это предел числа poll, а не latency scheduler/native work.
Если VM не может выполнить сам cleanup, retirement requester либо уничтожение
target всё равно освобождает bounded native credits. У library нет изменяемых
globals: capabilities и данные каждого caller локальны его вызову.

## Приёмка и дальнейшая работа

Imported-module fixture проверяет точные Unicode labels, next-PC bounds, frozen
peek после return/отпускания instance, последующую caller boundary, requester
isolation и final provider release. Пример, managed-error и Pending cleanup
исполняются в C/JIT с обоими polling profiles. Imported caller также проходит
quantum=1. Native probes проверяют active owner/TLS, UTF-8 truncation, end-PC,
native-PC/misalignment refusal и invalid text. Два omission controls убирают
publication либо меняют copied path: точный DETECTED должен сопровождаться normal
exit и полным native/GC drain. Crash/timeout/sanitizer не являются успехом.

Workload owner-location включён в обязательный architecture list (58 workloads на момент его добавления).
Typed-variable capture, полный stack сверх bounded prefix, уникальный module catalog, pause/resume,
source symbols и внешние capabilities остаются отдельными задачами.

Итоговая выборка затронутых регрессий: 10/10 групп в обычной сборке и 10/10
под ThreadSanitizer, включая mailbox/control/registry, отмену, retirement и
владение таблицами. Это выборочная регрессия, а не утверждение о завершении
полной новой архитектурной матрицы.

Ограниченная цепочка кадров теперь реализована отдельно: [OWNER-STACK.ru.md](OWNER-STACK.ru.md).
Полный стек сверх bounded prefix и значения переменных остаются следующими задачами.
