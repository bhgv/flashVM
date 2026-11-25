# phytcpaccept

[Русский](phytcpaccept.ru.md) · [Declaration](../../remote_phy/drivers/posix/tcp.h) · [Source](../../remote_phy/drivers/posix/tcp.c)

Use `<styx/phy/drivers/posix/tcp.h>` with `Styx::carrier` in a hosted Linux64 build.

```c
int phytcpaccept(int listener);
```

`listener` is a borrowed blocking TCP listener, for example from
[`phytcplisten`](phytcplisten.md). Waits for one peer and retries interrupted
accepts. Run on a launch thread. Returns an owned connected nonblocking
close-on-exec socket with `TCP_NODELAY`, or `-1` on failure. Ownership of
`listener` stays with the caller, which may accept again or close it. The
returned stream can be adopted by [`phyposixfd`](phyposixfd.md).
