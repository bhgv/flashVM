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

## Named launches and reverse context

```sh
python3 examples/disvm/named.py --runtime /path/to/disvm \
    --limbo /path/to/limbo-bootstrap --module /path/to/upstream/module \
    --native-module /path/to/disVM/module --driver build/styxrpc-named
```

The script connects a persistent DisVM provider and a C client with one socket.
The client publishes `.prog/42` on the reverse stream, selects two public
program names, closes the fid of a running loop, and uses the same connection
for the following request. The first image reads `.pid` and a `.vfs` file,
updates `.var/count`, reads FD 0 and writes to FD 1 and 7.
Expect `PASS one socket, two named Dis modules, reverse .var/.fd/.vfs` in both VM modes.

## Generated interface and nested ref

The `styxrpc-interface` executable prints `device.m` from the device's
registered shape. The script `examples/disvm/run.py` locates the generator
next to `--device`, or at `--interface` if provided, and compiles the Limbo
client with `-D RG`. After `bump`, it checks the changed field and the
nested object's original identity. Expect
`PASS Dis load of standalone C device` in both modes.

## UniBus guarded channel operations

```sh
ctest --test-dir build -R styxrpc-unibus-socket --output-on-failure
```

Two C endpoints use `.prog/42/.unibus` over Styx. The fixture scheduler
chooses the ready string guard while another fid is waiting to receive.
Clunk triggers exactly one application cancellation. Cross-runtime
`chan of T` still requires grant and Limbo scheduler integration.

## Live channels across Dis and C

```sh
python3 examples/disvm/run.py --channels --runtime /path/to/disvm \
    --limbo /path/to/limbo-bootstrap --module /path/to/disVM/upstream/module \
    --device build/styxrpc-channel-device
```

Expect `PASS Limbo/C live channels over Styx`. Add `--jit` for the JIT.
Limbo passes three channels to the C export. C tests an idle guard, receives
a string through reverse UniBus, then sends a reply. In a separate call C
returns a buffered channel. Limbo sends a value and receives it with
`alt`. The module's `.unibus` verifies its lease token. Clunk of the
function fid revokes the lease.

The named [example](disvm/named.py) also accesses live Int32 collections
held by C. It inserts, appends, pops and updates the vector, then adds
and selects a numeric key. Closing the selector revokes the old value fid.
