# styxrpc_value_get_integer

[Русский](value_get_integer.ru.md) · [Header](../include/styxrpc/managed.h) · [Source](../src/managed.c)

`StyxRpcStatus styxrpc_value_get_integer(unsigned long tag, const void *bytes, size_t size, int64_t *value);`

Include `styxrpc/managed.h` and link the portable target `StyxRpc::rpc`.
Pass `STYXRPC_VALUE_INT`, `STYXRPC_VALUE_BIG` or `STYXRPC_VALUE_BYTE` and
one complete encoded child of that type. The function borrows the input
and checks its framing, kind and exact signed or unsigned width. On
`STYXRPC_OK` it writes `value`. On `STYXRPC_INVALID` the output is
unchanged. The function does no I/O, allocation or blocking.
See [the matching encoder](value_integer.md) and the [C caller](../examples/client.c).
