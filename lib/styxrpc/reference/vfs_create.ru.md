# styxrpc_vfs_create

[English](vfs_create.md) · [Заголовок](../include/styxrpc/vfs_client.h) · [Исходник](../src/vfs_create.c) · [Открыть существующий файл](vfs_open.ru.md)

```c
#include <styxrpc/vfs_client.h>
StyxRpcIoOutcome styxrpc_vfs_create(struct StyxClient *session, const char *path,
    StyxRpcVfsFile **file);
```

Подключите `StyxRpc::launch`. Передайте подключённую сессию, непустой
обычный [относительный путь](vfs_stat.ru.md) и адрес указателя на файл.
Родительский каталог должен существовать. Библиотека посылает один
запрос Styx на создание обычного файла для записи с режимом 0666.
Провайдер применяет свои права и отклоняет занятые имена.

При `STYXRPC_IO_OK` вызывающий получает открытый на запись fid.
Сериализуйте его использование и освободите через [close](vfs_close.ru.md).
При ошибке `file` сохраняется. `STYXRPC_IO_NOT_OPEN` означает отказ до
запроса создания. `STYXRPC_IO_FAILED` означает, что открытый дескриптор
не получен. `STYXRPC_IO_UNCERTAIN` означает неопределённую доставку.
Перед повтором проверьте каталог. Вызов может ждать I/O до окончания срока
запроса сессии.
