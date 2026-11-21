# styxunibusserverstartloaded

[Русский](UNIBUS-SERVER-LOADED.ru.md) · [Header](unibus_server.h) · [Implementation](unibus_loaded.c)

```c
StyxUnibusServer *styxunibusserverstartloaded(
    RemotePhy *channel, const char *module, const char *name);
```

Include `unibus_server.h` and link `styx_unibus_server`. Supply an owned
connected Styx channel and two borrowed path components. `module` names
the directory inside `mounted`; `name` names the function and may contain
at most 48 UTF-8 bytes. The names need to remain accessible through the
call. The constructor consumes `channel` and returns an owned server or
NULL on invalid input or startup failure.

The constructor reserves one export. The embedder grants the owner VM
the private [load profile mailbox](UNIBUS-SERVER-LOAD-PROFILE.md) plus the
request reader and reply writer from `styxunibusservermailbox`. Start
[the shared service](../../runtime/SCALAR-OWNER-SERVICE.md#profile-from-the-original-load)
in the owner's namespace with those three paths. See the
[paired launcher](tests/scalar/pair.c) for the setup.

Opening the function or its `.interface` can wait up to three seconds
for the owner to link it. A timeout permits another open. A refused or
malformed offer refuses later opens in the same session. An accepted
offer determines the dispatch profile for the life of the server.
After the carrier closes, call `styxunibusserverjoin` to release it.

For declared exports and multi-export directories see
[styxunibusserverstartmounted](SCALAR-FUNCTION-MOUNTED.md).

For multiple exports, use
[styxunibusserverstartloadedset](UNIBUS-SERVER-LOADED-SET.md).

See [styxunibusserverstartscopedset](UNIBUS-SERVER-SCOPED-SET.md) for caller namespaces on shaped value calls.
