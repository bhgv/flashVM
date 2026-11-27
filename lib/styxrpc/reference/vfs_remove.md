# styxrpc_vfs_remove

[Русский](vfs_remove.ru.md) · [Header](../include/styxrpc/vfs_client.h) · [Implementation](../src/vfs_create.c) · [FlashVM](../../../flashVM/doc/STYX_VFS_RM.md) · [styxrpc_vfs_remove_until](vfs_remove_until.md)

Link `StyxRpc::launch` and pass an attached session and a canonical,
nonempty ordinary path relative to its root. The library walks and checks
the entry and then sends one consuming Styx remove. The provider determines
whether removal is allowed. `STYXRPC_IO_OK` confirms removal.
`STYXRPC_IO_NOT_OPEN` means the entry could not be selected.
`STYXRPC_IO_FAILED` indicates refusal. `STYXRPC_IO_UNCERTAIN` means the
request may have taken effect. Inspect the directory before retrying.
The call owns and consumes all of its temporary fids.
