# styxrpc_vfs_close

[English](vfs_close.md) · [Source](../src/vfs_client.c)

```c
#include <styxrpc/vfs_client.h>
void styxrpc_vfs_close(StyxRpcVfsFile *file, int deferred);
```

`StyxRpc::launch`

Освободите собственный `file`. Допускается NULL. При нуле в `deferred` клиент синхронно пытается выполнить clunk. Ненулевой флаг ставит очистку в очередь клиента без ожидания удалённого ответа. Сериализуйте с обращениями к fid. После вызова указатель недействителен.
