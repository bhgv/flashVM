# styxrpc_vfs_int_open

[English](vfs_int_open.md) · [API](../include/styxrpc/vfs_client.h#L221) · [Source](../src/vfs_int.c#L54) · [Guide](../MODULES.ru.md)

`StyxRpc::launch`

```c
StyxRpcIoOutcome styxrpc_vfs_int_open(struct StyxClient *session, const char *path,
    int writable, StyxRpcVfsFile **out);
```

## Контракт

Откройте известный относительный путь `.var/name`, включая выданный хостом путь под `.prog`. SDK проверяет обычный тип узла и длину в четыре байта. `writable` равен 0 для чтения и 1 для чтения с записью. Успех задаёт удерживаемый fid в `out` и возвращает `STYXRPC_IO_OK`. Отказ пути, режима или начального stat даёт `STYXRPC_IO_NOT_OPEN`. Ошибка stat или проверки размера после открытия даёт `STYXRPC_IO_FAILED`. Закройте fid через [API закрытия VFS](vfs_close.ru.md). Вызов использует срок сессии.

[Вариант с отменой](vfs_int_open_identified_until.ru.md).
