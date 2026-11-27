# styxrpc_vfs_open_until

[English](vfs_open_until.md) · [Обычный вызов](vfs_open.ru.md) · [Общий контракт](../VFS_CONTROL.ru.md)

## Синтаксис

```c
#include <styxrpc/vfs_client.h>

StyxRpcIoOutcome styxrpc_vfs_open_until(struct StyxClient *session, const char *path, StyxRpcFileMode mode,
    StyxRpcVfsFile **file,
    StyxRpcVfsStop stop, void *state);
```

Хостовая сборка C99, цель `StyxRpc::launch`.

Выполнить обход, проверку метаданных и открыть обычный файл в выбранном режиме. Если отмена выигрывает, временный fid освобождается, выходной указатель сохраняется.

## Параметры

| Параметр | Значение |
| --- | --- |
| `session` | Заимствованное подключение, живое до возврата. |
| `path` | Заимствованный видимый относительный путь UTF-8 до 1024 байт без начального слеша, точки и двух точек. |
| `mode` | STYXRPC_FILE_READ, STYXRPC_FILE_WRITE либо STYXRPC_FILE_RDWR. |
| `file` | Выходной параметр для собственного файла при успехе. Закройте через styxrpc_vfs_close. |
| `stop` | Необязательный короткий нативный колбэк в потоке вызова. Ненулевой результат останавливает ожидание. NULL сохраняет обычный режим. |
| `state` | Непрозрачный аргумент stop, заимствованный до возврата. |

## Результат

Коды ниже используют префикс `STYXRPC_IO_`. `OK` подтверждает результат. `CANCELLED` означает остановку ожидания колбэком. При включённом мультиплексоре посланный запрос завершается через `Tflush`, когда отмена выигрывает. Для последовательного транспорта колбэк проверяется перед этапом обмена. Подтверждённый исход имеет приоритет над отменой.

Скрытый путь, неверный тип или отказ открытия дают `NOT_OPEN`. Новый fid выдаётся только после `Ropen`.

## Пример

Используйте подключённый session или ранее открытый дескриптор и функцию stop, которая читает флаг остановки вашего потока.

```c
StyxRpcVfsFile *file = NULL;
StyxRpcIoOutcome status = styxrpc_vfs_open_until(session, "item",
    STYXRPC_FILE_READ, &file, stop, state);
styxrpc_vfs_close(file, 0);
```

Этот вызов проверяет [тест протокола](../tests/vfs_control_wire.c). Запустите его через `ctest --test-dir build-styxrpc -R styxrpc-vfs-control-wire --output-on-failure`. Он проверяет подтверждения, отмену и работу соседнего fid.

[Заголовок](../include/styxrpc/vfs_client.h) · [Реализация](../src/vfs_client.c#L210)
