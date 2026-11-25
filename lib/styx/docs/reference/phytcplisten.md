# phytcplisten

[Русский](phytcplisten.ru.md) · [Declaration](../../remote_phy/drivers/posix/tcp.h) · [Source](../../remote_phy/drivers/posix/tcp.c)

Use `<styx/phy/drivers/posix/tcp.h>` with `Styx::carrier` in a hosted Linux64 build.

```c
int phytcplisten(const char *host, const char *port, unsigned *bound_port);
```

`host` names the local interface or an address to bind. `port` is a decimal
string in 0–65535. With `0` the kernel selects the port. `bound_port` must
point to writable caller storage for the selected port. The function can block
during name resolution and uses the first address it can bind and listen on.

Success returns an owned blocking listener with close-on-exec and writes the
actual port. Failure returns `-1`, closes any temporary sockets and leaves
`bound_port` unchanged. For example, listen on `"127.0.0.1", "0"`, then call
[`phytcpaccept`](phytcpaccept.md). Close the listener when no more connections
are needed. The TCP layer supplies plaintext transport. Select the bind address
with the intended network exposure in mind.
