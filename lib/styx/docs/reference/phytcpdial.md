# phytcpdial

[Русский](phytcpdial.ru.md) · [Declaration](../../remote_phy/drivers/posix/tcp.h) · [Source](../../remote_phy/drivers/posix/tcp.c)

Use `<styx/phy/drivers/posix/tcp.h>` with `Styx::carrier` in a hosted Linux64 build.

```c
int phytcpdial(const char *host, const char *port, int timeout_ms);
```

`host` is an explicit, nonempty name or IPv4/IPv6 address. `port` is a decimal
string in 1–65535. `timeout_ms` is a positive connect budget, shared by all
resolved addresses. Hostname resolution can block before this budget starts.

Success returns an owned connected socket with close-on-exec, nonblocking
mode and `TCP_NODELAY`. Failure returns `-1` and frees all temporary sockets.
Call on a launch thread. Close the returned socket or transfer ownership to
[`phyposixfd`](phyposixfd.md), for example by passing it with
`PHY_STREAM | PHY_ORDERED | PHY_RELIABLE`. Both endpoints choose the Styx or
P9MX protocol separately after opening the carrier.

[Listen for a peer](phytcplisten.md) · [Native FlashVM commands](https://github.com/bhgv/flashVM/blob/master/flashVM/doc/STYX_RPC_PLAYER.md#start-over-tcp)
