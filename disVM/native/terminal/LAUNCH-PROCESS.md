# Styx owner execution

[Русский](LAUNCH-PROCESS.ru.md) · [API](launch_process.h) · [Receiver](LAUNCH-RECEIVE.md)

The POSIX `styx_terminal` library provides `StyxLaunchProcess` for a published,
one-shot command. The host authorizes the Styx write and chooses the executable,
working directory and argument vector.

## styxlaunchprocessinit

Call `styxlaunchprocessinit(&process, tag, directory, argv)` with unused storage,
a nonzero 32-bit route tag, a working directory and a null-terminated argument
vector. The first argument is an absolute executable path. Keep the strings
and vector live until submission and the process object at a stable address
until retirement. Returns 1 on success or 0 for invalid inputs. No process
starts during initialization.

Delegate the write to `styxterminallaunchreceive(&process.receiver, offset, bytes,
size)`. An accepted frame starts the child and acknowledges scheduling.
The child inherits the owner's environment. Mark private host descriptors
close-on-exec. Calls on the same process object must be serialized.

## styxlaunchprocessstatus

Call `styxlaunchprocessstatus(&process, wait, &status)` after submission. Set
`wait` to zero to poll, or nonzero to wait. The result is 1 with a cached child
status, 0 before completion or submission, or -1 on invalid arguments or wait
failure. Exit status is 0–255 for a normal exit, or 128 plus the signal for
signal termination. Child setup failure exits 126 for the directory and 127
for executable failure. Reap the child before releasing process storage.
