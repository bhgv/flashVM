# libstyxrpc

Copyright (c) 2026 bhgv

[English](README.md)

Переносимый слой удалённых вызовов поверх Styx и S-записей UniBus. Устройство
регистрирует C-функции и публикует их как виртуальный модуль. Клиент с DisVM
может загрузить его обычным `load`. C-клиент может сформировать запрос и разобрать
ответ тем же кодеком, использовав своё Styx-соединение.

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

Штатный load Dis через прямой Styx поддерживает позиционный формат графа значений. В примере изменяется вложенный объект с сохранением его идентичности.

[`styxrpc_graph_profile`](reference/graph_profile.ru.md) · [`styxrpc_graph_request`](reference/graph_request.ru.md) · [`styxrpc_graph_arguments`](reference/graph_arguments.ru.md) · [`styxrpc_graph_reply`](reference/graph_reply.ru.md) · [`styxrpc_graph_read`](reference/graph_read.ru.md)

## Кадры каналов UniBus

Dis передаёт живые каналы C-экспорту. C выбирает ветки через встречный `.prog/<pid>/.unibus` и может принять вложенный канал в пределах того же вызова. Выданный C-канал сохраняет аренду до закрытия fid функции. Переносимый планировщик C обслуживает FIFO и синхронные каналы, выбирает ветки и снимает ожидание при clunk.

[`styxrpc_channel_request`](reference/channel_request.ru.md) · [`styxrpc_channel_read`](reference/channel_read.ru.md) · [`styxrpc_channel_reply`](reference/channel_reply.ru.md) · [`styxrpc_channel_result`](reference/channel_result.ru.md) · [`styxrpc_channel_fid_write`](reference/channel_fid_write.ru.md) · [`styxrpc_channel_fid_finish`](reference/channel_fid_finish.ru.md) · [`styxrpc_channel_fid_read`](reference/channel_fid_read.ru.md) · [`styxrpc_channel_fid_close`](reference/channel_fid_close.ru.md)

[`styxrpc_channel_profile`](reference/channel_profile.ru.md) · [`styxrpc_channel_call`](reference/channel_call.ru.md) · [`styxrpc_channel_call_reply`](reference/channel_call_reply.ru.md) · [`styxrpc_channel_export_value`](reference/channel_export_value.ru.md) · [`styxrpc_channel_exchange`](reference/channel_exchange.ru.md)

[`styxrpc_channel_import_element`](reference/channel_import_element.ru.md) · [`styxrpc_scheduler_init`](reference/scheduler_init.ru.md) · [`styxrpc_scheduler_admit`](reference/scheduler_admit.ru.md) · [`styxrpc_scheduler_cancel`](reference/scheduler_cancel.ru.md) · [`styxrpc_scheduler_progress`](reference/scheduler_progress.ru.md)

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

## Делегированные каналы

Вложенный канал может использовать ключ текущей шины, отдельный встречный PID
или токен модуля. Приложение привязывает полномочие к исходному соединению
Styx. Отправитель владеет временем жизни выданного ресурса. На стороне C
колбэк проверяет вложенные адреса в типизированных сообщениях, а адаптер
открывает отдельный fid на точном `.unibus`. Опубликованный PID нельзя
использовать повторно в той же сессии. Токен модуля действует столько, сколько
его удерживает открытый fid, выдавший канал.

[`styxrpc_channel_element_routes`](reference/channel_element_routes.ru.md) · [`styxrpc_channel_export_routes`](reference/channel_export_routes.ru.md) · [`styxrpc_channel_route_value`](reference/channel_route_value.ru.md) · [`styxrpc_channel_read_routed`](reference/channel_read_routed.ru.md) · [`styxrpc_channel_route_exchange`](reference/channel_route_exchange.ru.md)

## Исходники

[Вызовы и профиль](include/styxrpc/rpc.h) · [Виртуальная ФС](include/styxrpc/vfs.h) ·
[Происхождение форматов](SOURCES.jsonl)
