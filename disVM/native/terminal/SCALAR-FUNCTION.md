# styxunibusserverstartfunction

[Русский](SCALAR-FUNCTION.ru.md) · [Header](unibus_server.h) · [C example](tests/scalar/route.c) · [Limbo example](../../examples/remote-scalar/styx-mounted.b)

```c
StyxUnibusServer *styxunibusserverstartfunction(
    RemotePhy *channel, const char *module, const char *name,
    const struct StyxScalarFunction *function);
```

Link `styx_unibus_server`. Include `unibus_server.h` and `scalar_function.h`.
The function takes ownership of the Styx transport channel even on failure.
It returns an owned server or NULL. After closing the peer, consume the
server with `styxunibusserverjoin`.

The module and export names are single path components. The server publishes
`mounted/<module>/<name>` with mode 0777 and allows ORDWR to invoke it. It copies
the function descriptor. The caller keeps the callback context alive until
join. Both tags are nonzero unsigned 32-bit values. The callback runs on the
Styx dispatcher thread, so use this endpoint with synchronous native methods
that complete promptly. Each open has independent response state. Callbacks
on the same server are serialized.

Write one complete [scalar S-record](SCALAR-WRITE.md) at offset zero. Read from
offset zero, advancing the read offset by the confirmed byte count. A result
or method error is encoded as one record. A malformed request, a read before
the request, or a second accepted-call attempt on the same fid gets a Styx
error. Closing the fid discards its response. The endpoint uses the existing
scalar codec.

With request tag `STYX_FUNCTION_REQUEST_TAG` (72) and reply tag
`STYX_FUNCTION_REPLY_TAG` (73), the server also publishes a read-only
`.interface` profile. It describes the single named export with the compiled
signature of `fn(int): int`. DisVM checks that profile when a typed `load`
selects the directory on a cancellable VFS mount. It captures the export
through that directory and opens a fresh read/write fid for each call.
A `bind` alias of the directory works the same way. Other tag pairs continue
to work for explicit C clients.

Build and run `terminal-scalar-route` from the native SDK. The example opens
`mounted/compute.dis/compute` twice, submits two scalar calls and reads the
responses in reverse order. It checks that a truncated frame and a duplicate
write do not run the callback. Run `ctest -R '^terminal-scalar-guest$' -V`
with the SDK configured for the hosted DisVM and Limbo compiler. The Limbo
example checks a wrong compiled import, a result and an exception through
a direct mount and a `bind` alias. It covers the original mount with an empty
directory to verify that both loaded instances retain their captured export.
Expected output includes
`PASS typed Styx mounted function` for each VM mode. See the [paired DisVM tutorial](../../examples/remote-scalar/STYX.md)
for the existing typed route.

For an original Dis implementation, see [styxunibusserverstartdis](SCALAR-FUNCTION-DIS.md).

## Caller PID on a typed call

A native export can set `caller_method` with request tag 74 and reply tag 73.
Its `.interface` entry uses tag 75. The request is one S-record containing
two big-endian 32-bit words, the nonzero caller PID and the signed argument.
The reply uses the usual scalar format. Legacy entries with tag 72 retain
their original request format.

DisVM reads the active process PID for each call. The PID is scoped to the
client session.

For context-aware native dispatch, set `program_method` instead of
`caller_method` and supply `program_root`, the granted module directory on
the reverse Styx mount. Keep that root alive until the server joins.
The dispatcher captures `.prog/<pid>` before calling the method and gives
the method that exact directory as its private `.self` root. It can open
children such as `.pid`, `.fd`, `.vfs` and `.var` as granted by that root.
The reference is borrowed for the callback. A failed lookup or a non-directory
returns a Styx permission error before dispatch. Each invocation has its
own capture. Automatic publication of the client VM's live tree by
`load` remains pending.

The native reverse Styx example in `terminal-scalar-route` selects two
directories by PID and refuses a third, absent PID. Expected output includes
`PASS mounted scalar .self over reverse Styx`.

The `styx-caller.b` example runs as part of `terminal-scalar-guest`. The
parent and a spawned child share one module instance. The native callback
checks the automatically supplied PID against the PID observed by each guest
process. Both VM modes print `PASS typed Styx caller PID`.

## Building the reverse caller tree

Native DisVM embedders can assemble the module root with
`dvm_program_view`. Supply one `DVM_ProgramView` per process. Each entry
creates a read-only decimal `.pid` file with a trailing newline and maps
only the supplied `.fd`, `.vfs` and `.var` directory grants. The mapping
retains access to the underlying providers with their original permissions.
A duplicate PID or invalid directory rejects the entire construction.
The embedder owns the result and can use it as the root of its reverse
publication. A captured PID directory stays valid after releasing that root.

Run `ctest -R '^terminal-scalar-guest$' -V` to exercise construction,
capture lifetime and grant visibility. Look for
`PASS captured caller program tree`. The builder still requires the caller
entries and grants from its embedder.

For a persistent reverse export, `dvm_program_publication` returns an
empty module root and update control. Keep the root mounted while calling
`dvm_program_publication_replace` with the current entries. A successful
update changes what fresh lookups from the module root see. A zero count
removes all entries. Failed updates and existing captures keep their
previous contents. An embedder still supplies the updates.

## Publishing the active caller at invocation

An embedder can install a trusted `DVM_CallerBinding` on a fresh native
mount provider with `dvm_provider_bind_caller` before publishing the mount.
Captured function nodes inherit that binding. During the ordinary typed
call, DisVM builds a tree for the active caller and calls `begin` before
sending the request. The tree contains `.prog/<pid>/.pid`. The exact
function provider and tree are borrowed during the callback. The lease
lasts through the exchange and `end` consumes it. A refused lease prevents
the request from being sent. Unbound mounts continue to use the existing
PID record.

The guest fixture uses two Styx sessions on one multiplexed carrier.
The forward session carries the function request. The provider walks and
reads `.prog/<pid>/.pid` on the reverse session, which serves the virtual
file created by DisVM. The parent and its spawned child call the same
loaded instance. Expected output includes `PASS typed Styx caller PID`
with JIT enabled and disabled.

Before publishing the caller tree, DisVM captures explicit directory
mounts at `/.fd`, `/.vfs` and `/.var` in the current caller's namespace.
Missing mounts are omitted. A union or a mount that cannot be captured
cancellably prevents submission. The fixture verifies `.vfs/env/CLIENT` and `.var/counter` through reverse
Styx for both callers.

The fixture installs the reverse binding. Production mount setup and
automatic projection of the VM's open descriptor table, namespace and
shared variables into those grants remain open.

When the caller has no explicit `/.fd` mount, DisVM projects the
numbered descriptors imported at process launch. The exported alias
retains the open slot, uses its original read/write mode, and shares its
cursor with local aliases. The guest fixture imports FD 3 at launch. The
provider reads two consecutive blocks over reverse Styx on the parent
and child's calls. The parent then reads the third block locally with
`Sys->fildes(3)` and acknowledges this read with one more typed call.
This supports the existing numbered launch
range 0–31. Host stdio and unnumbered managed opens are outside this
projection. The reverse mount setup still requires an embedder.
