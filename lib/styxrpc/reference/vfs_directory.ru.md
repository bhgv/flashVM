# Итераторы каталогов VFS

[English](vfs_directory.md) · [Заголовок](../include/styxrpc/vfs_client.h) · [Реализация](../src/vfs_directory.c) · [styxrpc_vfs_dir_open_until](vfs_dir_open_until.ru.md) · [styxrpc_vfs_dir_next_until](vfs_dir_next_until.ru.md)

`styxrpc_vfs_dir_open(session, path, &dir)` открывает видимый каталог по
относительному пути. Пустой путь обозначает корень. При успехе вызывающая
сторона владеет итератором и его fid.
`styxrpc_vfs_dir_next(dir, visit, state, &found)` читает одну видимую запись
и вызывает `visit` с заимствованным именем и метаданными. Null-колбэк
пропускает эту запись. На EOF функция задаёт нулевой `found` и возвращает
`STYXRPC_IO_OK`. Ошибка чтения или неверная запись прерывает текущий
обход. `styxrpc_vfs_dir_close(dir, deferred)` освобождает итератор. Передайте
1 для отложенного закрытия fid в сессии или 0 для синхронного. Открытие,
чтение и синхронное закрытие могут ждать I/O Styx. Работайте с итератором
из одного потока владельца.

Используйте [styxrpc_vfs_dir_rewind](vfs_dir_rewind.ru.md) для нового обхода того же fid.
