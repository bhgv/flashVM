# styxunibusserverstartdis

[Русский](SCALAR-FUNCTION-DIS.ru.md) · [Header](unibus_server.h) · [Example launcher](tests/scalar/pair.c)

```c
StyxUnibusServer *styxunibusserverstartdis(
    RemotePhy *channel, const char *module, const char *name);
```

Include `unibus_server.h` and link `styx_unibus_server`. The constructor
takes ownership of the connected Styx channel on every outcome. It returns an
owned server or NULL. Both names are individual path components. The server
publishes one module directory under `mounted`, with a read-only `.interface`
and one `fn(int): int` function file. The export name must match the one
configured for the owner service.

Use `styxunibusserverinbox(server)` as the local request reader and
`styxunibusserverreply(server)` as the local reply writer. They are borrowed
providers valid until join. Mount them in the owner's namespace and start
the [shared scalar owner service](../../runtime/SCALAR-OWNER-SERVICE.md) with
those paths, tags 72 and 73, and the path and export name of the original
Dis module. Use call count zero to serve until the peer closes the carrier.
The client mounts the Styx root and uses ordinary typed `load` on
`mounted/<module>` relative to that mount.

The dispatcher admits one call at a time to the service and keeps each
completed response in its own opened Styx fid. It waits up to three seconds
for the owner's reply. An ambiguous exchange retires the mailbox route and
returns a Styx error without replaying the request. The dispatcher seals the
request mailbox on carrier shutdown so the owner can exit when idle. Close
the carrier before joining the server with `styxunibusserverjoin`.

With the SDK configured for the hosted DisVM and Limbo compiler, run
`ctest -R '^terminal-scalar-owner-guest$' -V`. The mounted case runs
[the Limbo client](../../examples/remote-scalar/styx-mounted.b) and the
shared owner in separate DisVM processes. Expect
`PASS mounted Styx original Dis` in interpreter and JIT modes. The result
depends on the owner's private file and persistent state, and the example
checks exception propagation.

To publish several module directories on one channel, use [styxunibusserverstartmounted](SCALAR-FUNCTION-MOUNTED.md).

## Full-width Dis export

To publish one original `fn(big): big` export, use
`styxunibusserverstartmounted` with a NULL `native` field and
`dis_big = 1`. Mount the indexed request and reply mailboxes in the
owner's namespace. Start the same scalar owner service with the
tab-separated fields `big`, implementation path, export name, request
path, reply path, `76`, `73`. The service links the original export once
with its compiled signature and serves requests until EOF. The caller
uses the ordinary Limbo interface and `load`.

The [caller](../../examples/remote-scalar/styx-wide-owner.b) and
[owner](../../examples/remote-scalar/wide-owner.b) exercise the signed
limits, an exception and persistent instance state in
`terminal-scalar-owner-guest`.
