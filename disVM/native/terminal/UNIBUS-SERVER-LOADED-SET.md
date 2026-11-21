# styxunibusserverstartloadedset

[Русский](UNIBUS-SERVER-LOADED-SET.ru.md) · [Header](unibus_server.h) · [Implementation](unibus_loaded.c)

```c
StyxUnibusServer *styxunibusserverstartloadedset(
    RemotePhy *channel, const char *module, const char *const *names, size_t count);
```

Include `unibus_server.h` and link `styx_unibus_server`. Supply an owned
connected Styx channel, the borrowed directory name inside `mounted`,
and an ordered list of one to eight distinct export names. Each name
is one VFS component of up to 48 UTF-8 bytes. `.interface` is reserved.
The names and array need to remain accessible through this call. The
constructor consumes the channel on every outcome and returns an owned
server or NULL on invalid input or startup failure.

Grant the owner VM a request reader, reply writer and
[profile writer](UNIBUS-SERVER-LOAD-PROFILE-AT.md) per index. Start
[the shared owner service](../../runtime/SCALAR-OWNER-SERVICE.md#one-load-for-several-exports)
with the same order. The service gets the types and signatures from the
original module. Both the function files and the read-only `.interface`
admit opens only after all offers pass validation. One failed offer
refuses the entire publication. An open waits at most three seconds for
the set. Timeout permits a later open. Accepted offers retain their
order and are immutable until shutdown.

After the carrier closes, use `styxunibusserverjoin`. See the
[two-VM example](../../examples/remote-scalar/STYX.md#load-six-exports-that-share-state)
for the setup, expected results and refusal case.
