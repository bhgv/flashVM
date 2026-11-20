# styxunibusserverstartfile

[Русский](UNIBUS-SERVER-FILE.ru.md) · [Header](unibus_server.h) · [Two-VM example](../../examples/remote-scalar/STYX.md)

```c
StyxUnibusServer *styxunibusserverstartfile(RemotePhy *channel, unsigned long tag,
    DVM_Provider *original);
```

Include `unibus_server.h` and link `styx_unibus_server`. Supply an owned
connected Styx channel, a nonzero 32-bit inbox tag, and an optional borrowed
exact file provider. The server consumes the channel on every outcome.
When supplied, the original is retained and published as a read-only
`implementation.dis` beside the writable `.unibus`. The provider needs
metadata and sequential reads. To serve arbitrary read offsets it also
needs seek support.

The constructor reads metadata and starts a dedicated native thread. It
returns an owned server or NULL if publication fails. The original provider
must remain valid through construction. The server then owns a separate
reference. After the carrier shuts down, join with `styxunibusserverjoin`.
The single inbox accepts one complete record with the given tag.
