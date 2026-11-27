# styxrpc_vfs_int_read

[English](vfs_int_read.md) · [Source](../src/vfs_int.c#L102)

```c
#include <styxrpc/vfs_client.h>
StyxRpcIoOutcome styxrpc_vfs_int_read(StyxRpcVfsFile *file, int32_t *value);
```

`StyxRpc::launch`

Прочитайте всю нативную знаковую 32-битную переменную открытого файла `.var` одним запросом со смещения ноль. `value` принадлежит вызывающему и меняется после подтверждения ровно четырёх байтов в сетевом порядке. Короткая или отклонённая передача даёт `STYXRPC_IO_FAILED`, отсутствие подтверждения может дать `STYXRPC_IO_UNCERTAIN`. Храните fid открытым и сериализуйте обращения.

[Вариант с отменой](vfs_int_read_until.ru.md).
