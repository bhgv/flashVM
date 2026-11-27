# styxrpc_vfs_mkdir

[English](vfs_mkdir.md) · [Заголовок](../include/styxrpc/vfs_client.h) · [Реализация](../src/vfs_create.c) · [FlashVM](../../../flashVM/doc/STYX_VFS_MKDIR.ru.md) · [styxrpc_vfs_mkdir_until](vfs_mkdir_until.ru.md)

Подключите `StyxRpc::launch` и передайте подключённую сессию и непустой
канонический путь относительно корня. Родитель должен быть обычным каталогом.
Библиотека отправляет один запрос создания каталога с правами 0777 и
закрывает полученный fid. Провайдер применяет свои правила доступа.
`STYXRPC_IO_OK` означает подтверждённое создание. Неверный путь и
отсутствие родителя дают `STYXRPC_IO_NOT_OPEN`. Отказ провайдера даёт
`STYXRPC_IO_FAILED`, отсутствие подтверждения — `STYXRPC_IO_UNCERTAIN`.
Вызов ждёт до истечения срока запроса.
