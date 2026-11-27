# styxrpc_module_ref_open_remote

[English](module_ref_open_remote.md) · [API](../include/styxrpc/module_ref.h#L48) · [Исходник](../src/variable_ref_client.c#L78) · [Руководство](module_refs.ru.md)

`StyxRpc::launch`

```c
StyxRpcIoOutcome styxrpc_module_ref_open_remote(struct StyxClient *issuer,
    const StyxRpcModuleRef *ref, StyxRpcVfsFile **out);
```

## Контракт

Передайте обычное подключение к VFS исходного владельца в `issuer`. Используйте выбранный по роли владельца [декодированный дескриптор](module_ref_decode.ru.md), канонический путь, ненулевой Qid.path и режим 0 для чтения или 1 для чтения и записи. Вызов открывает файл в этом режиме и сверяет Qid.path открытого fid.

При успехе результат `STYXRPC_IO_OK` и в `out` передаётся собственный fid. Недопустимые аргументы, отказ или другой Qid дают `STYXRPC_IO_NOT_OPEN`. Ошибка stat или проверка длины с отказом даёт `STYXRPC_IO_FAILED`. В обоих случаях `out` сохраняется, временный fid закрывается. Синхронный вызов использует сроки запросов клиента. Для полученного файла используйте [чтение](vfs_int_read.ru.md), [запись](vfs_int_write.ru.md) и [закрытие](vfs_close.ru.md).

[Вариант с отменой](module_ref_open_remote_until.ru.md).
