# styxrpc_vfs_read_at

[English](vfs_read_at.md) · [Заголовок](../include/styxrpc/vfs_client.h) · [Реализация](../src/vfs_client.c)

Подключите `styxrpc/vfs_client.h` и цель `StyxRpc::launch`.

```c
StyxRpcIoOutcome styxrpc_vfs_read_at(struct StyxClient *session, const char *path, uint64_t offset, void *bytes, size_t size, size_t *used);
```

Прочитать до 65536 байтов за операцию с указанного байтового смещения
существующего файла. Для `path` действуют
[правила относительных путей](vfs_stat.ru.md). Нулевой размер проверяет
допуск. Удерживается один fid до запрошенного размера, EOF или ошибки.
При наличии `used` туда записывается подтверждённый префикс, даже если
следующее чтение завершилось ошибкой. Короткий результат с
`STYXRPC_IO_OK` допустим. Ошибка до операции с данными даёт
`STYXRPC_IO_NOT_OPEN`, подтверждённая ошибка I/O — `STYXRPC_IO_FAILED`,
ненадёжное подтверждение — `STYXRPC_IO_UNCERTAIN`. Неподтверждённые
байты не включаются в `used`.

Операция синхронна и использует сроки сессии Styx. Сессия и буферы должны жить до возврата. [Проверка на C](../examples/client.c).
