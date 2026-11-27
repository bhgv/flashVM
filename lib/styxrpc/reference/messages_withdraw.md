# styxrpc_messages_withdraw

[Русский](messages_withdraw.ru.md) · [messages.h](../include/styxrpc/messages.h#L72) · [Publish](messages_publish.md)

`StyxRpc::serve`

```c
StyxRpcStatus styxrpc_messages_withdraw(StyxRpcServer *server, const char *name);
```

Call on the owner thread outside exchange callbacks. Pass a live `server` and
the exact root component in `name`. NULL selects `.unibus`.
`STYXRPC_OK` means the root has been removed, existing opens and walks are
invalidated, and every outstanding close hook has run. The caller can then
release the withdrawn registration's callback state. `STYXRPC_INVALID` means
the server or named root was absent.

To replace a registration, withdraw it before publishing the same name again.
Clients must walk and open the replacement on new fids. The server keeps old
fids separate until the client closes them.
