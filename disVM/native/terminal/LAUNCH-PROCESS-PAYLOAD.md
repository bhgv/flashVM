# styxlaunchprocesspayload

[Русский](LAUNCH-PROCESS-PAYLOAD.ru.md) · [API](launch_process.h) · [Process configuration](LAUNCH-PROCESS.md) · [Example](../../examples/vfs-run/README.md)

`int styxlaunchprocesspayload(StyxLaunchProcess *process, size_t limit);`

Link `styx_terminal`. Call on an initialized, owner-configured process before
publication. The executable must support the DisVM startup hex option. The
read/write capabilities and working directory come from the existing host
configuration. `limit` is a byte count from 0 to 32,768. A successful call
returns 1. Invalid, spent, or oversized configuration returns 0. The function
borrows process storage and does not start a child.

For example, call `styxlaunchprocesspayload(&publication.process, 3)` before
publishing the route. A subsequent `Vfsctl.runargs` request carries its bytes
into `Launchcontext.arguments()` before guest dispatch. The adaptation uses
a child command-line argument, so local process inspection may expose the
encoded bytes. The transport can impose a smaller limit.
