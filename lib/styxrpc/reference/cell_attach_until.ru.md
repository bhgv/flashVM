# styxrpc_cell_attach_until

[English](cell_attach_until.md) · [Ячейки](named_cell.ru.md) · [Объявление](../include/styxrpc/named_cell.h#L74) · [Реализация](../src/named_channel_client.c#L283)

```c
#include <styxrpc/named_cell.h>

StyxRpcIoOutcome styxrpc_cell_attach_until(
    struct StyxClient *session, const void *token, StyxRpcVfsFile **out,
    StyxRpcNamedStop stop, void *state);
```

Присоединиться по заимствованному `token` длиной
`STYXRPC_CELL_TOKEN` байтов в выдавшей `session`. До возврата удерживайте
ссылку переданного дескриптора либо другую аренду этой ячейки. При
`STYXRPC_IO_OK` в `out` передаётся новый fid чтения и записи. При
отказе выход не меняется. Новый fid удерживает ту же ячейку вместе
с остальными владельцами. Освободите его через
[styxrpc_vfs_close](vfs_close.ru.md). Чужой или отозванный жетон
отклоняется. Завершите присоединение до следующего обращения к каналу,
которое может заменить переданную ссылку.

Подключите хостовую цель C `StyxRpc::launch`. Необязательный
`stop(state)` вызывается в потоке операции и возвращает ненулевое
значение для остановки. Удерживайте `state` до возврата и исключите
повторный вход в сессию из колбэка. NULL оставляет обычное ожидание.
В мультиплексированной сессии ожидающий запрос можно отменить по тегу.
Освобождение временного fid может продолжиться после возврата.
Последовательная сессия проверяет флаг между обменами.

`STYXRPC_IO_NOT_OPEN` означает неверный аргумент или отсутствие службы.
`STYXRPC_IO_FAILED` означает определённый отказ или неверный ответ.
`STYXRPC_IO_CANCELLED` сообщает остановку вызывающим. Обмен без
надёжного подтверждения может вернуть `STYXRPC_IO_UNCERTAIN`.

```c
StyxRpcVfsFile *alias = NULL;
StyxRpcIoOutcome result = styxrpc_cell_attach_until(session, token, &alias,
    stop_requested, task);
```

В примере `stop_requested(task)` читает флаг вызывающего.
Возьмите жетон полученной ячейки в том же подключении. После
успеха используйте [чтение](cell_get.ru.md) и [запись](cell_set.ru.md)
на выданной аренде либо удерживайте её во время временного I/O по жетону.
