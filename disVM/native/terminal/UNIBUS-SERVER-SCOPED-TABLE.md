# styxunibusserverstartscopedtable

[Русский](UNIBUS-SERVER-SCOPED-TABLE.ru.md) · [Header](unibus_server.h) · [Implementation](unibus_loaded.c)

```c
StyxUnibusServer *styxunibusserverstartscopedtable(RemotePhy *channel,
    const StyxMountedScalar *entries, size_t count, DVM_Provider *reverse);
```

Include `unibus_server.h` and link `styx_unibus_server`. The server publishes
several loaded Dis modules on one forward Styx session. Each directory under
`mounted` has its own `.interface`, built from that module's exports.

| Parameter | Contract |
| --- | --- |
| `channel` | Forward Styx transport. Consumed on success and failure. |
| `entries` | Borrowed publication table. Set `module` to the directory name and `name` to the export name. The loaded Dis implementation determines the call profile. |
| `count` | Number of entries, 1 through 64, with up to 8 entries per module. |
| `reverse` | Required provider rooted above the client's `.prog`. Retained until join. |

The function returns an owned server, or NULL on invalid input, allocation
failure or dispatcher startup failure. The native dispatcher starts
without waiting for the owner VM. The first profile access waits for the
load offers. Use the indexed load-offer, request and reply mailboxes in
the same order as `entries`.

Start the owner dispatcher with tab-separated arguments beginning with
`loaded-table`. Follow it with one group of five fields per entry:
the original Dis implementation path, export name, request path, reply
path and offer path. Exports with the same implementation path share
one loaded data instance. Independently loaded implementations keep
their own module data. Each scoped caller uses one provider `Prog` across
the table. A revoked call retires that `Prog`, and the next call from the
same PID creates a replacement. Close the carrier and join the server
to release the session.

The [two-module fixture](tests/scalar/pair.c) publishes the table.
The [Limbo client](../../examples/remote-scalar/styx-scoped-calls.b)
checks that calls across both modules reuse the executor and that
cancellation replaces it.

## Share the session with a launch endpoint

`styxunibusserverstartscopedlaunchtable` adds `implementation.dis`,
the root `.interface`, `.unibus`, and `.exit` to the same session.
Pass a `StyxUnibusLaunch` containing the nonzero launch S-record tag,
the original image provider and the launch declaration provider.
The server retains both providers until join. The module-specific
interfaces remain under `mounted`.

Set `persistent` on `StyxUnibusLaunch` for a bounded queue of scoped
requests. The host takes each record with `styxunibusservernextlaunch`
and reports through `styxunibuslaunchcomplete`. The ticket belongs to
that request, even if later requests finish earlier. A closed fid
discards its queued request before dequeue. An already running child
can finish independently. The legacy write-only fid has one status
slot at `.exit`.

With `persistent` clear, retain the one-shot inbox and
`styxunibusserverlaunchstatus`. The [wire test](tests/scalar/route_launch.inc)
checks independent fids, reverse completion order and the legacy result.
