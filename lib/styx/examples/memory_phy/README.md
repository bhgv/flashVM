# 9P over a custom memory PHY

Copyright (c) 2026 bhgv

[English](README.md) · [Русский](README.ru.md) · [Examples](../README.md) · [Library](../../README.md)

## Purpose

A real native 9P client and the original libstyx server exchange wire messages through bounded memory buffers. The carrier creates no socket, pipe, eventfd or file descriptor. Console stdout is only a process PASS/diagnostic channel. No VM, GUI or Unibus is needed.

This demonstrates a custom PhyOps driver, not a new 9P codec. Operations remain ordinary Tversion, Tattach, Twalk, Topen, Tstat, Tread and Tclunk. Bytes flow both ways, but this is a conventional client/server session: no PHY_BIDIR_SESSION or hidden reverse server is enabled.

## Build and run

```sh
cmake -S . -B build -DSTYX_HOSTED=ON -DBUILD_TESTING=ON
cmake --build build -j
./build/examples/styx-memory-phy
ctest --test-dir build -R '^styx-memory' --output-on-failure
```

Run from the package root. There are no command-line arguments. Expect PASS real 9P over memory PHY; exit 0 verifies RPCs and final destruction of both endpoints. Exit 1/stderr identifies setup/RPC/lifetime failure. Native join failure terminates via _Exit rather than reclaiming a possibly live worker's memory.

The driver is Linux-only: pthread, GCC/Clang atomics and binary futex wait/wake. These are example dependencies, not PhyOps ABI or portable-codec requirements. STYX_HOSTED=OFF excludes the example and retains its minimal dependency profile.

## Files and settings

| File/parameter | Value and purpose |
| --- | --- |
| memory.c | Factory, adoption, shared lifetime |
| io.c | Byte prefixes, circular buffers, backpressure |
| wait.c | Descriptor-free native wait/wake; one monotonic deadline per wait |
| server.c / main.c | Original libstyx server and real client RPCs |
| MEMORY_CAPACITY | 16 incoming bytes per direction, not the 9P frame size |
| max_read / max_write | 3/2 bytes per callback; fragment boundaries are not message boundaries |
| cut_after | 0 normally; test value 5 models a confirmed prefix with ERROR/UNCERTAIN |
| MemoryStats | Zeroed caller-owned observations surviving both endpoint destroys |
| msize / timeout / limit | 256 / 3000 ms / 4 for the native client, not protocol-global settings |

## Ownership and flow

1. [memory_pair](memory.c#L60) creates two owned RemotePhy endpoints. [phycreate](../../docs/reference/phycreate.md) copies ops/info and adopts state only on success; the factory unwinds partial construction itself.
2. The server thread gives one endpoint to [styxphyinit](../../docs/reference/styxphyinit.md); main gives the other to [styxclientmountphy](../../docs/reference/styxclientmountphy.md). Both constructors consume their grants on every outcome.
3. [memory_server](server.c#L45) owns server/ops/file table through styxend. Main waits for actual startup through a native condition without touching the active server object.
4. [use_file](main.c#L20) observes denied write-open on readonly status, then uses a separate fid for read/stat, EOF at signed64 max offset and read at offset 7. Open Rerror does not destroy the healthy connection.
5. [styxclientshutdown](../../docs/reference/styxclientshutdown.md) and [styxclientfree](../../docs/reference/styxclientfree.md) retire the client owner. The peer observes disconnect and ends; main joins before reclaiming startup storage.
6. [memory_destroy](memory.c#L41) releases the shared allocation only after the second endpoint retires. Wake callers must finish before final free; buffered prefixes can outlive the opposite endpoint.

## Driver contract

Read/write use trylock and never wait for peer consumption. Empty/full/contention returns AGAIN with zero count, not EOF. A positive count is the accepted prefix. At cut_after, count and ERROR/UNCERTAIN coexist: retain the prefix and never replay the whole request. Peer disconnect first permits draining accepted bytes, then EOF.

Wake is sticky local work, not a wire message. Wait clears its binary latch before inspecting persistent predicates/wake; the kernel refuses to sleep if a notifier already changed the latch. The futex word is only 0/1, not a 32-bit queue ordinal or generation counter. Timeout 0 polls, -1 waits indefinitely; positive timeouts share one CLOCK_MONOTONIC deadline across EINTR/retries. This does not promise realtime scheduler latency.

PHY_STREAM/ORDERED/RELIABLE are declared because memory preserves byte order and reports failure. PHY_PACKET is not directly suitable for 9P. Real SPI/I2C/CAN/radio links may need reassembly, ordering, error/loss handling and half-duplex arbitration below this interface. Setting flags does not implement those properties. PhyInfo.connected is local lifecycle, not heartbeat, access permission or proof of peer state.

## Checks and limits

styx-memory-construction injects allocation/mutex and first/second adoption failure, checking final frees. styx-memory-contract checks bounded prefixes, full/AGAIN, drain/EOF, uncertainty, timeout and 500 concurrent wake rounds without sleeps. styx-memory-no-descriptor runs the same client/server with link-time refusal of project socket/connect/open/close/pipe/eventfd/read/poll paths. This guards linked code, not every libc-internal kernel call.

The example does not implement authentication, VFS unions, real DMA/IRQ, packet loss, board drivers or remote-module migration. It leaves a replaceable boundary: change PhyOps/context and endpoint construction while retaining client/server and wire protocol. An MCU needs its own native wait/wake adapter, not a copied Linux futex implementation.

[Driver interface](../../remote_phy/phy.h) · [Source factory](memory.c) · [SDK integration](../../docs/embedding.md) · [Main-project carrier guide](https://github.com/bhgv/flashVM/blob/master/remote_phy/README.md)
