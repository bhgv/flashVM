# styxrpc_value_get_real

[Русский](value_get_real.ru.md) · [Header](../include/styxrpc/managed.h) · [Implementation](../src/managed.c)

Include `styxrpc/managed.h` and link `StyxRpc::rpc`.

```c
StyxRpcStatus styxrpc_value_get_real(const void *bytes, size_t size, double *value);
```

Decode one complete Dis real child from `bytes` of length `size`.
The function checks tag, framing and record kind and preserves the
binary64 payload without numerical conversion. `STYXRPC_OK` writes
`value`. `STYXRPC_INVALID` leaves it unchanged, including when the
host's double layout is unsupported. The input is borrowed. No I/O,
blocking or allocation occurs.

[Round-trip check](../tests/managed.c)
