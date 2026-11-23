# Two channels on one C module

[Русский](module_channels.ru.md) · [libstyxrpc](../README.md)

## Prerequisites

Use Linux, a C compiler, CMake, Python 3, an installed Styx/UniBus prefix,
built DisVM and `limbo-bootstrap` binaries, and the DisVM Limbo module
directory `disVM/upstream/module`. Build from the `lib/styxrpc` directory
with `STYXRPC_EXAMPLES=ON`. This option adds the hosted libraries and
example programs. The example uses Linux `getrandom` to issue tokens.

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/prefix -DSTYXRPC_EXAMPLES=ON
cmake --build build
python3 examples/disvm/run.py --channels --runtime /path/to/disvm \
    --limbo /path/to/limbo-bootstrap --module /path/to/disVM/upstream/module \
    --device build/styxrpc-channel-device
```

`--channels` compiles the Limbo channel client, connects both processes
over one socket and mounts the module under `/device`. Repeat the command
with `--jit` to run the same scenario with the JIT. The initial command
uses the interpreter. Both modes print `PASS Limbo/C live channels over Styx`.

## Follow the example

1. In `channel_device.c`, the `leases` array reserves eight simultaneous
   grants. `fill_entropy` fills the entire requested buffer and retries
   interrupted reads. `main` initializes the registry before serving.
   Opens of `.unibus` use the registry's resource descriptor.
2. For each `port()` call, `publish_port` verifies the request and creates
   a separate `DevicePort`. In `owned_port.c` the scheduler gets a
   one-element string queue. Its token binds that queue to the call result.
   If result encoding fails after publication, the code revokes the token
   and releases the port. The call write fails on the client.
3. `channel_client.b` calls `port()` twice and keeps both results alive.
   The channels share a key number in their respective leases. The client
   sends different strings, checks a ready `alt` receive on the first
   port and reads the second port. Each result belongs to its function fid.
4. On the first operation write to `.unibus`, the library authenticates
   the token and opens the corresponding queue. The library subtracts
   the 16-byte token extent from request write offsets. The client reads
   a receive result or selected `alt` from offset zero. A successful
   receive frees the queue slot for another string.
5. Clunk of a function fid revokes its token, cancels pending operations
   and releases the queue. The other token keeps serving. The launcher
   closes the socket and reaps both processes at exit.

To check wrong-token rejection, capacity limits and reuse of a released slot,
run the focused C test:

```sh
ctest --test-dir build -R '^styxrpc-module-channels$' --output-on-failure
```

[C provider](disvm/channel_device.c) · [Queue](disvm/owned_port.c) · [Limbo client](disvm/channel_client.b) · [Publication](../reference/module_channels_publish.md) · [Revocation](../reference/module_channels_revoke.md)
