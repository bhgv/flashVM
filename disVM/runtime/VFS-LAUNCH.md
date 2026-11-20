# Mounted Dis command execution

[Русский](VFS-LAUNCH.ru.md) · [Tutorial](../examples/vfs-run/TUTORIAL.md) · [DisVM](../README.md)

A launched application can refer to a command by the name visible in its VFS.
This lets the host arrange execution near the files and services owned by the
publishing DisVM. A binding can change the visible name without changing the
selected provider's launch route.

The client resolves the name, obtains the selected provider's route and submits
a Styx write. The owner selects the command and its working directory. A separate
read-only provider can expose the resulting exit code through ordinary
`Sys->open` and `Sys->read`. The owner's result publication can outlive the
original call.

| Operation | Use |
| --- | --- |
| [`Vfsctl.run`](VFS-RUN.md) | Submit a command with an empty payload. |
| [`Vfsctl.runargs`](VFS-RUNARGS.md) | Submit binary data to a route configured to receive it. |
| [Owner publication](../native/terminal/LAUNCH-PUBLICATION.md) | Publish a fixed command and its exit status. |

The TCP example exercises a bound route, owner-side `init` and exit-status
reading in interpreter and JIT. It also sends binary bytes to a separate
callback. Forwarding those bytes into the command's `init`, ordinary command
integration and transparent remote library calls remain open.
