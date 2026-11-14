# Снимки цепочки вызовов от владельца потока

Copyright (c) 2026 bhgv

[English](OWNER-STACK.md) · [Рабочий пример](../examples/resident-stack/README.ru.md)

На завершённой границе исполнения владелец публикует ограниченную копию цепочки
Dis-вызовов вместе с существующим location. Максимум восемь кадров, от текущего
к вызывающим. Нулевой кадр содержит следующую original-Dis инструкцию; остальные —
сохранённые позиции возврата, а не вычисленные call-site или номера строк.
Адреса кадров не выходят из функции снятия.

## Владение и согласованность

Постоянный OS-владелец читает только свои живые TLS module/FP и rooted frames,
пока удерживает active managed claim. Используются нормальная завершённая граница
и установленный обработчик исключения. Начало нового claimed turn очищает рабочую
копию. Чужой поток не читает стек; idle mailbox service читает только native copies.
Нет global stop/exclusive, принудительного pthread suspend, миграции Prog либо
возобновления из сохранённого образа регистров.

Local call наследует module; nonnil saved `mr` обозначает переход между модулями.
Headers копируются через memcpy без предположения о выравнивании. Читаются максимум
восемь headers, распознаются повторные посещения кадров, неверные sentinel/overflow
и недоступные return coordinates. Это проверки доверенных живых metadata своего
владельца, а не валидация произвольных native pointers или защита от повреждённой
native-памяти. Stack-extent headers и Type maps не выдаются.

Каждый кадр имеет ту же насыщаемую owner boundary sequence, что и top location.
Готовый ticket неизменяем при возврате, освобождении экземпляра модуля и завершении
потока. Native records не содержат frame/module pointers, managed references или
GC roots. Reaping может удалить ticket между отдельными projections: частичный
результат нужно отбросить при любом non-Ready. Ticket не удерживает гостя.

Цена ограничена, но не равна нулю: копирование происходит на каждой завершённой
границе, а фиксированные native records mailbox и публикация потока стали больше.
Сохраняются восемь credits на target и отсутствие native allocation на каждый
запрос. Полный учёт памяти и global/client quotas — отдельные задачи.

## Проекции Processctl

Все три метода читают ответ без поглощения credit исходного requester:

| Метод | Результат |
| --- | --- |
| residentstack(id, ticket) | status, boundary sequence, число кадров, stack flags |
| residentframe(id, ticket, frame) | status, sequence, next/return PC, instruction extent, длины name/path, location flags |
| residentframebyte(id, ticket, frame, part, index) | status, unsigned UTF-8 byte; part 0=name, 1=path |

Число кадров — 0..8. StackComplete=1: достигнут корень. StackTruncated=2: за восемью
кадрами есть ещё вызывающие функции. StackUnavailable=4: корректный prefix заканчивается
недоступной позицией либо неверным/повторным trusted link. Публикуется ровно одна
классификация. Все нулевые sequence/flags/count означают отсутствие снятой границы.
Ready описывает готовность ticket, а не полноту стека.

Неверные index/part/copied geometry дают DiagInvalid; non-Ready payload нулевой.
Чужой requester получает DiagDenied. Обычный scalar residentreply поглощает Ready/
Closed; residentforget освобождает запрос. Peek сохраняет даже Closed credit до
consume/forget/reaping. Pending внутри native callback не прерывает этот callback.
Имена/пути сохраняют bounded UTF-8/LocationValid/truncation semantics; это подписи,
не уникальные идентификаторы модуля.

## Limbo и проверки

`module/residentstack.m` / `debug/residentstack.b` предоставляют
`inspect(id, attempts) -> (status, ref Stack)`. Attempts1..1000 ограничивает polling,
не общую latency. Библиотека копирует все кадры и подписи в данные своего caller,
забывает ticket при normal/Pending/managed-error exits и повторно выбрасывает
managed errors. Частичные результаты отбрасываются; locals допускают concurrent
calls shared-library. Если cleanup не может исполниться, requester/target retirement
удаляет bounded native credits. Искусственная managed error — не тест реального OOM.

Fixture проверяет imported Unicode module, local recursion, shallow complete chain,
depth80 с ростом stack, ограниченное усечение, frozen frames после return/drop,
поздний caller-only capture, requester isolation, bounds, scalar consumption и
final provider release. Пример и reader-error/Pending cleanup работают в C/JIT с
fast/locked polling; imported fixture дополнительно запускается с quantum1.
Native units проверяют cycles, local/imported links, unaligned headers, malformed
return и предел глубины. Два semantic omissions убирают publication или искажают
caller label; требуются точный DETECTED, normal exit и полный native/GC drain.
Crash/timeout/sanitizer не считаются успехом.

Содержимое typed variables, source symbols, уникальный module catalog,
pause/resume/step и external debugger capabilities/transport здесь ещё не реализованы.
Это bounded Dis call-chain snapshot, не native C stack и не полный debugger.

Итоговая затронутая выборка: 11/11 групп обычной сборки и 11/11 ThreadSanitizer,
включая control/registry/mailbox/location, новый stack workload, отмену, retirement,
таблицы и ошибки. Явно проверен успешный production extent growth provider.
Обязательный architecture list содержит59 workloads (список проверен); полная
новая архитектурная матрица этой выборкой не объявляется пройденной.
