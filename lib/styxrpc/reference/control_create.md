# styxrpc_control_create

[Русский](control_create.ru.md) · [API](../include/styxrpc/control.h) · [Source](../src/control.c) · [Guide](../MODULES.md)

`StyxRpc::serve`

```c
StyxRpcStatus styxrpc_control_create(const StyxRpcControlSpec *spec, StyxRpcControl **out);
```

## Contract

Provide a positive byte limit and three callbacks. The upload callback borrows a complete payload. The command callback receives one parsed command. The status callback copies at most 4096 bytes into the supplied buffer and sets the byte count. Each callback returns 1 on success. The SDK copies the spec and borrows its context. It returns `STYXRPC_OK` and an owned descriptor on success, or `STYXRPC_INVALID` without changing `out`. The owner must serialize callbacks and keep their context alive through server shutdown.
