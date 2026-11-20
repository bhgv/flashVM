# styxscalarread

[Русский](SCALAR-READ.ru.md) · [API](scalar_record.h) · [Test](tests/scalar/record.c)

## Read an exact scalar S-record

`scalar_record.h` · `styx_terminal`

Provide the expected nonzero 32-bit route tag, the read-only record bytes
and length, and writable kind and value outputs. Returns 1 after decoding.
Returns 0 for invalid input, a wrong tag, an unknown kind, an incomplete
record or trailing bytes. Refusal leaves both outputs unchanged. No I/O.

```c
styxscalarread(29, reply, length, &kind, &value);
```
