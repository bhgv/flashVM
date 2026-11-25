# styxrpc_vfs_list

[English](vfs_list.md) · [Source](../src/vfs_client.c)

```c
#include <styxrpc/vfs_client.h>
StyxRpcIoOutcome styxrpc_vfs_list(struct StyxClient *session, const char *path,
    size_t skip, size_t limit, StyxRpcVfsVisitor visit, void *state);
```

`StyxRpc::launch`

Откройте каталог на смонтированной `session` для одного обхода. [Относительный путь](vfs_stat.ru.md) может быть пустым для корня. Пропустите первые `skip` записей и посетите до ненулевого `limit` в порядке провайдера. `visit(state, name, info)` заимствует аргументы на время вызова и возвращает 1 для продолжения. Ошибка записи, передачи или колбэка завершает обход. Колбэк мог ранее получить часть списка. SDK закрывает каталог до возврата. EOF даёт успешный короткий список.

Курсор между вызовами удерживает [итератор каталога](vfs_directory.ru.md).
