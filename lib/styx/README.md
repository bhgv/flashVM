# Standalone Styx / 9P2000

Copyright (c) 2026 bhgv. Original Inferno notices remain in [vendor/inferno/NOTICE](vendor/inferno/NOTICE)
and the individual component notices.

[Русский](README.ru.md) · [API reference](docs/index.md) · [Examples](examples/README.md)

This project is independent of DisVM, FlashVM and Unibus. It contains original
Inferno codecs and the existing native client/server adapters as source, including
their legal notices. There is no graphics, font, guest-heap or interpreter dependency.

## Choose the required layer

| CMake target | Purpose | Requirements |
| --- | --- | --- |
| `Styx::codec` | Original `Fcall`/`Dir` 9P2000 encode/decode | C99, `stdint.h`, `memmove`, `memset`, `strlen`; no OS, allocation or threads |
| `Styx::wire` | Optional S-record framing used by the explicit traversal extension | C89, caller-provided buffers; ordinary 9P does not use this wrapper |
| `Styx::nine` | Complete selected original POSIX lib9 support | Linux64 libc/libm/pthread |
| `Styx::server` | Original C libstyx with its POSIX TCP backend | `Styx::nine` |
| `Styx::client` | Native multiplexed 9P client, retained fids, cancellation/heartbeat | `Styx::nine`, carrier, traversal, pthread |
| `Styx::phy_server` | Original server dispatcher with the replaceable PHY backend | Same non-graphical native support; alternative to the POSIX backend |
| `Styx::carrier` | Native read/write/wait/poll/connect-disconnect abstraction | Core callbacks plus the supplied POSIX descriptor driver |
| `Styx::walk` | Optional request-local traversal identity/history | Native allocation/pthread; explicit extension, not a standard 9P requirement |

`Styx::codec` and `Styx::nine` are alternative providers of the original codec
symbols. Do not link both into one program or mix the codec-only scalar facade
with a second conflicting set of Inferno native type declarations. Native client
and server consumers use their supplied native headers through their targets.

## Build a small codec-only project

```sh
cmake -S . -B build-small -DSTYX_HOSTED=OFF -DBUILD_TESTING=ON
cmake --build build-small
ctest --test-dir build-small --output-on-failure
```

No Python, lib9, socket, thread or VM build is required by that configuration.
The original codecs operate on caller-owned byte buffers. `convM2S` may modify
the message buffer to terminate counted strings; returned strings/data borrow
that buffer. Keep it live and exclusive until the decoded message is consumed.
Do not treat a native `Fcall` or `Dir` structure as wire bytes.

An MCU application supplies framing, byte transport, fid/session bookkeeping,
backend operations and scheduling around this codec. The hosted original server
is not claimed to be an already tested freestanding MCU server or a universal
32-bit lib9 port. See [porting](docs/porting.md).

## Build hosted client/server and examples

```sh
cmake -S . -B build -DSTYX_HOSTED=ON -DBUILD_TESTING=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
cmake --install build --prefix "$PWD/sdk"
```

Hosted selection currently covers Linux amd64, arm64 and riscv64 source layouts;
native execution acceptance is performed on the available Linux amd64 host.
Python3 selects retained source files and runs examples/tests; formatter/decimal
host adaptations are already applied to this distribution at export time; it is not linked into the client/server. The HTTP example
uses a Python standard-library helper; other servers need no HTTP package or database engine.

For a separate CMake consumer:

```cmake
find_package(Styx CONFIG REQUIRED)
add_executable(device main.c)
target_link_libraries(device PRIVATE Styx::codec)
```

Set `CMAKE_PREFIX_PATH` to the installed `sdk` directory. Alternatively use
`add_subdirectory(path-to-this-copied-project)` and the same target. An installed
consumer never needs the original repository or its bootstrap tools.

## File-server examples

| Example | What it demonstrates |
| --- | --- |
| [flat](examples/flat/README.md) | Fixed `status` and `temperature` files directly under root |
| [tree](examples/tree/README.md) | Fixed directories, including `devices/sensor/temperature` |
| [database](examples/database/README.md) | Fresh root keys and values from an explicitly supplied TSV key/value source |
| [http](examples/http/README.md) | Lazy immediate-child HTTP queries through country/city/district/street/house/room levels |
| [live](examples/live/README.md) | Mutable window/control registry; adding/removing entries changes directory results |

The shared [catalog contract](examples/catalog.h) passes stable object IDs, not a
serialized tree. `walk` asks for a component, directory `read` returns whole encoded
`Dir` entries for that directory, and file `read` uses the already opened fid.
It does not perform `Twalk` again before every read/write. Backend identity and
permissions remain authoritative; a removed object must not redirect an old fid
to another object later created at the same name.

## Inferno and Unix access

For example, start `./build/examples/styx-tree 5640`. Its first stdout line is the
actual port, useful when port0 selects an ephemeral listener. A native client reads:

```sh
./build/examples/styx-cat 5640 devices/sensor/temperature
```

An Inferno environment can dial the TCP listener and mount the resulting ordinary
Styx connection using its existing `mount` tools. A Unix 9P client must select
**9P2000**, not assume 9P2000.L or 9P2000.u extensions. Kernel mount commands and
available transports vary by OS; a working native/oracle client test is not a claim
that every kernel dialect or authentication setup has been exercised.

Examples expose synthetic state and implement no authentication or TLS. The original
POSIX listener binds the configured TCP port; select a deployment interface/policy
through the PHY backend for a real service. Passing a `uname` is not authentication.
The demo mode checks do not replace production user/group/credential policy.

## Backend and concurrency contracts

The example catalog fetches only immediate-child pages. The HTTP fixture computes
billions of synthetic nodes without storing them, but supplies no real-world geography
dataset. Replace its JSON service/adapter with your actual Internet or database source.
Directory offsets are encoded-byte positions; changing a listing can require rewind.
This example rescans earlier pages to resolve offsets, trading simplicity for work.

The example server calls synchronous backend callbacks from its original dispatcher.
A slow HTTP query delays that example dispatcher; production asynchronous providers
should use the supplied deferred-read/server ownership APIs. The HTTP helper has
a 3-second socket timeout and a separate 5-second process alarm. Responses, fields
and immediate-child pages are bounded; no whole-tree download occurs.

Native clients retain each opened fid and its connection. Clunk/free consume their
respective owners. An I/O failure/uncertain write is not permission to replay an
effect automatically. Basic raw 9P remains distinct from the opt-in S-record traversal
profile, which must be enabled at both endpoints.

## Browse and source compatibility

```sh
python3 view_docs.py --port 8910 --no-browser
```

The viewer uses only Python3's standard library, serves loopback read-only, and
contains both language guides, function pages, examples and line-addressable source.
Links to the main project's help are optional external links, never build dependencies.
See [API index](docs/index.md), [protocol](docs/protocol.md) and [porting](docs/porting.md).

The extraction's existing wire/API names are intentionally retained. This directory
is not a promise of every Inferno facility or an imported operating system. Original
license terms apply to their components; [LICENSE.bhgv](LICENSE.bhgv) covers first-party additions.

## Introduction and tutorial

[What and why](docs/introduction.md) · [Writing a service](docs/tutorial.md)

C/C++ integration, package versions and component selection: [SDK](docs/embedding.md).

Real 9P without TCP or carrier descriptors: [Memory PHY](examples/memory_phy/README.md).
