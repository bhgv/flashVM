# styxrpc_vfs_dir_rewind

[English](vfs_dir_rewind.md) · [Курсоры каталогов](vfs_directory.ru.md) · [Заголовок](../include/styxrpc/vfs_client.h) · [Реализация](../src/vfs_directory.c)

```c
#include <styxrpc/vfs_client.h>
StyxRpcIoOutcome styxrpc_vfs_dir_rewind(StyxRpcVfsDir *dir);
```

Слинкуйте hosted SDK. Передайте живой `dir`, которым владеет
вызывающая сторона. Функция очищает буфер записей и сбрасывает
смещение в байтах, EOF и прежнюю ошибку обхода. Возвращает
`STYXRPC_IO_OK` или `STYXRPC_IO_NOT_OPEN` для нулевого указателя.
Исходный Styx fid остаётся открытым. Ввода-вывода нет. Следующий
`styxrpc_vfs_dir_next` читает с нуля на том же fid. Если права или
идентификатор на стороне сервера больше не позволяют чтение, этот
вызов сообщит об ошибке. Разрешения пути и подмены каталога нет.

Сериализуйте вызовы для каждого курсора. Перемотка допустима после
EOF или ошибки чтения каталога. Для разорванного соединения нужна
новая сессия. Например, после успешного `styxrpc_vfs_dir_rewind(dir)`
вызывайте `styxrpc_vfs_dir_next` до EOF, чтобы перечитать каталог
с начала. Освободите курсор через [close](vfs_directory.ru.md).

После [отменённого шага](vfs_dir_next_until.ru.md) fid уже освобождён. Перемотка возвращает `STYXRPC_IO_NOT_OPEN`.
