# vfs.cd(path)

[Русский](STYX_VFS_CD.ru.md) · [pwd](STYX_VFS_PWD.md) · [Implementation](../src/styxrpc/session_paths.c)

Use `vfs.cd(path)` or `os.cd(path)` to set the shared working directory.
The path is a nonempty string. Absolute paths start at the attachment root.
Relative paths start at the current working directory. The resolver folds
`.`, `..` and repeated slashes. Walking above the root stays at the root.
The call verifies that the destination opens as an ordinary directory and
returns `true`. A failed check preserves the current working directory.

Later relative file and named channel calls use this path in the live VFS.
The mounted service paths and the peer's working directory are independent.
File and directory handles already opened keep their existing fids.

```actionscript
var entered = vfs.cd("/work");
var file = os.open("readme.txt");
file.close();
```
