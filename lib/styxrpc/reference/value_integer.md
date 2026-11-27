# styxrpc_value_integer

[Русский](value_integer.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/managed.h>

StyxRpcStatus styxrpc_value_integer(unsigned long tag, int64_t value,
    void *bytes, size_t capacity, size_t *used);
```

Build as C99 and link `StyxRpc::rpc`.

## Arguments

| Parameter | Description |
| --- | --- |
| `tag` | Child type from the checked schema. Use `STYXRPC_VALUE_INT` for signed 32-bit, `STYXRPC_VALUE_BIG` for signed 64-bit or `STYXRPC_VALUE_BYTE` for 0…255. |
| `value` | Signed value to encode, within the selected type's range. For `STYXRPC_VALUE_BIG`, every `int64_t` is valid. |
| `bytes` | Caller-owned writable output buffer. |
| `capacity` | Size of `bytes` in bytes. |
| `used` | Writable location for the encoded record length in bytes. |

## Returns

`STYXRPC_OK` reports a complete child record in the first `*used` bytes.
`STYXRPC_INVALID` reports an unsupported tag, out-of-range value, invalid
output argument or insufficient capacity. When `used` is valid, an error
sets `*used` to zero. Use the buffer's content only on success.

This codec runs synchronously without allocating, performing I/O or
retaining the buffer. Use independent output storage for concurrent
calls. A child record can be placed in a managed argument, result,
array, tuple or typed channel payload where the checked schema expects
its tag. DisVM and the C SDK use the same signed integer representation.

[Decode](value_get_integer.md) · [Standalone C example](../examples/adjust.c) · [Signed 64-bit example](../examples/disvm/TYPED_VALUES.md#check-the-same-integer-record-with-the-c-sdk) · [Source](../src/value_codec.c#L12)
