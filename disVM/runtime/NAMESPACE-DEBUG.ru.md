# Исследование namespace по запросу

Независимый native debugger читает namespace удержанного guest. Выполнить
`pause PID`, дождаться retained=true в `status PID`, либо использовать retained
first-chance fault. Исходный continuation, group и resources живы до явного
release. Запрос не исполняет guest code и не возобновляет поток. Другие owners
продолжают работу; shared group может меняться.

Используется существующий session envelope private socket/pipe debugger.
Обычные stdin/stdout и guest channels не заняты диагностикой. VM не требует
установленных библиотек/debugger; Python driver — только средство разработки.

| Команда | Результат |
| --- | --- |
| `namespace PID` | Revision, число bindings, refs группы и held flags root/cwd/import. |
| `namespace-read PID REVISION INDEX FIELD OFFSET` | До128 bytes text field при точной revision, в hex. |

Revision — decimal JSON string без потери64 bits в client. Ответ содержит полную
byte length, copied count и binding-held flag. Offset=length допустим и даёт ноль
bytes; больший offset, неверные indexes/fields, integer overflow, знаки и лишние
tokens отклоняются. Ответ укладывается в4096 bytes. Текст не обрезается молча:
читать offsets до полной length. Chunk может разрезать UTF-8 character; сначала
соединить bytes, затем декодировать. Hex сохраняет quotes/control characters.

| FIELD | Текст | INDEX |
| --- | --- | --- |
|0| Captured native root display spelling |0|
|1| Virtual cwd |0|
|2| Virtual import directory |0|
|3| Captured source удержанного cwd |0|
|4| Captured source import directory |0|
|5| Virtual target binding |0 .. bindings-1|
|6| Captured native source binding |тот же index|
|7| Component: пусто для каталога, basename для обычного файла |тот же index|

Порядок bindings относится только к текущей revision данной группы. FORKNS не
обязан сохранять порядок списка. Искать target через field5 по всем indexes.
Любая mutation делает старую enumeration недействительной, даже если выбранный
text не менялся. При `namespace changed` отбросить pages и начать с summary.
Каждый ответ копируется под mutex группы. Постоянного snapshot cache/каталога
статистики, allocation, filesystem I/O и GC traversal нет. Подсчёт bindings и
поиск index обходят существующий список под mutex: lock-free/constant-time не
заявляется. References — owners в этот момент, включая startup controller;
spawn/retirement могут менять их без mutation revision. Это не per-thread
ownership и не учёт memory bytes.

Для running target возвращается `not retained`. Existing wake/native gate
удерживает source, затем читается только native state группы. Heap inspector не
подключается; переданный client адрес не разыменовывается. Shared peers могут
менять группу между запросами — revision защищает от смешения. Global stop нет.

Display sources после host rename могут устареть. Held flags показывают владение
capability, не доступность display path или неизменность child file. FD/env/
credentials, полные Chan/provider graphs, union mounts и provider diagnostics
остаются работой. Non-Linux runtime/debugger transports не перенесены. Workload
добавлен в mandatory architecture CI; местная приёмка — Linux x64.

[Native contract](vfs/observation.h), [реализация](vfs/observation.c),
[протокол](debugger/namespace.c),
[пример](../examples/namespace-debug/README.ru.md), [English](NAMESPACE-DEBUG.md).

Приёмка:13 целевых обычных и14 TSan CTest-групп — namespace/imports, anchors,
retained debugger/mailbox, owner stack/location, memory statistics, GC requests,
ошибки и load cancellation. Новый driver включает два настоящих mixed-revision
omission controls в обеих сборках. Native core с concurrent observations прошёл
ASan/UBSan при ASAN_OPTIONS=detect_leaks=0; LSan и whole-runtime sanitizer не
заявляются. Новые исходники меньше500 строк, local doc links проверены.
