# Run the mounted command and read its status

[Русский](TUTORIAL.ru.md) · [Build and run](README.md) · [Overview](../../runtime/VFS-LAUNCH.md)

1. Build and run the TCP fixture with the [README commands](README.md). It
   creates a temporary workspace, builds the Limbo modules, and publishes the
   command and result files from its Styx server.
2. Follow [`runexample.b`](runexample.b). `execute` loads `Sys` for the file
   operations and `Vfsctl` for route submission. `vfs->init()` opens access to
   the hosted builtin. `vfs->run(command)` resolves the command in the calling
   namespace and sends one empty request to its owner.
3. The optional branch calls `vfs->runargs(payloadroute, payload)`. In this
   fixture the separate owner callback checks each byte of `x\0y`. Pass
   `nil` for `payloadroute` to skip this branch.
4. `delivered` raises on definitive refusal or an uncertain delivery. The
   example leaves retry decisions to the caller because a lost acknowledgement
   can follow an already admitted request.
5. `exitstatus` opens the result stream for reading and accumulates decimal
   digits until newline. An unavailable file, premature EOF or malformed
   record raises an exception. The local FD reference is released when the
   routine returns or unwinds.

The owner prints `PASS owner Dis init` after reading its working-directory
file. The caller expects a zero exit code and prints
`PASS path-only Styx command submission`. CTest requires both the interpreter
and JIT variants to complete. The harness closes the peer connections, waits
for its processes and removes the temporary files.

See [`run`](../../runtime/VFS-RUN.md) and
[`runargs`](../../runtime/VFS-RUNARGS.md) for their full result contracts.
