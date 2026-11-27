# Run a cancellable C file provider and a mounted Limbo caller

[Русский](DEFERRED_IO.ru.md) · [Provider model](../../DEFERRED_IO.md) · [Examples](../README.md)

1. Configure the SDK with `STYXRPC_EXAMPLES=ON`, `BUILD_TESTING=ON` and
   `CMAKE_PREFIX_PATH` pointing to the installed Styx and Unibus packages.
   To add the Limbo caller, set `STYXRPC_DISVM_SOURCE` and
   `STYXRPC_DISVM_BUILD` to a matching native DisVM source and terminal
   launch build. The build needs `disvm` and `bootstrap/limbo-bootstrap`.
2. Build and run the hosted tests.

   ```sh
   cmake --build build-styxrpc
   ctest --test-dir build-styxrpc -R 'styxrpc-(disvm-)?deferred-' --output-on-failure
   ```

The [C provider](deferred_device.c) installs a selector for its byte file
`data` and the writer returned by creating `new`. Each open has its own
cookie. The read callback waits for permission from the owner and the write
callback confirms one byte before waiting at offset one. The synchronous
`gate` file reports admission counters and accepts commands to release reads,
pause new reads and let cancelled callbacks finish. It remains reachable
while several workers wait.

The [C caller](deferred_client.c) launches a read and a two-byte write. It
cancels the write after the first byte is confirmed and closes that fid before
letting the provider worker exit. It confirms that the read finishes, cancels
a separate read, opens the file again and creates a new file with a worker-backed
initial writer. It leaves a final read pending while disconnecting. The
capacity case fills all sixteen slots. It checks that a flushed worker
still occupies its slot until it exits, then uses the freed slot for a new read.

The [Limbo caller](deferred.b) exercises the same C provider via standard
`Sys.read` and `Sys.write` on a mount at `/device`. The parent waits for
both callbacks to enter and calls `Processctl.cancelwait` on the writer.
It checks the first byte is still confirmed, lets the other child finish,
then cancels a fresh reader. It opens the file again and checks the writer
returned by `Sys.create`.

The [driver](deferred.py) provides the two endpoints of a Unix socket pair
or a connected loopback TCP stream. The four DisVM cases use interpreter and
JIT modes on both carriers. A successful client prints `PASS`. Before
exiting, the provider checks exact cancellation counts and that all its file
cookies were released.
