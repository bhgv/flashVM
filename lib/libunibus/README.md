# Standalone libunibus

Copyright (c) 2026 bhgv. [License](LICENSE.bhgv).

The library is implemented in C and requires no C++ compiler/runtime. Examples, test clients and tools may use other languages; C++ consumer tests verify C API compatibility, not the implementation language.

[Русский](README.ru.md) · [API reference](docs/index.md) · [Examples](examples/README.md)

Unibus is a separate source library. It does not include or link Styx, DisVM,
FlashVM, a GUI, lib9 or another workspace directory. The optional 9P gateway
transfers opaque protocol bytes to an external server; it links only this library
and system facilities. Unibus can equally carry sensor, robot-control, database
or application-defined messages.

## What is included

| Layer | CMake target | Dependencies and responsibility |
| --- | --- | --- |
| Binary S-records and canonical integers | `Unibus::wire` | C89, caller-owned buffers, standard byte operations; no allocation/threads/OS |
| Fragmented input, directional type bindings, limit negotiation | `Unibus::wire` | Same allocation-free core; explicit state held by its owner |
| Fixed slots, media fragments, measurements and adaptation decisions | `Unibus::wire` | Protocol/state machines only; caller supplies buffers and policy |
| uint64 ordinal prefix, CRC5, named order-service messages | `Unibus::wire` | Existing exact wire implementation; no per-packet generation field |
| Per-tag queues, reordering, acknowledgement checkpoints | `Unibus::queues` | C99, allocator and pthread; independent consumers wait only for their own tag |

The host decides how to read/write a physical carrier and runs its service owner.
The wire core does not spawn an interpreter, acquire files, dial TCP, or impose
robot/terminal application policy. Hardware-specific SPI/I2C/USB/CAN/radio/serial
drivers are application adapters, not implicit dependencies on TCP or UDP.

Terminal-specific FlashVM grants, runtime sysevents publication, native launch
admission and automatic creation of concrete media transports are deliberately
outside this external library. Their wire records/state-machine helpers are
available here; embedding an OS or device requires choosing its own ownership,
worker creation, limits and access policy. The example gateway is not advertised
as the complete native FlashVM terminal broker or its generic RPC marshaller.

## Minimal build

```sh
cmake -S . -B build-small -DUNIBUS_QUEUES=OFF -DBUILD_TESTING=ON
cmake --build build-small
ctest --test-dir build-small --output-on-failure
```

Only the C89 wire library and allocation-free nested-record example are built.
No Python, pthread or socket is required. With `BUILD_TESTING=OFF`, only the archive
and headers are produced, suitable for incorporation into a device toolchain.

## Hosted queues and examples

```sh
cmake -S . -B build -DUNIBUS_QUEUES=ON -DBUILD_TESTING=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
cmake --install build --prefix "$PWD/sdk"
```

Native queues need pthread. The POSIX gateway and its independent acceptance test
add sockets and Python3 respectively; Python is not linked into the library.
For a separate installed consumer:

```cmake
find_package(Unibus CONFIG REQUIRED)
add_executable(sensor main.c)
target_link_libraries(sensor PRIVATE Unibus::wire)
```

Set `CMAKE_PREFIX_PATH` to the installed prefix, or use this copied project through
`add_subdirectory`. Choose `Unibus::queues` instead when native ordered queues are
needed. The retained `styxorder*` C names are historical source-compatible names
of this independent queue engine; they do not create a libstyx dependency.

## Records, services and client isolation

An ordinary S-record contains an encoded checksum, encoded length, encoded tag and
opaque payload. Integers use the existing canonical variable-width format, not a
native C structure. Fragment boundaries from the carrier are not record boundaries.
Input is not dispatched until framing, bounds and checksum validate.

Outer tag0 belongs to the service owner. It carries direction-local assignments,
negotiation and transport-management records. An outer nonzero client envelope
contains that client's own records. Clients can both use inner tag1; outer routing
keeps them independent. Applications must not receive or repurpose the carrier's
outer tag0. A client's own inner tag0 is a different namespace if its schema uses it.

The legacy version1 binding table is immutable after assignment; exact repeats
are idempotent and conflicting assignments are refused. Do not implement dynamic
rebinding by overwriting that table. Named order-service negotiation, renewal and
acknowledged per-tag counter boundaries are separate APIs. Data using a new
assignment/profile must wait for its matching agreement. No global drain of other
clients is required merely because one client's assignment changes.

## Order, partial I/O and shutdown

An ordered packet uses a prefix byte with a 3-bit unsigned-number width and CRC5,
followed by 1..8 ordinal bytes and the existing checked S-record. Numbers are
uint64, scoped to the selected tag/direction. There is no generation number added
to every packet. At exhaustion the selected counter requires explicit renewed
agreement; it never silently wraps and aliases an old packet.

An encoded packet is retained until accepted by its carrier: retry the same bytes,
not a second encoding with a new number. Partial writes must not replay effects.
Checksums detect corruption; they are not authentication or encryption. The S-record
profile retains its existing shorter checksum for short records; the extraction
does not silently replace that algorithm with a different wire format.

One feed/dispatch owner may enqueue independently of consumer execution. Consumers
block only on their selected queues. Queue allocation failure is an explicit error,
not permission to discard a message or sleep the entire channel. Physical output
backpressure can delay output; it must not be confused with a client waiting for its
own receive. Close wakes queue waiters and allows already-ready records to drain.
Free requires all feed/encode/consumer calls to have retired.

## Examples

- [Nested records](examples/nested.md): real tag0 bindings, one-byte input fragments,
  outer client7 and inner application tag1, without an allocator or OS.
- [Independent queues](examples/queues.md): a native consumer waits for client7
  while the dispatcher serves client8, then joins the consumer.
- [9P gateway](examples/bridge/README.md): one RX/service owner, two per-client FIFOs
  and workers, binary stdin/stdout, tag0 handled only by the broker. Both clients
  reuse inner tag1 and identical 9P fid numbers on separate backend connections.

The gateway is one explicit baseline compact-record application profile. It uses
predeclared client7/client8 assignments and one outstanding backend RPC per client.
It does not implement arbitrary service commands, new-client negotiation, automatic
lane scaling or concurrent Tflush for a blocked RPC on the same client. Those
are embedding/coordinator policies, not implied by this example. Its socket timeout
is per I/O operation, not a proof of a total end-to-end deadline.

The [file-server recipes](examples/bridge/README.md) cover flat, fixed multi-level,
live key/value, lazy HTTP hierarchy and changing window/control namespaces through
the same opaque gateway. Backend choice changes neither Unibus nor its core dependencies.

## Reference and compatibility

```sh
python3 view_docs.py --port 8911 --no-browser
```

The copied viewer uses only Python3's standard library and serves this directory
read-only on loopback. Both language guides, dictionary-style function pages,
examples and line-addressed source are available offline.

Existing wire functions and public names are retained from the main implementations.
`SOURCES.jsonl` records provenance and extraction transformations. Compatibility
tests include ordinal widths, CRC5, malformed/truncated packets, independent tag
progress, acknowledgement/rollover and allocation-failure boundaries. See
[protocol](docs/protocol.md), [porting](docs/porting.md) and [API](docs/index.md).

## Introduction and tutorial

[What and why](docs/introduction.md) · [Writing a service](docs/tutorial.md)

C/C++ integration, package versions and component selection: [SDK](docs/embedding.md).
