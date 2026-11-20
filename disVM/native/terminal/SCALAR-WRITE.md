# styxscalarwrite

[Русский](SCALAR-WRITE.ru.md) · [API](scalar_record.h) · [Test](tests/scalar/record.c)

## Encode a scalar S-record

`scalar_record.h` · `styx_terminal`

Provide a nonzero 32-bit route tag, a kind (call, value or error), a signed
64-bit value, a writable byte buffer, its capacity in bytes and an output size.
Returns 1 and writes the encoded length on success, or 0 on invalid input or
insufficient capacity. The function borrows the buffer for the call. No I/O.

```c
styxscalarwrite(17, STYX_SCALAR_CALL, -7, request, sizeof(request), &length);
```
