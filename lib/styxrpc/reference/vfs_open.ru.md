# styxrpc_vfs_open

[English](vfs_open.md) · [Source](../src/vfs_client.c)

```c
#include <styxrpc/vfs_client.h>
StyxRpcIoOutcome styxrpc_vfs_open(struct StyxClient *session, const char *path,
    StyxRpcFileMode mode, StyxRpcVfsFile **file);
```

`StyxRpc::launch`

Передайте смонтированную `session`, непустой [относительный путь](vfs_stat.ru.md) и режим `STYXRPC_FILE_READ`, `STYXRPC_FILE_WRITE` либо `STYXRPC_FILE_RDWR`. Открывается существующий обычный файл. При `STYXRPC_IO_OK` вызывающий получает собственный fid в `file`. Отказ допуска даёт `STYXRPC_IO_NOT_OPEN` и сохраняет выход. Сериализуйте обращения и освободите дескриптор через [close](vfs_close.ru.md).
