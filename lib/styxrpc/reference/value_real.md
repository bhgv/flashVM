# styxrpc_value_real

[Русский](value_real.ru.md) · [Header](../include/styxrpc/managed.h) · [Implementation](../src/managed.c)

Include `styxrpc/managed.h` and link `StyxRpc::rpc`.

```c
StyxRpcStatus styxrpc_value_real(double value, void *bytes, size_t capacity, size_t *used);
```

Encode the supplied binary64 `value` as the existing Dis real child,
including its signed zero and NaN payload bits. The caller owns output
`bytes`, of `capacity` bytes. On `STYXRPC_OK`, `used` receives the frame
extent. On `STYXRPC_INVALID`, `used` is cleared if supplied. Invalid
arguments, insufficient capacity and unsupported native double layouts
are refused without I/O or allocation.

[Round-trip check](../tests/managed.c)
