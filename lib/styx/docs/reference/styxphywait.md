# styxphywait

[Русский](styxphywait.ru.md) · [Declaration](../../server/phy/server.h#L227) · [Implementation](../../server/phy/backend.c#L229)

Use the installed `server.h` from `Styx::phy_server`.

```c
char *styxphywait(struct Styxserver *server, int timeout);
```

The native owner waits for a complete request, outgoing progress, a local wake, or the same monotonic budget across partial readiness. `timeout` is `-1` for infinite wait, zero for a nonwaiting cycle, or a positive number of milliseconds. Pass a live PHY-backed server. NULL means the owner can continue with `styxprocess`, including after timeout or on a carrier failure that dispatch must report. An invalid server, invalid budget or clock error returns an error string. This call does not execute request callbacks. All waits and dispatch calls use the same owner.
