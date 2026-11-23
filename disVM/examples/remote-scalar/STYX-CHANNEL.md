# Pass a Limbo channel through Styx

[Русский](STYX-CHANNEL.ru.md) · [Mounted library guide](STYX.md)

An ordinary `load` can link a library export that takes a live `chan of T`.
The caller opens the mounted function as usual. DisVM publishes a virtual
`.prog/<pid>/.unibus` file in the caller's reverse Styx grant, then sends one
capability key for each distinct non-nil channel argument. Repeating an
argument preserves channel identity in the provider. The provider can execute
the original Dis function with its normal channel instructions. A thread
spawned by the provider can use those same proxies while the call is live.
If the provider returns an argument channel, the caller receives its original
channel object, with the same queue and buffering.

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
| [Interface](chain-channel.m) | Regular typed parameters and a channel result. |
| [Client](styx-channel-client.b) | Normal `load`, a large tuple, unchanged caller data after transfer, cancellation and reuse, and a returned channel with its original identity. |
| [Provider](styx-channel-owner.b) | Direct channel instructions in the original process and in `spawn`, a selection across two proxy channels, and a mixed selection with a fallback. |

## Channel operations

The VM waiting for the function reply services UniBus requests using the
original channel queues. A send completes when a receiver takes the value or
when the original buffer has room. A receive uses the original buffer or a
waiting sender. A provider `alt` with remote guards submits them together and
commits one guard. An `alt` with a fallback can also mix direct local and remote
guards. It tests each guard at most once in randomized order and stops at the
first match.

Each operation opens a fresh read/write fid on the existing reverse Styx
session. The operation and its reply share that fid. A complete record can span
several Styx messages. The operation becomes visible only after the full record
passes framing and type validation. On cancellation or return the caller
seals its UniBus endpoint and drops the grant. The provider revokes the route
in its spawned threads at the same boundary. If transport fails after a match
commits, the provider receives an error and the VM does not repeat the operation.

Channel elements use the managed value codec. It covers numeric types,
strings, arrays, tuples and supported `ref` values. References in one
message preserve aliases within that message. The wire bound is 64 KiB per
operation, including its headers and any nested guards. Channel handles can
appear directly or in tuples. Each call and each provider selection supports
up to eight distinct channels or direct guards, respectively. Blocking
selections mixing local and remote guards, proxy channel arrays in `alt`,
channels inside message values, and export of a newly created provider
channel as a result still need runtime support.

## Wire records

| Tag | Record |
| --- | --- |
| 24 | Argument or result capability with a positive 32-bit key in network byte order. Tag 9 represents `nil`. |
| 25 | Send with a four-byte key followed by a typed value frame. An `int` channel retains the original scalar record. |
| 26 | Receive with a four-byte key. |
| 27 | Int-only reply using the [UniBus scalar record](../../native/terminal/scalar_record.h). An `alt` reply packs the one-based guard ordinal in the high 32 bits and the value bits in the low 32 bits. Zero denotes the fallback. |
| 28, 29 | Blocking or fallback selection, respectively. The payload is an ordered sequence of send and receive records. |
| 30 | Typed reply. The first byte is the one-based guard ordinal, zero for fallback or 255 for failure. A selected receive includes one managed value frame. |

[Native transport](../../runtime/vfs/channel_bus.c) ·
[Dis instructions and channel matching](../../runtime/vfs/channel_proxy.c)
