# styxrpc_graph_invoke

[Русский](graph_invoke.ru.md) · [Declaration](../include/styxrpc/graph_client.h#L16) · [Implementation](../src/graph_client.c#L11)

Link `StyxRpc::launch` and include `styxrpc/graph_client.h`. Fill a [managed target](managed_invoke.md) with a borrowed mounted session, exact relative module and export names, signature, schema and reply wait. A scoped offer requires the host-supplied reverse PID. Pass encoded argument children and a caller-owned reply buffer of at most 64 KiB. The call verifies the compiled offer and the exact positional marker on one held module directory before its first write.

`STYXRPC_CALL_OK` publishes borrowed result and update records into the reply buffer. Keep the buffer intact for as long as the records are used. `STYXRPC_CALL_NOT_SENT` means no request was acknowledged and submission is known not to have occurred. `STYXRPC_CALL_REMOTE_ERROR` is a confirmed remote error. `STYXRPC_CALL_UNCERTAIN` leaves execution unresolved. The call never retries a function after an uncertain transfer.

[Independent client](../examples/client.c) · [Sparse fields](graph_updates.md)
