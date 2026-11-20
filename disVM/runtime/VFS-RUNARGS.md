# Vfsctl.runargs

[Русский](VFS-RUNARGS.ru.md) · [Overview](VFS-LAUNCH.md) · [Tutorial](../examples/vfs-run/TUTORIAL.md) · [Source](vfs/vfsnative.c)

## Synopsis

```limbo
include "vfsctl.m";
# Load Vfsctl and call its init() before using runargs.
runargs: fn(path: string, arguments: array of byte): int;
```

Available in hosted DisVM with [Vfsctl](../module/vfsctl.m) and `$Vfsnative`.
`path` is required and follows the same absolute, relative and bind rules
as [`run`](VFS-RUN.md), with a maximum of 4095 UTF-8 bytes. `arguments`
contains zero to 65,536 bytes. `nil` and an empty array send an empty payload.
Embedded zero bytes are preserved. The owner callback and transport can impose
smaller limits. Fixed-command publications accept empty payloads by default.
The [owner can opt in](../native/terminal/LAUNCH-PROCESS-PAYLOAD.md) to deliver
data to the command’s `Launchcontext.arguments()`.

## Result and effects

| Result | Meaning |
| --- | --- |
| `1` | Delivery of the complete request was acknowledged. |
| `0` | Invalid input, unavailable route or a definitive refusal. |
| `-1` | The transport could not establish whether the request was delivered. |

The runtime copies the array before waiting for provider I/O. The caller keeps
ownership of the array. The owner callback borrows the received bytes for the
duration of the callback. An owner that retains them must take a copy.
Cancellation applies to the transport wait and never triggers a retry.
The owner callback controls how it uses the payload.

## Example

```limbo
data := array[] of {byte 'x', byte 0, byte 'y'};
delivery := vfs->runargs("/payload.dis", data);
```

See the [runnable TCP client](../examples/vfs-run/runexample.b) and
[owner receiver](../native/terminal/LAUNCH-RECEIVE.md).
