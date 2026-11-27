# styxrpc_cell_write_until

[English](cell_write_until.md) · [Ячейки](named_cell.ru.md) · [Объявление](../include/styxrpc/named_cell.h#L90) · [Реализация](../src/named_channel_client.c#L367)

```c
#include <styxrpc/named_cell.h>

StyxRpcIoOutcome styxrpc_cell_write_until(
    struct StyxClient *session, const void *token, const void *frame,
    size_t size, StyxRpcNamedStop stop, void *state);
```

Заменить полный [кадр значения](cell_value.ru.md) в выдавшей
`session`. Заимствуются `token` длиной `STYXRPC_CELL_TOKEN` байтов
и `size` байтов кадра `frame`. Удерживайте исходную аренду или
ссылку переданного дескриптора. SDK использует временный fid и
освобождает его при любом исходе. Остальные владельцы сохраняют доступ.

`STYXRPC_IO_OK` подтверждает полную замену. Отмена подготовки или
отправки заголовка возвращает `STYXRPC_IO_CANCELLED`. Если отправленный
фрагмент значения потерял подтверждение, в том числе при отмене,
SDK возвращает `STYXRPC_IO_UNCERTAIN`. Сервер меняет ячейку после
приёма полного кадра. Полностью подтверждённая запись может завершиться
успехом при одновременной отмене. SDK отправляет присваивание один раз.
Закрытие fid сохраняет уже применённую замену.

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
const unsigned char replacement[] = {STYXRPC_NAMED_BOOLEAN, 0};
StyxRpcIoOutcome result = styxrpc_cell_write_until(session, token,
    replacement, sizeof(replacement), stop_requested, task);
```

В примере `stop_requested(task)` читает флаг вызывающего.
При неопределённом исходе [прочитайте](cell_read_until.ru.md)
текущее видимое значение, если оно нужно приложению. Другие владельцы
могут писать между операциями. Согласуйте последовательности чтения,
изменения и записи на уровне приложения.
