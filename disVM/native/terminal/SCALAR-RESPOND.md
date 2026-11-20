# styxscalarrespond

[Русский](SCALAR-RESPOND.ru.md) · [API](scalar_record.h) · [Test](tests/scalar/record.c)

## Dispatch one admitted scalar call

`scalar_record.h` · `styx_terminal`

Provide two nonzero 32-bit route tags, one complete request and its byte
length, a method callback and its opaque state, plus a writable reply buffer
of at least 32 bytes and an output length. Authorization, serialization
and one-outstanding-call policy belong to the host. The function validates
the request before invoking the callback. The callback returns 1 and writes
a value, or another result and an error number. The function returns 1
when it encodes the corresponding reply. Invalid inputs or encoding failure
return 0. It does no network I/O, but the callback can block. No automatic retry.

```c
styxscalarrespond(17, request, length, 29, method, state,
    reply, sizeof(reply), &reply_length);
```
