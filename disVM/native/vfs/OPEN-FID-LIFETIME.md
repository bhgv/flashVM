# Open remote files outlive namespace bindings

[Русский](OPEN-FID-LIFETIME.ru.md) · [Mount retirement](MOUNT-RETIREMENT.md) · [Client I/O](mount_io.c)

A pathname selects an object when it is opened. The resulting native
`DVM_VfsHandle` retains a remote opened fid and its client/provider lifetime.
Unbinding the pathname, destroying the calling namespace or releasing the factory
reference does not reopen that name or invalidate the retained open.

The [wire regression](../tests/mount/detached.c) mounts a real connected Styx
provider at `/remote`, resolves and opens `/remote/first`, unbinds `/remote` and
destroys the namespace. The [main client](../tests/mount/client.c) keeps that
handle while the server changes/unpublishes names; after releasing the last caller
factory reference it reads the same opened object and closes it. Seven real
opened handles must close on the server exactly once. The server runs in another
process and requests cross a real TCP stream with fragmented framing.

Other existing wire cases check short confirmed writes, unknown effects after a
lost write reply, readonly ceilings, explicit clunk and connection loss. Confirmed
byte counts remain authoritative; an uncertain write is not replayed. A broken
connection is an error, not fabricated EOF or a successful close acknowledgement.
The tests use the ordinary 9P messages and do not add a descriptor-address format.

```sh
cmake --build <native-build> --target vfs-mount-test vfs-mount-deep-server vfs-file-wire-test server-loopback-test vfs-peer-wire-test
ctest --test-dir <native-build> -R '^vfs-mount-(basic|partial|loss|retirement-(clunk|disconnect)|prepared)$' --output-on-failure
```

`<native-build>` is a configured `disVM/native` build with explicit matching
Inferno source and VFS support paths. It is distinct from the application runtime
build. This verifies the existing native Styx/VFS opened-object path. It does not
implement automatic marshalling of arbitrary Limbo `Sys->FD` values through a
remote module call, or process migration. Those integrations remain separate work.
