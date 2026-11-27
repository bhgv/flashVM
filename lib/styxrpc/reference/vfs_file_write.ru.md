# styxrpc_vfs_file_write

[English](vfs_file_write.md) · [Source](../src/vfs_client.c)

```c
#include <styxrpc/vfs_client.h>
StyxRpcIoOutcome styxrpc_vfs_file_write(StyxRpcVfsFile *file, uint64_t offset,
    const void *bytes, size_t size, size_t *used);
```

`StyxRpc::launch`

[Вариант с отменой](vfs_file_write_until.ru.md)

Запишите до `size` байтов открытого `file` по смещению `offset`. Вызывающий владеет исходными `bytes`, обязательным счётчиком `used` и курсором. Действуют [границы и статусы VFS](vfs_write_at.ru.md). SDK продвигается только на подтверждённые байты и завершает при ошибке либо нулевом результате. Неопределённый фрагмент не повторяется. Сериализуйте обращения к fid.
