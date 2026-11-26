# vfs.mkdir(path)

[Русский](STYX_VFS_MKDIR.ru.md) · [File API](STYX_RPC_OS.md) · [Implementation](../src/styxrpc/session_paths.c) · [C API](../../lib/styxrpc/reference/vfs_mkdir.md)

`vfs.mkdir(path)` or `os.mkdir(path)` creates one new directory and returns
`true` after the peer confirms creation. Relative paths start at the shared
[working directory](STYX_VFS_CD.md). The parent must exist and allow creation.
An occupied name, hidden service path, or provider refusal raises an error.
The operation waits for the Styx reply. In the stock DisVM pair the mode is
0777 restricted by the native process umask. If the result is uncertain,
inspect the current directory before attempting creation again.

```actionscript
var made = vfs.mkdir("drafts");
var entered = os.cd("drafts");
```
