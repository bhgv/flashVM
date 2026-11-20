# styxunibusserverstartmounted

[Русский](SCALAR-FUNCTION-MOUNTED.ru.md) · [Header](unibus_server.h) · [Example launcher](tests/scalar/pair.c)

```c
StyxUnibusServer *styxunibusserverstartmounted(
    RemotePhy *channel, const StyxMountedScalar *entries, size_t count);
DVM_Provider *styxunibusservermailbox(
    StyxUnibusServer *server, size_t index, int reply);
```

Include `unibus_server.h` and link `styx_unibus_server`. The constructor
consumes the connected channel on every outcome. Pass 1 to 64 entries with
distinct module directory names. Each entry names one function. Directory and
function names are single path components, and `.interface` is reserved.
The input table is borrowed during startup. Native method contexts are
borrowed until join. Invalid input or setup failure returns NULL and
publishes no partial session.

A NULL `native` field sets up a separate request and reply pair for a Dis
service. The mailbox accessor uses the entry's zero-based index. Pass 0 for the
request reader or 1 for the reply writer. The result is borrowed through join.
It is NULL for a native entry, invalid server or out-of-range index. See
[service configuration](SCALAR-FUNCTION-DIS.md) for the paths and tags to
supply to the owner.

The resulting Styx root has one `mounted` directory. Each module has its
own function file and, for typed endpoints, its own read-only `.interface`.
The common dispatcher admits requests sequentially. Each opened function fid
retains its own response. When the carrier closes, all Dis request mailboxes
are sealed. Join with `styxunibusserverjoin` to release the server.

Run `ctest -R '^terminal-scalar-owner-guest$' -V` in an SDK configured for
the hosted VM and Limbo compiler. The sample uses one Styx mount for a native
helper and an original Dis export. It checks both results and rejects attempts
to load either directory with the other's compiled import.
