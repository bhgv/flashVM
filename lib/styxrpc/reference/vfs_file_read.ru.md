# styxrpc_vfs_file_read

[English](vfs_file_read.md) · [Source](../src/vfs_client.c)

```c
#include <styxrpc/vfs_client.h>
StyxRpcIoOutcome styxrpc_vfs_file_read(StyxRpcVfsFile *file, uint64_t offset,
    void *bytes, size_t size, size_t *used);
```

`StyxRpc::launch`

Прочитайте до `size` байтов открытого `file` со смещения `offset`. Вызывающий передаёт записываемые `bytes` и `used` и ведёт свой курсор. Действуют [границы и статусы VFS](vfs_read_at.ru.md). В `used` входят подтверждённые байты. Успешный короткий ответ включает EOF. Сериализуйте обращения к fid.

Используйте [styxrpc_vfs_file_read_until](vfs_file_read_until.ru.md),
если нужна остановка во время ожидания ответа.
