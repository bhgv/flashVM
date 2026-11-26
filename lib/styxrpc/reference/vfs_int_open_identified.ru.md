# styxrpc_vfs_int_open_identified

[English](vfs_int_open_identified.md) · [API](../include/styxrpc/vfs_client.h#L138) · [Исходник](../src/vfs_client.c#L313) · [Руководство](module_refs.ru.md)

`StyxRpc::launch`

```c
StyxRpcIoOutcome styxrpc_vfs_int_open_identified(struct StyxClient *session,
    const char *path, int writable, StyxRpcVfsFile **out, uint64_t *identity);
```

## Контракт

Передайте подключённую `session` и относительный путь `path` типизированной переменной, например `mounted/counters.dis/.var/count`. Режим `writable` равен 0 для чтения или 1 для чтения и записи. При успехе результат `STYXRPC_IO_OK`, `out` получает собственный fid, а `identity` получает Qid.path из той же операции stat, если указатель задан. NULL вместо `identity` даёт поведение обычного [открытия](vfs_int_open.ru.md).

Неверные аргументы или отказ walk либо open дают `STYXRPC_IO_NOT_OPEN`. Отказ stat или размер, отличный от четырёх байтов, дают `STYXRPC_IO_FAILED`. В обоих случаях результаты сохраняются. Вызов использует сроки запросов сессии. Полученный fid освобождается через [закрытие](vfs_close.ru.md).
