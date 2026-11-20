# styxlaunchprocesspayload

[Русский](LAUNCH-PROCESS-PAYLOAD.ru.md) · [API](launch_process.h) · [Process configuration](LAUNCH-PROCESS.md) · [Example](../../examples/vfs-run/README.md)

`int styxlaunchprocesspayload(StyxLaunchProcess *process, size_t limit);`

Link `styx_terminal`. Call on an initialized, owner-configured process before
publication. The executable must support the DisVM sealed startup FD option. The
read/write capabilities and working directory come from the existing host
configuration. `limit` is a byte count from 0 to 65,536. A successful call
returns 1. Invalid, spent, or oversized configuration returns 0. The function
borrows process storage and does not start a child.

For example, call `styxlaunchprocesspayload(&publication.process, 3)` before
publishing the route. A subsequent `Vfsctl.runargs` request carries its bytes
into `Launchcontext.arguments()` before guest dispatch. The launcher delegates a sealed Linux memfd
and puts only its descriptor number in argv. DisVM reads it from offset zero
and closes it before guest dispatch. The transport can impose a smaller limit.
See [`--startup-fd`](../../runtime/STARTUP-FD.md).
