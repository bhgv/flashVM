# styxunibusserverstartfunction

[Русский](SCALAR-FUNCTION.ru.md) · [Header](unibus_server.h) · [Example](tests/scalar/route.c)

```c
StyxUnibusServer *styxunibusserverstartfunction(
    RemotePhy *channel, const char *module, const char *name,
    const struct StyxScalarFunction *function);
```

Link `styx_unibus_server`. Include `unibus_server.h` and `scalar_function.h`.
The function takes ownership of the Styx transport channel even on failure.
It returns an owned server or NULL. After closing the peer, consume the
server with `styxunibusserverjoin`.

The module and export names are single path components. The server publishes
`mounted/<module>/<name>` with mode 0777 and allows ORDWR to invoke it. It copies
the function descriptor. The caller keeps the callback context alive until
join. Both tags are nonzero unsigned 32-bit values. The callback runs on the
Styx dispatcher thread, so use this endpoint with synchronous native methods
that complete promptly. Each open has independent response state. Callbacks
on the same server are serialized.

Write one complete [scalar S-record](SCALAR-WRITE.md) at offset zero. Read from
offset zero, advancing the read offset by the confirmed byte count. A result
or method error is encoded as one record. A malformed request, a read before
the request, or a second accepted-call attempt on the same fid gets a Styx
error. Closing the fid discards its response. The endpoint uses the existing
scalar codec. Typed Dis loading, `.interface` discovery and compound arguments
are separate integration work.

Build and run `terminal-scalar-route` from the native SDK. The example opens
`mounted/compute.dis/compute` twice, submits two scalar calls and reads the
responses in reverse order. It checks that a truncated frame and a duplicate
write do not run the callback. See the [paired DisVM tutorial](../../examples/remote-scalar/STYX.md)
for the existing typed route.
