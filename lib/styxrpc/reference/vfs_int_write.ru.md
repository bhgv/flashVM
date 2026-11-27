# styxrpc_vfs_int_write

[English](vfs_int_write.md) · [Source](../src/vfs_int.c#L164)

```c
#include <styxrpc/vfs_client.h>
StyxRpcIoOutcome styxrpc_vfs_int_write(StyxRpcVfsFile *file, int32_t value);
```

`StyxRpc::launch`

Замените нативную знаковую 32-битную переменную в открытом на запись файле `.var`. Отправьте одно четырёхбайтовое `value` в сетевом порядке со смещения ноль и потребуйте подтверждения всех байтов. Короткая или отклонённая передача даёт `STYXRPC_IO_FAILED`. Потеря ответа даёт `STYXRPC_IO_UNCERTAIN` без повтора. Храните fid открытым и сериализуйте обращения.

[Вариант с отменой](vfs_int_write_until.ru.md).
