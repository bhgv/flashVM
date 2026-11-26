# styxrpc_vfs_service_name

[Русский](vfs_service_name.ru.md) · [Header](../include/styxrpc/vfs_client.h) · [Source](../src/named_channel_path.c)

Link `StyxRpc::rpc`. Borrow a pathname component and its byte count.
The byte range must be readable for the call. Return one for an exact
reserved name and zero otherwise. This spelling check does no I/O and
can run in any native thread. Ordinary opens also check peer metadata
for service nodes published under custom names.
