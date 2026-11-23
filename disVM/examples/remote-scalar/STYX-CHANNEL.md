# Pass a Limbo channel through Styx

[Русский](STYX-CHANNEL.ru.md) · [Mounted library guide](STYX.md)

An ordinary `load` can link a library export that takes a live `chan of int`.
The caller opens the mounted function as usual. DisVM publishes a virtual
`.prog/<pid>/.unibus` file in the caller's reverse Styx grant, then sends one
capability key for each distinct non-nil channel argument. Repeating an
argument preserves channel identity in the provider. The provider can execute
the original Dis function with its normal channel instructions.

## Run the example

Build the SDK and SDK-enabled runtime as described in the
[mounted library guide](STYX.md#prepare-and-exercise-the-publication).
Run from the repository root with the resulting DisVM and Limbo paths.

```sh
python3 disVM/examples/remote-scalar/run_channel.py \
    --runtime build/scalar-runtime/disvm \
    --limbo build/scalar-runtime/bootstrap/limbo-bootstrap
```

Add `--jit` to run both guests in JIT mode. The runner compiles the client and
provider with `-D RG`, starts two DisVM processes in separate temporary
directories and connects them over TCP loopback. It checks for
`PASS automatic Limbo channels over UniBus` and removes the images on exit.

| Source | What it demonstrates |
| --- | --- |
| [Interface](chain-channel.m) | A typed export using regular Limbo parameters. |
| [Client](styx-channel-client.b) | Normal `load`, an unbuffered rendezvous with a local `alt`, a buffered send and cancellation followed by a successful call. |
| [Provider](styx-channel-owner.b) | Direct `send`, `recv` and `alt`, including a selection across two proxy channels and a fallback arm. |

## Channel operations

The VM waiting for the function reply services UniBus requests using the
original channel queues. A send completes when a receiver takes the value or
when the original buffer has room. A receive uses the original buffer or a
waiting sender. A provider `alt` submits its guards together and commits one
guard. An `alt` with a fallback performs one readiness check on the source VM.

Each operation opens a fresh read/write fid on the existing reverse Styx
session. The operation and its reply share that fid. On cancellation or return
the caller seals its UniBus endpoint and drops the grant. If transport fails
after a match commits, the provider receives an error and the VM does not
repeat the operation.

This implementation supports direct `chan of int` arguments, including tuple
positions, and keeps their capabilities live for the duration of the call.
Provider `alt` takes up to eight direct proxy guards in the same call.
Other element types, channel results, proxy use by a separately spawned
provider thread, and selections mixing local and remote guards or using channel
arrays still need runtime support.

## Wire records

| Tag | Record |
| --- | --- |
| 24 | Argument capability with a positive 32-bit key in network byte order. Tag 9 represents `nil`. |
| 25 | Send with a four-byte key followed by one typed int S-record. |
| 26 | Receive with a four-byte key. |
| 27 | Reply using the [UniBus scalar record](../../native/terminal/scalar_record.h). An `alt` reply packs the one-based guard ordinal in the high 32 bits and the value bits in the low 32 bits. Zero denotes the fallback. |
| 28, 29 | Blocking or fallback selection, respectively. The payload is an ordered sequence of send and receive records. |

[Native transport](../../runtime/vfs/channel_bus.c) ·
[Dis instructions and channel matching](../../runtime/vfs/channel_proxy.c)
