# disvm --startup-fd

[Русский](STARTUP-FD.ru.md) · [Owner launcher](../native/terminal/LAUNCH-PROCESS-PAYLOAD.md) · [Reader](host_runtime/execution.inc)

`disvm --startup-fd=FD [VM options] MODULE.dis`

For Linux native launchers. `FD` is a required decimal inherited descriptor
from 3 through the native signed integer limit. Supply one sealed regular
file, typically a Linux memfd with write, grow and shrink seals. Its length
is the payload size in bytes, up to 65,536. An empty file supplies an empty
payload. This option is unavailable together with a native launch handoff
or the legacy hex option.

DisVM reads from offset zero, takes ownership of the delegated descriptor,
and closes it before dispatch. The startup context holds a copy for
`Launchcontext.arguments()`. The option can wait for file I/O. Invalid
descriptors, missing seals, excessive size and read failures stop startup.
The example launcher obtains the FD from the owner-configured Styx route.
See the [runnable TCP example](../examples/vfs-run/README.md).
