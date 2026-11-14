# Текущая статистика native арены

Copyright (c) 2026 bhgv

[English](MEMORY-STATISTICS.md) · [Рабочий пример](../examples/memory-statistics/README.ru.md)

Processctl.memorysnapshot возвращает один согласованный набор counters managed
арены hosted VM. Чтение безопасно при параллельных allocate/free: берётся существующий
mutex метаданных аллокатора и копируются counters за O(1). Нет обхода объектов,
чтения Type/REG/stack, новых managed references либо остановки других owners.
Изменения аллокатора уже используют тот же mutex: новые callbacks/замки не добавлены.
Статистика не даёт права доступа к памяти.

## Область учёта и единицы

| Поле | Значение |
| --- | --- |
| arena | Полная calloc арена; не physical committed/resident bytes и не process RSS |
| header | Заголовок арены вне раздела блоков |
| live / free | Занятые/свободные spans с native overhead и alignment slack |
| liveblocks / freeblocks | Занятые блоки/свободные extents, включая unpublished buffers |
| overhead | Native header, allocation generation и tail на блок, без alignment slack |
| allocations / releases | Успешные pool allocations/frees с инициализации pool |
| peak | Максимум одновременно занятых block spans |
| flags | History increment ограничен signed-big максимумом |

Точный баланс: `arena = header + live + free`. Текущие gauges ограничены размером
арены и не насыщаются. Без flags: `allocations - releases = liveblocks`. История
насыщается на9223372036854775807; flag означает хотя бы один неуместившийся increment.
Peak — точный максимум за жизнь pool. Отказавшие allocations/frees и попытка destroy
не меняют counters. Успешный destroy требует joined users/отсутствия live blocks
и очищает accounting.

`livecapacity = live - liveblocks*overhead`, аналогично для free. Capacity всё ещё
включает alignment slack и managed Heap headers. Это не requested payload,
reachable application data, per-type bytes или число опубликованных переменных.
Аллокатор не объединяет соседние free blocks. Суммарная free capacity не равна
крупнейшему свободному extent и не гарантирует одно столь же большое выделение.

Исключены native malloc metadata вне pool, выделенные отдельно code/Type/modules,
OS stacks, files/sockets, diagnostic buffers, origin/retainer attribution и RSS.
Логическое освобождение может предшествовать deferred RC/GC reclamation; free
counters меняются только при настоящем poolfree. Каталог объектов, origin IDs,
per-thread/per-type attribution этим API не реализуются.

## API и публикация

`Processctl.memorysnapshot()` возвращает:

```
(status, arena, header, live, free, liveblocks, freeblocks,
 overhead, allocations, releases, peak, flags)
```

Status1 означает coherent record. Status-1 — недоступный/несогласованный native
accounting; все payload fields нулевые. Native функция тоже очищает output при
invalid/uninitialized state. Caller удерживает lifetime pool/mutex до возврата;
mutex не защищает собственную жизнь при concurrent pool destruction.

C boundary сначала снимает native record, затем публикует чисто scalar96-byte
NC64 result через существующий shared range-guard domain. Лимит scalar adapter
расширен с64 до96 bytes; нет raw store либо раздельной публикации. Busy повторяет
только publication, не чтение counters. Другой вызов возвращает другой instant:
нельзя смешивать поля отдельных snapshots в один баланс. Public Dis64/ISA/Sys IDs
и режим исполнения не меняются.

`module/memoryview.m` и `debug/memoryview.b` дают
`inspect() -> (status, ref Stats)` с именованными полями и capacities. Данные
принадлежат caller; mutable globals нет. Выделение возвращаемого ADT идёт после
снятого instant и в этот же sample не входит. Managed allocation errors выходят
наружу; удерживаемых tickets/native requests для cleanup нет. Native refusal даёт
nil, а не частичное представление.

## Проверки

Private native unit использует восемь настоящих OS threads и3200 суммарных
allocation/write/size/free rounds. Независимый oracle складывает actual block
headers под pool lock и сравнивает cached counters того же instant. Проверяются
whole-block allocation без free remainder, неизменность при refused allocation/
destroy, zero failed output, signed-big saturation и joined quiescent destruction.
Oracle не читает managed objects и не вычисляет свои суммы из проверяемых counters.

VM fixture запускает четыре allocating guests и непрерывного shared-result observer.
Двести snapshots проверяют partition/history и monotonic traffic; ordinary payloads
и child joins обязаны завершиться. Соседние canaries защищают96-byte tuple.
Limbo facade, пример и явный unavailable/zero-output path работают в C/JIT с
fast/locked polling; main fixture также quantum1. Два copied-output controls
искажают free-span или allocation-history projection: нужны точный DETECTED,
normal exit и полный native/GC drain. Это projection controls, не инъекция пропуска
обновления allocator-accounting counters.

Итоговая выборка: 14/14 групп обычной сборки и14/14 ThreadSanitizer, включая
диагностику, отмену, retirement, таблицы, packet charging и fast claims. Прежний
actual-pool тест16000 allocations и три его controls тоже проходят. Обязательный
architecture list содержит60 workloads (список проверен); полная новая
архитектурная матрица — отдельный результат CI.
