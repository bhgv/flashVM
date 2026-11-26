# vfs.rm(path)

[Русский](STYX_VFS_RM.ru.md) · [File API](STYX_RPC_OS.md) · [Implementation](../src/styxrpc/session_paths.c) · [C API](../../lib/styxrpc/reference/vfs_remove.md)

`vfs.rm(path)` or `os.rm(path)` removes one ordinary file or empty
directory. Relative paths start at the shared [working directory](STYX_VFS_CD.md).
The call returns `true` after confirmation and raises an error on refusal.
Nonempty directories, the attachment root and reserved service paths are
protected. In the stock DisVM pair this operation applies to entries backed
by the native filesystem. It waits for Styx I/O and reports an uncertain reply
as an error. Inspect the path before repeating an uncertain removal.

```actionscript
var removed = os.rm("drafts/old.txt");
```
