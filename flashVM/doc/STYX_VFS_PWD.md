# vfs.pwd()

[Русский](STYX_VFS_PWD.ru.md) · [cd](STYX_VFS_CD.md) · [Implementation](../src/styxrpc/session_paths.c)

`vfs.pwd()` and `os.pwd()` return the same absolute working path as a string
and take no arguments. A fresh movie starts at `"/"`. The result holds
the path at the time of the call. It remains valid across later changes
to the working directory. The operation performs no network I/O.
