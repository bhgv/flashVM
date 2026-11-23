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

## Имя, Qid и fid

При `Twalk` сервер сопоставляет имя в каталоге с устойчивым идентификатором
узла. В примере этот идентификатор хранится в `Qid.path`. В `Topen` сервер
передаёт идентификатор и режим доступа колбэку, а полученный `cookie` закрепляет
за текущим fid. `Tread` и `Twrite` получают этот `cookie`, смещение и размер.
Два открытия одного узла дают отдельное состояние открытия и могут обращаться
к одной переменной. `Tclunk` или обрыв соединения освобождает состояние один раз.

В примере сервер перечисляет файлы устройства при публикации дерева. Для
каталогов, состав которых меняется, диспетчер должен вызывать перечисление при
обходе дерева и обеспечивать стабильный порядок для открытого каталога.

## Пример

[Устройство на C](examples/device.c) публикует `/mounted/device.dis`.
Профиль создаётся из описателя `sample`, а необязательное совпадающее объявление
Limbo показывает [клиент](examples/disvm/client.b). Клиент делает обычный
`load`, дважды вызывает функцию и читает и пишет виртуальный файл `value`.
[Сценарий проверки](examples/disvm/run.py) соединяет отдельные процессы через
сокет и запускает DisVM с `--styx-mount-fd FD --raw --at /device`.
Флаг `--jit` проверяет тот же сценарий в JIT.

## Что ещё нужно для общего интерфейса устройств

- Генерация читаемого `.m`, вложенные графы с `ref`, именованные запуски с обратным контекстом и отмена запуска.
- Перечисление и поиск в меняющихся каталогах при обходе Styx.
- Типизированные переменные и права чтения/записи, прокси открытых `.fd/<n>`
  с режимом исходного FD, каналы с `send`, `recv`, `alt` и семантикой
  синхронизации. У каждого предоставленного ресурса свой срок жизни.
- Согласование возможностей соединения, контексты `.prog/<pid>` и
  `.self`, отзыв доступа, лимиты и диагностика по запросу. Полезно определить,
  как выдавать уведомления об изменениях и завершении через существующие
  sysevents.

## Исходники

[Вызовы и профиль](include/styxrpc/rpc.h) · [Виртуальная ФС](include/styxrpc/vfs.h) ·
[Происхождение форматов](SOURCES.jsonl)
