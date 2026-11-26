# styxrpc_vfs_stat

[English](vfs_stat.md) · [Заголовок](../include/styxrpc/vfs_client.h) · [Реализация](../src/vfs_client.c)

Подключите `styxrpc/vfs_client.h` и цель `StyxRpc::launch`.

```c
StyxRpcIoOutcome styxrpc_vfs_stat(struct StyxClient *session, const char *path, StyxRpcFileInfo *info);
```

Разрешить путь в заимствованной сессии и выполнить stat точного fid.
`path` — относительный путь длиной до 1024 байтов. Пустая строка адресует
корень. В непустом пути пустые компоненты, точка и родительские переходы отклоняются. При успехе `info`
получает длину в байтах, режим и признак каталога. При ошибке выход
сохраняется. Неправильный путь или ошибка обхода дают
`STYXRPC_IO_NOT_OPEN`, ошибка stat после обхода — `STYXRPC_IO_FAILED`.

Операция синхронна и использует сроки сессии Styx. Сессия и буферы должны жить до возврата. [Проверка на C](../examples/client.c).

Обычные пути отклоняют служебный компонент `.prog` на любой глубине.

[Служебные имена](vfs_service_name.ru.md) можно проверять при нормализации пути.
