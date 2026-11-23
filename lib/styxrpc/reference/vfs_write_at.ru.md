# styxrpc_vfs_write_at

[English](vfs_write_at.md) · [Заголовок](../include/styxrpc/vfs_client.h) · [Реализация](../src/vfs_client.c)

Подключите `styxrpc/vfs_client.h` и цель `StyxRpc::launch`.

```c
StyxRpcIoOutcome styxrpc_vfs_write_at(struct StyxClient *session, const char *path, uint64_t offset, const void *bytes, size_t size, size_t *used);
```

Записать до 65536 заимствованных байтов в существующий доступный
на запись файл с указанного смещения на одном fid. Применяются
[правила путей и статусы чтения](vfs_read_at.ru.md). Передача продолжается
со следующего подтверждённого смещения. Подтверждённая нулевая запись
останавливает передачу со статусом `STYXRPC_IO_OK`, в том числе после
короткого префикса. При ошибке `used` содержит только подтверждённые
байты. Запись с неопределённым эффектом не повторяется. Вызов не создаёт
и не усекает файл.

Операция синхронна и использует сроки сессии Styx. Сессия и буферы должны жить до возврата. [Проверка на C](../examples/client.c).
