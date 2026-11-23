# Calling an independent C device

[Русский](README.ru.md) · [libstyxrpc](../README.md)

Build with `-DSTYXRPC_EXAMPLES=ON` and an installed hosted Styx library.
From the `styxrpc` source directory, assuming the build directory is `build`:

```sh
python3 examples/socketpair.py build/styxrpc-device build/styxrpc-client
```

This starts two C processes on opposite ends of a socket. The device publishes
`sample` under `/mounted/device.dis`, opens one call state per fid and runs
the increment callback once per complete request. The caller opens a new fid
for each of two calls and checks both replies. Success prints
`PASS native C calls standalone C device`. Each reply is consumed before clunk.
An unconfirmed transfer fails the test and the launcher closes both processes.

To exercise the same service from DisVM, run the following with paths to a
built DisVM, the Limbo bootstrap and the upstream Limbo module directory:

```sh
python3 examples/disvm/run.py --runtime /path/to/disvm \
    --limbo /path/to/limbo-bootstrap --module /path/to/upstream/module \
    --device build/styxrpc-device
```

Expect `PASS Dis load of standalone C device`. Add `--jit` for the JIT.
The client also reads the `value` byte, writes a new value and reads it again.
The launcher passes a connected descriptor with
`--styx-mount-fd FD --raw --at /device`. Here `--raw` selects a forward-only
Styx mount for this endpoint. The launcher owns timeouts and reaps both
processes. The Limbo caller is compiled before the device is started.

## Launch DisVM from C

```sh
python3 examples/disvm/launch.py --runtime /path/to/disvm \
    --limbo /path/to/limbo-bootstrap --module /path/to/disVM/module \
    --driver build/styxrpc-launch
```

The C client selects the existing bidirectional carrier framing and mounts its
forward Styx stream. It checks the raw launch profile, sends startup bytes on
`.unibus` and polls `.exit` over the same connection. The script runs one
successful and one failing program per VM execution mode. Status 2 means a
confirmed success and 3 a confirmed program failure.
