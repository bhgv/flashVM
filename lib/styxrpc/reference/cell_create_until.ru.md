# styxrpc_cell_create_until

[English](cell_create_until.md) · [Ячейки](named_cell.ru.md) · [Объявление](../include/styxrpc/named_cell.h#L66) · [Реализация](../src/named_channel_client.c#L240)

```c
#include <styxrpc/named_cell.h>

StyxRpcIoOutcome styxrpc_cell_create_until(
    struct StyxClient *session, const void *frame, size_t size,
    void *token, StyxRpcVfsFile **out, StyxRpcNamedStop stop, void *state);
```

Создать общую ячейку в подключённой `session`. Провайдер должен
обслуживать профиль общих ячеек на `/.channels`. Заимствуется допустимый
[кадр значения](cell_value.ru.md) из `size` байтов. Передайте записываемый
буфер на `STYXRPC_CELL_TOKEN` байтов в `token` и отдельный выходной
указатель `out`. При `STYXRPC_IO_OK` возвращаются полный жетон и
собственный fid для чтения и записи. Удерживайте fid для сохранения ячейки
и освободите его через [styxrpc_vfs_close](vfs_close.ru.md). При отказе оба
выхода сохраняют прежнее содержимое. Если провайдер принял отменённое
создание, временный fid удерживает ячейку до своего закрытия.

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
unsigned char token[STYXRPC_CELL_TOKEN];
const unsigned char initial[] = {STYXRPC_NAMED_BOOLEAN, 1};
StyxRpcVfsFile *owner = NULL;
StyxRpcIoOutcome result = styxrpc_cell_create_until(session, initial,
    sizeof(initial), token, &owner, stop_requested, task);
```

В примере `stop_requested(task)` читает флаг вызывающего.
После успеха передайте жетон через дескриптор общей ячейки либо
[прочитайте](cell_read_until.ru.md) значение в той же сессии. Закройте
собственный fid после последнего обращения.
