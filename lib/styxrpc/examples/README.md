# Calling an independent C device

[Русский](README.ru.md) · [libstyxrpc](../README.md)

Build with `-DSTYXRPC_EXAMPLES=ON` and an installed hosted Styx library.
From the `styxrpc` source directory, assuming the build directory is `build`:

```sh
python3 examples/socketpair.py build/styxrpc-device build/styxrpc-client
```

This starts two C processes on opposite ends of a socket. The device publishes
`sample` under `/mounted/device.dis`, opens one call state per fid and runs
the increment callback once per complete request. The caller uses fresh fids
for the scalar calls, an ordinary ref update and a nested graph update through
[`styxrpc_graph_invoke`](../reference/graph_invoke.md). It also reads and writes
the device register through the positional VFS API. Success prints
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
ctest --test-dir build -R 'styxrpc-(unibus-socket|channel-scheduler)' --output-on-failure
```

Two C endpoints use `.prog/42/.unibus` over Styx. The fixture scheduler
chooses the ready string guard while another fid is waiting to receive.
Clunk triggers one application cancellation. The standalone C scheduler
test covers FIFO, mixed `alt`, WAIT expiry and removal of a pending fid.

## Live channels across Dis and C

```sh
python3 examples/disvm/run.py --channels --runtime /path/to/disvm \
    --limbo /path/to/limbo-bootstrap --module /path/to/disVM/upstream/module \
    --device build/styxrpc-channel-device
```

Expect `PASS Limbo/C live channels over Styx`. Add `--jit` for the JIT.
Limbo passes three channels to the C export. C tests an idle guard, receives
a string through reverse UniBus and sends a reply. Limbo then passes
an inner channel through `chan of chan of string`, checks its identity on
return and exchanges strings on it. Two `port()` calls return channels with separate C-owned
buffers. Limbo checks receive and `alt`. The module's
`.unibus` verifies its token, which clunk of the function fid revokes. The
device reports errors on the open fid. The launcher closes the socket and
reaps both processes.

The named [example](disvm/named.py) also accesses live Int32 collections
held by C. It inserts, appends, pops and updates the vector, then adds
and selects a numeric key. Closing the selector revokes the old value fid.

## Reverse hybrid table

Run the [named launch example](disvm/named.py) using the command in the named
launch section. `device-program.b` opens `.prog/.self/.var/hybrid` and compares
position `1`, string `"1"` and sparse numeric key `5`. Key `-1` selects the
last dense position. The program inserts and appends values, pops the last
value and checks `.len` and `.count`. Re-reading an opened `.pop` returns the
same snapshot. A duplicate add and a value read after the selector is clunked
are expected to fail. The C process owns the table state.

[Registration](../reference/program_bigtable.md) · [Limbo operations](disvm/device-program.b) · [C callback](disvm/hybrid.c)

## Recursive grants between C and DisVM

Build with `STYXRPC_EXAMPLES=ON` and run from the libstyxrpc source directory.
Point `--disvm-source` at a DisVM checkout containing the compiler interfaces
and recursive Limbo channel fixture.

```sh
python3 examples/disvm/routed.py --runtime /path/to/disvm \
    --limbo /path/to/limbo-bootstrap --disvm-source /path/to/disVM \
    --driver build/styxrpc-routed-client
```

The C client publishes two reverse `.prog/<pid>/.unibus` endpoints and opens
the DisVM function on the same connection. The function returns two module
channels. C sends a grant for its reverse bus on the first channel. DisVM
sends the same module token twice on that bus. The client compares the
addresses and echoes the original frame. C then opens the delegated channel by
token and sends its second reverse PID. DisVM sends 43 to that endpoint,
receives 81 in return and passes 81 back on the second module channel.

The client checks refusal of an incorrect token, plus revocation of both issued
tokens when the function fid is clunked. Failure reports the stage number.
Success prints `PASS delegated PID and module channels between C and DisVM`
for both VM modes. The launcher closes the socket and waits for the provider.

[C client](disvm/routed_client.c) · [Address decoding](../reference/channel_element_routes.md) · [Routed operations](../reference/channel_route_exchange.md)

[Two channels over one socket](module_channels.md)

## DisVM integration tests

Build [native DisVM with the terminal launch SDK](https://github.com/bhgv/flashVM/blob/master/disVM/runtime/TERMINAL-LAUNCH-CLI.md).
Supply the DisVM source directory and that build directory, which contains
`disvm` and `bootstrap/limbo-bootstrap`. From `lib/styxrpc`, run:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/prefix \
    -DSTYXRPC_EXAMPLES=ON -DSTYXRPC_DISVM_SOURCE=/path/to/disVM \
    -DSTYXRPC_DISVM_BUILD=/path/to/disvm-terminal
cmake --build build
ctest --test-dir build -R '^styxrpc-disvm-' --output-on-failure
```

The tests exercise the interpreter and JIT. They cover loading C modules,
typed calls and nested refs, named and one-shot launches, reverse resources,
live channels and revocation of delegated routes.

For FlashVM, build [the adapter test host](https://github.com/bhgv/flashVM/blob/master/flashVM/doc/STYX_RPC.md#build) and also pass
`-DSTYXRPC_FLASHVM_TEST_HOST=/path/to/flashvm_tests_styxrpc_host`. Then run
`ctest --test-dir build -R 'styxrpc-((managed-|ref-|graph-|hybrid-|channel-|channel-args-|channel-incoming-|channel-delegated-)?(flash-to-dis|dis-to-flash)|flashvm-)' --output-on-failure`.
Twenty-four cases cover scalar, managed, ordinary ref, positional graphs, hybrid tables and live result channels in both directions and both DisVM execution modes. Graph calls exercise nested and returned refs, arrays of refs, tuple edits and rollback. [Hybrid tables](../reference/hybrid_tables.md) add ordered key patches, mixed ADT/table updates, signed 64-bit values and the result-only table profile. [Live channels](../reference/channel_leases.md) cover scoped returns, exact aliases, idle selection, seeded FIFO, rendezvous and clunk retirement. Two additional cases lend FlashVM argument channels to DisVM in interpreter and JIT mode and check alt, queue effects and scope retirement. Two cases lend DisVM channels to [FlashVM handlers](../reference/channel_handler_modules.md) and check reverse exchange, rendezvous, exceptions, timeouts, retirement of retained objects, independent result channels, and repeated delivery of nested channels. Both provider directions exercise nested result channels after closing the containing channel. The FlashVM provider additionally preserves the rendezvous marker when Limbo sends a nested channel back. Three more tests cover named launch from ActionScript, reverse resources, cancellation and positional I/O against the independent C device.

The recursive FlashVM integration sends two reverse PIDs and admits a separate module token on the issuing lease. Run both DisVM modes with `ctest --test-dir build -R channel-delegated`.

## Writable 9P tree

[The C and Limbo example](disvm/MUTATIONS.md) shows creation, removal and retained fids.

## Deferred file transfers

[The C and Limbo walkthrough](disvm/DEFERRED_IO.md) tests cancellable file callbacks on a shared connection.
