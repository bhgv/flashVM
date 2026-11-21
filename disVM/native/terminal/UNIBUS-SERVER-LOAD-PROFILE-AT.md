# styxunibusserverloadprofileat

[Русский](UNIBUS-SERVER-LOAD-PROFILE-AT.ru.md) · [Header](unibus_server.h) · [Implementation](unibus_loaded.c)

```c
DVM_Provider *styxunibusserverloadprofileat(StyxUnibusServer *server, size_t index);
```

Include `unibus_server.h` and link `styx_unibus_server`. Pass a live
[load-backed server](UNIBUS-SERVER-LOADED-SET.md) and a zero-based export
index in the original names array. The call returns immediately. It
returns a borrowed provider until join, or NULL for an invalid index,
an absent server or a server with declared profiles.

Mount its root writable in the owner's namespace and give that path to the
corresponding `offer` field in the startup configuration. Each slot accepts
one complete profile from the shared owner. Index zero is also available
through [styxunibusserverloadprofile](UNIBUS-SERVER-LOAD-PROFILE.md). See the
[test launcher](tests/scalar/pair.c) for the grant setup.
