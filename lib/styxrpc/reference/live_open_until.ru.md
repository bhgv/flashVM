# styxrpc_live_open_until

[English](live_open_until.md) · [Объявление](../include/styxrpc/live_client.h#L38) · [Реализация](../src/live_collection.c#L228) · [Руководство](../LIVE_VALUES.ru.md)

## Синтаксис

```c
#include <styxrpc/live_client.h>
StyxRpcIoOutcome styxrpc_live_open_until(struct StyxClient *session, const char *path,
    int writable, StyxRpcLive **out, StyxRpcVfsStop stop, void *state);
```

Подключите `StyxRpc::launch`.

## Параметры

| Имя | Назначение |
| --- | --- |
| `session` | Смонтированный хостовый клиент. Заимствуется на всё время жизни привязки. |
| `path` | Канонический относительный путь с завершающим нулём. Длина 1–1024 байт. Допустимы опубликованные `.var` и `.prog` перед `.var`. Начальный слеш, пустые компоненты и конечный слеш недопустимы. Файл скаляра Int32 допускается внутри `.var`. |
| `writable` | Верхняя граница прав. 0 означает чтение, 1 допускает выданные права записи. Режимы владельца действуют. |
| `out` | Адрес результата. При успехе получает собственную привязку, при отказе прежнее значение сохраняется. |
| `stop`, `state` | Необязательная синхронная функция остановки и её заимствованный аргумент. Для остановки верните ненулевое число. Без остановки передайте `NULL`. |

## Результаты

`STYXRPC_IO_OK` передаёт точный неоткрытый fid и сохраняет проверенный тип. Освободите привязку перед уничтожением сессии. `STYXRPC_IO_NOT_OPEN` охватывает неверные аргументы, недоступный путь и неизвестную проекцию. Остановленный обход или чтение могут вернуть `STYXRPC_IO_CANCELLED`. Ошибки обмена и выделения тоже приводят к отказу. Выполняйте вызов у владельца сессии. Функция не удерживает `path` и функцию остановки.


## Пример

```c
StyxRpcLive *values = NULL;
StyxRpcIoOutcome status = styxrpc_live_open_until(session, "live/.var/numbers",
    1, &values, NULL, NULL);
/* Retire values even if subsequent reads fail. */
styxrpc_live_close(values, 0);
```

[Полный пример C](../examples/disvm/LIVE_STAGE.ru.md) · [Обмен и владение](../LIVE_VALUES.ru.md#владение-и-ожидание)
