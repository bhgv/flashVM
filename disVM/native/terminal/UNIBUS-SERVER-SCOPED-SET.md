# styxunibusserverstartscopedset

[Русский](UNIBUS-SERVER-SCOPED-SET.ru.md) · [Header](unibus_server.h) · [Implementation](unibus_loaded.c)

```c
StyxUnibusServer *styxunibusserverstartscopedset(RemotePhy *channel,
    const char *module, const char *const *names, size_t count, DVM_Provider *reverse);
```

Include `unibus_server.h` and link `styx_unibus_server`.
Use the same publication and private mailbox setup as
[styxunibusserverstartloadedset](UNIBUS-SERVER-LOADED.md).
Additional compiled value and reference profiles require
a caller lease, and their Dis exports receive the private `/.prog/.self`
namespace view during execution.

| Parameter | Contract |
| --- | --- |
| `channel` | Forward Styx transport. Consumed on both success and failure. |
| `module` | Directory name below `mounted`. Used during publication. |
| `names` | Array of export names. Each name is used during publication. |
| `count` | Number of names, from 1 through 8. |
| `reverse` | Non-null native provider for the same client's reverse Styx session, rooted above `.prog`. Retained through join. |

The function returns an owned server on success or NULL on invalid setup
or failure to start the dispatcher. It starts the native dispatcher without
waiting for the owner image. The usual
[load-offer handshake](UNIBUS-SERVER-LOAD-PROFILE.md) gates access to the
directory's profile. Configure a bidirectional client with
`styxcallermount` before publishing its forward mount. The server validates
the reverse grant before giving the owner VM a request. A missing
reverse binding prevents ordinary Limbo `load` of scoped exports.

The dispatcher serializes requests on its forward session. A grant is
valid for one invocation, including reverse reads before the reply.
A new reverse operation through a saved fid is refused after the lease
ends. Close the carrier and call `styxunibusserverjoin` to reap the server.
The scoped profiles select a private Sysevents inbox for the duration
of the call. An opened event FD can drain its queue and receive the
exit event and EOF after the reply. The other direction's client event
stream is independent.

See the [runnable example](../../examples/remote-scalar/STYX.md#call-with-a-reverse-styx-mount)
and the [wire format](SCALAR-FUNCTION.md#caller-context-for-compiled-calls).

For multiple modules, see [styxunibusserverstartscopedtable](UNIBUS-SERVER-SCOPED-TABLE.md).
