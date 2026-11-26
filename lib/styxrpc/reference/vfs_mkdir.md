# styxrpc_vfs_mkdir

[Русский](vfs_mkdir.ru.md) · [Header](../include/styxrpc/vfs_client.h) · [Implementation](../src/vfs_create.c) · [FlashVM](../../../flashVM/doc/STYX_VFS_MKDIR.md)

Link `StyxRpc::launch` and pass an attached session and a canonical,
nonempty path relative to its root. The parent must be an ordinary directory.
The library sends one directory create with requested mode 0777 and closes
the resulting fid. The provider applies its own permission rules.
The result is `STYXRPC_IO_OK` on confirmed creation. Invalid paths or a
missing parent return `STYXRPC_IO_NOT_OPEN`. Peer refusal is
`STYXRPC_IO_FAILED` and unconfirmed delivery is `STYXRPC_IO_UNCERTAIN`.
I/O waits up to the request deadline.
