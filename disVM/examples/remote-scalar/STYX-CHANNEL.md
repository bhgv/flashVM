# Pass a Limbo channel through Styx

[Русский](STYX-CHANNEL.ru.md) · [Mounted library guide](STYX.md)

An ordinary `load` can link a library export that takes a live `chan of T`.
The caller opens the mounted function as usual. DisVM publishes a virtual
`.prog/<pid>/.unibus` file in the caller's reverse Styx grant, then sends one
capability key for each distinct non-nil channel argument. Repeating an
argument preserves channel identity in the provider. The provider can execute
the original Dis function with its normal channel instructions. A thread
spawned by the provider can use those same proxies while the call is live.
If the provider returns a channel granted by the caller, the caller receives
its original channel object, with the same queue and buffering. The caller can
grant a channel in the initial arguments or inside a later channel message.

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
| [Client](styx-channel-client.b) | Normal `load`, a large tuple, channels inside messages, cancellation and reuse, and returned channels with their original identities. |
| [Provider](styx-channel-owner.b) | Direct channel instructions in the original process and in `spawn`, selection across sparse arrays of proxy channels, and local and remote guards with a fallback. |

Run [run_channel_pair.py](run_channel_pair.py) with the same options to
exercise two provider threads rendezvousing on one unbuffered proxy while
the caller waits for the function reply.

## Channel operations

The VM waiting for the function reply services UniBus requests using the
original channel queues. A send completes when a receiver takes the value or
when the original buffer has room. A receive uses the original buffer or a
waiting sender. When two operations on the same UniBus are waiting on
opposite ends of one unbuffered channel, the bus pairs their fids after checking
the original channel for local waiters. It publishes both replies under the
same lock and then returns the fallback for any unmatched immediate selection.

A provider `alt` with remote guards submits them together and commits one
guard. A receive array contributes one guard for each non-nil channel.
The result preserves the original branch and array index. An `alt` with a
fallback can also mix local and remote guards. It tests each eligible option at
most once in randomized order and stops at the first match.

Each operation opens a fresh read/write fid on the existing reverse Styx
session. The operation and its reply share that fid. A complete record can span
several Styx messages. The operation becomes visible only after the full record
passes framing and type validation. On cancellation or return the caller
seals its UniBus endpoint and drops the grant. The provider revokes the route
in its spawned threads at the same boundary. If transport fails after a match
commits, the provider receives an error and the VM does not repeat the operation.

Channel elements use the managed value codec for numbers, strings, arrays,
tuples, supported `ref` values and channel handles. Ordinary `ref` aliases
are local to one message. Channel identity is preserved across messages and
provider threads in the same call. The provider can send back handles it
received on this route. The bus checks every such key against the caller's
published types before admitting a send.

The wire bound is 64 KiB per operation, including its headers and any nested
guards. Channel handles can appear directly or in arrays and tuples of
arguments and results. The scope supports up to eight distinct channels,
including grants made in messages. A provider `alt` can include up to eight
eligible options after expanding receive arrays. Blocking mixed selections
and export of a newly created provider channel still need runtime support.

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
