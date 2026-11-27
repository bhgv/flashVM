# styxrpc_cell_read_until

[English](cell_read_until.md) · [Ячейки](named_cell.ru.md) · [Объявление](../include/styxrpc/named_cell.h#L82) · [Реализация](../src/named_channel_client.c#L346)

```c
#include <styxrpc/named_cell.h>

StyxRpcIoOutcome styxrpc_cell_read_until(
    struct StyxClient *session, const void *token, unsigned char **bytes,
    size_t *size, StyxRpcNamedStop stop, void *state);
```

Прочитать текущий [кадр значения](cell_value.ru.md) общей ячейки
в выдавшей `session`. Заимствуется `token` длиной
`STYXRPC_CELL_TOKEN` байтов. До возврата удерживайте существующую
аренду либо другую действующую ссылку на ячейку. SDK присоединяет
и освобождает собственный временный fid при любом исходе. Аренда
вызывающего остаётся доступной. `STYXRPC_IO_OK` передаёт в `bytes`
кадр от malloc и указывает длину в `size`. Освободите кадр через
`free`. При отказе оба выхода сохраняются.

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
unsigned char *frame = NULL;
size_t size = 0;
StyxRpcIoOutcome result = styxrpc_cell_read_until(session, token, &frame,
    &size, stop_requested, task);
if (result == STYXRPC_IO_OK)
{
    free(frame);
}
```

В примере `stop_requested(task)` читает флаг вызывающего.
Жетон выдаётся живой ячейкой этой сессии. Для работы со значением
декодируйте кадр перед освобождением. У параллельных вызовов отдельные
выходы и согласованное время жизни удерживающей ссылки.
