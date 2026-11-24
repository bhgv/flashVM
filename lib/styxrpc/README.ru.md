# libstyxrpc

Copyright (c) 2026 bhgv

[English](README.md)

Переносимый слой удалённых вызовов поверх Styx и S-записей UniBus. Устройство
регистрирует C-функции и публикует их как виртуальный модуль. Клиент с DisVM
может загрузить его обычным `load`. C-клиент может сформировать запрос и разобрать
ответ тем же кодеком, использовав своё Styx-соединение.

[Смешанные образы модулей, типизированные импорты и файлы управления](MODULES.ru.md).

## Сценарий интеграции

Внешнее приложение, например хост FlashVM или контроллер устройства,
связывает имена и типы экспортов со своими C-колбэками. Оно публикует
модуль в дереве Styx. DisVM монтирует дерево в VFS, обычный Limbo
`load` сверяет `.interface`, затем вызов идёт через отдельный fid
функции. Переносимая часть библиотеки проверяет S-записи UniBus и
работает с буферами приложения. Хостовые адаптеры обслуживают
соединение Styx и время жизни открытых ресурсов.

В обратном направлении C-клиент открывает функции DisVM или запускает
именованные образы. Обе стороны могут одновременно обслуживать запросы
по одному двустороннему соединению. Встречное дерево `.prog/<pid>`
предоставляет переменные, дескрипторы, VFS и шину UniBus. Результаты
могут включать ссылки и живые каналы. Обычные операции Limbo, включая
`alt`, используют их в пределах выданного доступа. Закрытие fid
завершает связанное с ним состояние и отменяет ожидающие операции.

[Сквозные примеры и приёмка](examples/README.ru.md#сквозная-приёмка-disvm)

Для хостовых скалярных модулей публикуйте описатели через [styxrpc_scalar_module_init](reference/scalar_module_init.ru.md), а проверенные экспорты вызывайте через [styxrpc_scalar_invoke](reference/scalar_invoke.ru.md). Самостоятельный C-клиент и мост FlashVM используют один API вызова. FlashVM линкуется с установленным SDK. Сам SDK не зависит от VM.

[Пользовательские сообщения UniBus](reference/custom_messages.ru.md) дают регистрацию своих типов и обработчиков в существующей сессии Styx.

[Заменяемые образы модулей](reference/module_slot.ru.md) обслуживаются на том
же дереве, что и постоянные файлы управления.

## Сборка

Установите [libunibus](../libunibus/README.ru.md) и передайте путь установки
в `CMAKE_PREFIX_PATH`. Основная библиотека — C99 и `Unibus::wire`. Для ядра
достаточно памяти, выделенной приложением. При включении `STYXRPC_EXAMPLES`
сборке также нужен установленный [Styx](../styx/README.ru.md) с `Styx::phy_server`.

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/prefix
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build --prefix /path/to/prefix
```

В другом CMake-проекте используйте `find_package(StyxRpc CONFIG REQUIRED)`
и `target_link_libraries(your_target PRIVATE StyxRpc::rpc)`.

## Интерфейсы

| Синтаксис | Параметры и назначение | Результат |
| --- | --- | --- |
| [`styxrpc_profile`](reference/profile.ru.md) `(exports, count, bytes, capacity, size)` | Массив имён, сигнатур типов, колбэков и их контекстов | Содержимое файла `.interface`, совместимое с загрузчиком DisVM |
| [`styxrpc_request`](reference/request.ru.md) `(signature, argument, bytes, capacity, size)` и [`styxrpc_reply`](reference/reply.ru.md) `(signature, bytes, size, value)` | Сигнатура и буфер вызывающей стороны | S-запись запроса и проверенный ответ |
| [`styxrpc_open`](reference/open.ru.md) `(call, exported)`, [`styxrpc_write`](reference/write.ru.md) `(call, offset, bytes, size)`, [`styxrpc_read`](reference/read.ru.md) `(call, offset, bytes, capacity, size)` | Состояние открытой функции, удерживаемое на одном fid. Запрос записывается последовательно, ответ читается по смещениям | Один вызов колбэка после полного запроса, затем байты результата |
| [`styxrpc_dir_entry`](reference/dir_entry.ru.md) `(ops, context, directory, index, entry)` | Идентификатор каталога и индекс потомка | Один потомок за вызов. `NULL` из колбэка означает конец и даёт `STYXRPC_MORE` |
| [`styxrpc_file_open`](reference/file_open.ru.md) / [`styxrpc_file_read`](reference/file_read.ru.md) / [`styxrpc_file_write`](reference/file_write.ru.md) / [`styxrpc_file_close`](reference/file_close.ru.md) | Таблица колбэков, номер узла, разрешённый режим доступа и состояние конкретного открытия | Чтение и запись с подтверждённым числом байт. Закрытие освобождает состояние открытия |

`STYXRPC_OK` означает успех. `STYXRPC_MORE` при вызове функции означает неполный
запрос или ещё недоступный ответ. `STYXRPC_REMOTE_ERROR` означает, что получен
ответ с ошибкой функции. `STYXRPC_INVALID` означает недопустимую сигнатуру,
кадр, режим или аргумент API. Текущие сигнатуры колбэков — `fn(int): int` и
`fn(big): big`. Описатели и таблицы колбэков должны жить до закрытия открывших их
fid. Диспетчер сериализует операции одного fid. Колбэк функции вызывается
синхронно, внутри приёма запроса. Файл читается до результата и последующего EOF.

[Запуск примеров](examples/README.ru.md)

## Типизированные вызовы и запуск

Типизированный API принимает строки, массивы и кортежи по существующей грамматике Dis. Для плоских сигнатур колбэк отмечает изменённые `ref` маской позиций. Чтобы завершить вызов позже, `begin` возвращает `STYXRPC_MORE`. Диспетчер вызывает close при clunk или обрыве. Флаг `-D RG` сохраняет типы импортов в клиенте Limbo.

Флаг `STYXRPC_HOSTED` подключает `StyxRpc::launch`. C-клиент использует существующий **одноразовый** сервис запуска Dis с профилем raw. Доставка запроса и завершение программы подтверждаются отдельно. Переносимое ядро по-прежнему использует только `Unibus::wire`.

[`styxrpc_managed_profile`](reference/managed_profile.ru.md) · [`styxrpc_managed_open`](reference/managed_open.ru.md) · [`styxrpc_managed_write`](reference/managed_write.ru.md) · [`styxrpc_managed_finish`](reference/managed_finish.ru.md) · [`styxrpc_managed_read`](reference/managed_read.ru.md) · [`styxrpc_managed_close`](reference/managed_close.ru.md) · [`styxrpc_managed_request`](reference/managed_request.ru.md) · [`styxrpc_managed_reply`](reference/managed_reply.ru.md) · [`styxrpc_value_integer`](reference/value_integer.ru.md) · [`styxrpc_launch_start`](reference/launch_start.ru.md) · [`styxrpc_launch_status`](reference/launch_status.ru.md)

Для независимого клиента доступен [управляемый вызов](reference/managed_invoke.ru.md), для синхронных колбэков — [публикация управляемого модуля](reference/managed_module_init.ru.md). В переносимом ядре доступно [декодирование целых](reference/value_get_integer.ru.md).

[Кодирование real](reference/value_real.ru.md) и [чтение real](reference/value_get_real.ru.md) сохраняют биты binary64 в существующем формате Dis.

## Именованный запуск и динамическое дерево

Постоянный сервис DisVM запускает разные именованные образы по одному соединению Styx. Каждый запуск удерживает отдельный fid чтения и записи. C-клиент публикует встречный контекст `.prog/<pid>` через `StyxRpc::serve`. Сервер вызывает приложение при поиске и чтении каталогов. Смена ревизии останавливает чтение устаревшего каталога. Открытые файлы сохраняют свои состояния до закрытия.

[`styxrpc_launch_named`](reference/launch_named.ru.md) · [`styxrpc_launch_named_status`](reference/launch_named_status.ru.md) · [`styxrpc_launch_named_close`](reference/launch_named_close.ru.md) · [`styxrpc_server_create`](reference/server_create.ru.md) · [`styxrpc_server_step`](reference/server_step.ru.md) · [`styxrpc_server_destroy`](reference/server_destroy.ru.md) · [`styxrpc_server_state`](reference/server_state.ru.md) · [`styxintvariableread`](reference/intvariableread.ru.md) · [`styxintvariablewrite`](reference/intvariablewrite.ru.md)

## Регистрация процесса

Регистрируйте переменные, FD и VFS до запуска диспетчера или в его потоке. Массив регистрации принадлежит приложению. Колбэк открытия может выделить отдельное состояние на каждый fid.

[`styxrpc_programs_init`](reference/programs_init.ru.md) · [`styxrpc_program_add`](reference/program_add.ru.md) · [`styxrpc_program_fd`](reference/program_fd.ru.md) · [`styxrpc_program_int`](reference/program_int.ru.md) · [`styxrpc_program_variable`](reference/program_variable.ru.md) · [`styxrpc_program_file`](reference/program_file.ru.md) · [`styxrpc_program_unibus`](reference/program_unibus.ru.md)

[`styxrpc_program_collection`](reference/program_collection.ru.md) · [`styxintarrayopen`](reference/intarrayopen.ru.md) · [`styxintarrayclose`](reference/intarrayclose.ru.md) · [`styxintarrayselect`](reference/intarrayselect.ru.md) · [`styxintarrayread`](reference/intarrayread.ru.md) · [`styxintarraywrite`](reference/intarraywrite.ru.md)

## Исходник Limbo

Библиотека создаёт .m из используемой схемы и указанных имён полей ADT. Сигнатуру вычисляет компилятор Limbo.

[`styxrpc_module_source`](reference/module_source.ru.md)

## Графы вложенных ссылок

Штатный load Dis через прямой Styx использует позиционный граф для вложенных и возвращаемых ref. Самостоятельная программа на C может выполнять такие вызовы и публиковать модули через SDK. Компилированный интерфейс, позиционные кадры и разреженные изменения общие с DisVM. Независимый клиент C проверяет изменение вложенного ref через fid Styx.

[`styxrpc_graph_profile`](reference/graph_profile.ru.md) · [`styxrpc_graph_request`](reference/graph_request.ru.md) · [`styxrpc_graph_arguments`](reference/graph_arguments.ru.md) · [`styxrpc_graph_reply`](reference/graph_reply.ru.md) · [`styxrpc_graph_read`](reference/graph_read.ru.md)

[`styxrpc_graph_invoke`](reference/graph_invoke.ru.md) · [`styxrpc_graph_module_init`](reference/graph_module_init.ru.md) · [Разреженные изменения графа](reference/graph_updates.ru.md)

## Гибридные таблицы в графах RPC

Нативный тип Dis `h` переносит идентификатор таблицы с числовыми и строковыми ключами. Самостоятельные клиенты и провайдеры на C используют то же хранилище, кодек упорядоченных снимков и патчи по ключам. [Руководство и API для C](reference/hybrid_tables.ru.md).

## Кадры каналов UniBus

Dis передаёт живые каналы C-экспорту. C выбирает ветки через встречный `.prog/<pid>/.unibus` и может принять вложенный канал в пределах того же вызова. Выданный C-канал сохраняет аренду до закрытия fid функции. Переносимый планировщик C обслуживает FIFO и синхронные каналы, выбирает ветки и снимает ожидание при clunk.

[`styxrpc_channel_request`](reference/channel_request.ru.md) · [`styxrpc_channel_read`](reference/channel_read.ru.md) · [`styxrpc_channel_reply`](reference/channel_reply.ru.md) · [`styxrpc_channel_result`](reference/channel_result.ru.md) · [`styxrpc_channel_fid_write`](reference/channel_fid_write.ru.md) · [`styxrpc_channel_fid_finish`](reference/channel_fid_finish.ru.md) · [`styxrpc_channel_fid_read`](reference/channel_fid_read.ru.md) · [`styxrpc_channel_fid_close`](reference/channel_fid_close.ru.md)

[`styxrpc_channel_profile`](reference/channel_profile.ru.md) · [`styxrpc_channel_call`](reference/channel_call.ru.md) · [`styxrpc_channel_call_reply`](reference/channel_call_reply.ru.md) · [`styxrpc_channel_export_value`](reference/channel_export_value.ru.md) · [`styxrpc_channel_exchange`](reference/channel_exchange.ru.md)

[`styxrpc_channel_import_element`](reference/channel_import_element.ru.md) · [`styxrpc_scheduler_init`](reference/scheduler_init.ru.md) · [`styxrpc_scheduler_admit`](reference/scheduler_admit.ru.md) · [`styxrpc_scheduler_cancel`](reference/scheduler_cancel.ru.md) · [`styxrpc_scheduler_progress`](reference/scheduler_progress.ru.md)

Хостовый клиент также поддерживает [stat](reference/vfs_stat.ru.md), [позиционное чтение](reference/vfs_read_at.ru.md) и [позиционную запись](reference/vfs_write_at.ru.md). Эти операции используют уже смонтированную сессию, удерживают один fid и возвращают подтверждённый префикс передачи.

Используйте [open](reference/vfs_open.ru.md) для удерживаемого дескриптора с [stat](reference/vfs_file_stat.ru.md), [чтением](reference/vfs_file_read.ru.md), [записью](reference/vfs_file_write.ru.md) и [закрытием](reference/vfs_close.ru.md). [List](reference/vfs_list.ru.md) читает каталог. Внешние переменные int используют [чтение целого значения](reference/vfs_int_read.ru.md) и [запись](reference/vfs_int_write.ru.md).

## Имя, Qid и fid

При `Twalk` сервер сопоставляет имя в каталоге с устойчивым идентификатором
узла. В примере этот идентификатор хранится в `Qid.path`. В `Topen` сервер
передаёт идентификатор и режим доступа колбэку, а полученный `cookie` закрепляет
за текущим fid. `Tread` и `Twrite` получают этот `cookie`, смещение и размер.
Два открытия одного узла дают отдельное состояние открытия и могут обращаться
к одной переменной. `Tclunk` или обрыв соединения освобождает состояние один раз.

Первый пример устройства публикует постоянное дерево. Пример встречного запуска ищет и перечисляет узлы по запросу.

## Пример

[Устройство на C](examples/device.c) публикует `/mounted/device.dis`.
Профиль создаётся из описателя `sample`, а [клиент](examples/disvm/client.b) компилируется с сгенерированным `device.m`. Клиент делает обычный
`load`, дважды вызывает функцию и читает и пишет виртуальный файл `value`.
[Сценарий проверки](examples/disvm/run.py) соединяет отдельные процессы через
сокет и запускает DisVM с `--styx-mount-fd FD --raw --at /device`.
Флаг `--jit` проверяет тот же сценарий в JIT.

## Встречная таблица `table of big`

Приложение хранит позиционную и ключевую части таблицы. Один колбэк C принимает операции над обеими частями. `.idx` различает числовые и строковые ключи, `.len` возвращает длину плотной части, а `.count` учитывает и ключевую часть. Это дополняет API коллекций Int32.

[`styxrpc_program_bigtable`](reference/program_bigtable.ru.md) · [Рабочий пример](examples/README.ru.md)

## Встречные каналы как аргументы

[Области аргументов-каналов](reference/channel_argument_scopes.ru.md) временно передают локальные очереди через стабильную встречную шину процесса. Вызывающий проверяет точное предложение и ключи, активирует их на время вызова и отзывает ожидающие операции при возврате.

[Хостовые обработчики каналов](reference/channel_handler_modules.ru.md) принимают аргументы Dis в самостоятельном приложении C. Диспетчер Styx подтверждает запросы и выдаёт готовые ответы, пока колбэк выполняется на потоке приложения и использует встречную шину. Для хостовой цели требуются потоки POSIX.

## Аренда каналов в клиенте и модуле

[Хостовая аренда каналов](reference/channel_leases.ru.md) поддерживает проверенный контекстный вызов, операции на удерживаемых каналах результата и публикацию через колбэки. Самостоятельные программы на C и мост FlashVM используют эти же привязки.

## Делегированные каналы

Вложенный канал может использовать ключ текущей шины, отдельный встречный PID
или токен модуля. Приложение привязывает полномочие к исходному соединению
Styx. Отправитель владеет временем жизни выданного ресурса. На стороне C
колбэк проверяет вложенные адреса в типизированных сообщениях, а адаптер
открывает отдельный fid на точном `.unibus`. Опубликованный PID нельзя
использовать повторно в той же сессии. Токен модуля действует столько, сколько
его удерживает открытый fid, выдавший канал.

[`styxrpc_channel_element_routes`](reference/channel_element_routes.ru.md) · [`styxrpc_channel_export_routes`](reference/channel_export_routes.ru.md) · [`styxrpc_channel_route_value`](reference/channel_route_value.ru.md) · [`styxrpc_channel_read_routed`](reference/channel_read_routed.ru.md) · [`styxrpc_channel_route_exchange`](reference/channel_route_exchange.ru.md)

Диспетчер C может немедленно отозвать встречную шину. Ожидающие операции закрываются до возврата, а старые fid остаются доступны только для clunk. [`styxrpc_program_unibus_revoke`](reference/program_unibus_revoke.ru.md) Для временных шин реестр выдаёт новые PID и переиспользует слоты после отзыва. [`styxrpc_program_unibus_grant`](reference/program_unibus_grant.ru.md)

Когда приложение и встречный диспетчер работают в разных потоках, используйте [сервер программ](reference/program_server_create.ru.md). Хост может [выдать](reference/program_server_grant.ru.md) и [отозвать](reference/program_server_revoke.ru.md) PID во время работы диспетчера. [Ресурс заимствованной очереди](reference/channel_group_reverse_borrow.ru.md) даёт партнёру доступ к существующему FIFO или rendezvous с элементами по значению и после отзыва возвращает обновлённый курсор владельцу.

[Передача графа очередей](reference/channel_group_reverse_borrow_graph.ru.md) публикует несколько существующих FIFO и rendezvous под одним PID. Вложенные каналы используют ключи той же шины. Отзыв возвращает оставшиеся кадры и курсоры в исходные дескрипторы.

## Общая шина C-модуля

Хостовая библиотека позволяет нескольким открытым вызовам вернуть свои
каналы через один файл `.unibus`. Токен выбирает очередь, а fid операции
хранит её открытое состояние. Реестр обслуживает токены и отзыв. Приложение
владеет очередями, выделяет память реестра и связывает отзыв с выдавшим fid.
Для C-публикатора используйте `StyxRpc::serve`. Для чтения и записи по
полученному маршруту используйте `StyxRpc::launch`.

[`styxrpc_module_channels_init`](reference/module_channels_init.ru.md) · [`styxrpc_module_channels_publish`](reference/module_channels_publish.ru.md) · [`styxrpc_module_channels_resource`](reference/module_channels_resource.ru.md) · [`styxrpc_module_channels_revoke`](reference/module_channels_revoke.ru.md)

[Два канала через один сокет](examples/module_channels.ru.md)

## Исходники

[Вызовы и профиль](include/styxrpc/rpc.h) · [Виртуальная ФС](include/styxrpc/vfs.h) ·
[Происхождение форматов](SOURCES.jsonl)
