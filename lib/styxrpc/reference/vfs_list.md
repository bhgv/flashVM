# styxrpc_vfs_list

[Русский](vfs_list.ru.md) · [Source](../src/vfs_client.c)

```c
#include <styxrpc/vfs_client.h>
StyxRpcIoOutcome styxrpc_vfs_list(struct StyxClient *session, const char *path,
    size_t skip, size_t limit, StyxRpcVfsVisitor visit, void *state);
```

`StyxRpc::launch`

Open the directory on attached `session` for one enumeration. The [relative path](vfs_stat.md) may be empty for the attach root. Drop the first `skip` entries and visit up to the nonzero `limit` in provider order. `visit(state, name, info)` borrows all arguments for the callback and returns 1 to proceed. A record, transfer or visitor failure ends enumeration. The visitor can have observed earlier entries before an error. The SDK closes the directory before returning. EOF gives success with fewer entries.

For a cursor retained across calls, see [open directories](vfs_directory.md).
