# styxrpc_module_capture_until

[English](module_capture_until.md) · [Объявление](../include/styxrpc/module_client.h) · [Реализация](../src/client_exchange.c)

```c
#include <styxrpc/module_client.h>

StyxRpcIoOutcome styxrpc_module_capture_until(struct StyxClient *session,
    const char *path, struct StyxFid *base, struct StyxFid **out,
    StyxRpcModuleStop stop, void *state);
```

Хостовая цель C `StyxRpc::launch` захватывает каталог смонтированного
модуля. Если `base` равен NULL, передайте подключённую `session` и
относительный `path` в обычной VFS. Для каталога нужны права чтения и поиска.
Если `base` задан, передайте заимствованный неоткрытый fid каталога модуля.
Функция клонирует точный fid и игнорирует `session` и `path`. Это
сохраняет каталог после перепривязки пути.

При `STYXRPC_IO_OK` `out` получает собственный неоткрытый fid.
Освободите его через `styxrpc_vfs_close`. `STYXRPC_IO_NOT_OPEN` означает
неверный аргумент, отказ обхода или неподходящий каталог.
`STYXRPC_IO_FAILED` может означать внутреннюю ошибку подготовки.
`STYXRPC_IO_CANCELLED` означает остановку. При отказе `out` сохраняется.

Необязательный `stop(state)` исполняется в потоке вызова. Ненулевой
результат останавливает работу. Удерживайте `state` и `base` до возврата.
NULL оставляет обычное ожидание. В мультиплексированной сессии обход
или stat отменяется по тегу. В последовательной сессии флаг проверяется
между обменами. Очистка временных fid может продолжиться после возврата.

```c
struct StyxFid *copy = NULL;
if (styxrpc_module_capture_until(NULL, NULL, directory, &copy, NULL, NULL) ==
    STYXRPC_IO_OK) {
    styxrpc_vfs_close(copy, 0);
}
```

Передайте каталог от [живой загрузки](module_live_open_until.ru.md).
Удерживайте её fid запроса во время работы с копией. До закрытия через
копию можно [обнаружить экспорты](module_exports_scoped_at_until.ru.md).
