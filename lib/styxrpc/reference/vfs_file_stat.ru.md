# styxrpc_vfs_file_stat

[English](vfs_file_stat.md) · [Source](../src/vfs_client.c) · [styxrpc_vfs_file_stat_until](vfs_file_stat_until.ru.md)

```c
#include <styxrpc/vfs_client.h>
StyxRpcIoOutcome styxrpc_vfs_file_stat(StyxRpcVfsFile *file, StyxRpcFileInfo *info);
```

`StyxRpc::launch`

Получите метаданные точного удерживаемого `file`. Вызывающий владеет `info`; при успехе копируются длина в байтах, права и признак каталога. Отсутствие дескриптора даёт `STYXRPC_IO_NOT_OPEN`, ошибка stat — `STYXRPC_IO_FAILED`. Ошибки сохраняют выход. Сериализуйте с другими обращениями и закрытием.
