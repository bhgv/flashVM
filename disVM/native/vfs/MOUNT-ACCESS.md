# Access checks on a mounted Styx directory

[Русский](MOUNT-ACCESS.ru.md) · [Mount provider](MOUNT.md) · [Provider interface](../../runtime/vfs/provider.h#L190) · [Mount implementation](mount.c#L166)

A directory mounted from another VM remains subject to the host access
ceiling and to the serving VM's decisions. The mount needs both
`FVM_ACCESS_READ` and `FVM_ACCESS_EXEC` in the host ceiling to open or
probe a directory for listing. A read-only ceiling without search
permission can still obtain metadata and open a readable regular file.
A read-and-search ceiling is sufficient for directory listing when
the peer permits it.

| Request to the mount provider | Result |
| --- | --- |
| `dvm_provider_info` | Metadata with mode bits masked by the host ceiling. |
| `dvm_provider_access` with read, search or both | With sufficient host rights, walk and stat a temporary fid. For search, also require an advertised execute bit. Ask the peer to open a directory read-only and retire the temporary fid. |
| `dvm_provider_access` with write or on a regular file | No generic access probe. Use the ordinary open operation. |
| `dvm_provider_open` | Obtain the exact open fid if both the host ceiling and the peer allow the operation. |

A missing host directory permission is rejected before any probe
traffic. A peer refusal to open a directory remains a failure even
when its metadata advertises read and search. The next real open
or walk is independent of a previous successful probe and can fail if
the namespace or permissions changed. The probe does not retain a
directory cursor or issue a write.

## Check the two transports

In the native CMake build used by DisVM, build `vfs-mount-test`,
`server-loopback-test` and `vfs-mount-deep-server`. Run:

```sh
ctest -R '^vfs-(mount|mux-provider)-(access|deep)$' --output-on-failure
```

The access scenarios include a readable directory, unsupported
general file probes and a restricted host provider on the same
connection. The driver counts remote opens. The deep hierarchy
scenarios additionally verify that a directory advertising read and
search can still refuse a read-only open on the server. Both legacy
callback and multiplexed fd-owner transports are exercised.
