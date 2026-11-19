# unibus-9p-bridge

Copyright (c) 2026 bhgv

[English](README.md) · [Русский](README.ru.md) · [Examples](../README.md) · [Library](../../README.md)

## Purpose

Two clients share S-record stdin/stdout but own different backend connections and queues. They may reuse inner IDs and 9P tags/fids. This example carries opaque 9P inside nested records without linking libstyx, DisVM or FlashVM. TCP is this POSIX demonstration backend, not a library carrier restriction.

## Syntax and parameters

```text
unibus-9p-bridge PORT7 [PORT8]
```

| Parameter | Values | Meaning |
| --- | --- | --- |
| PORT7 | Digits only, 1..65535 | Client 7 loopback backend |
| PORT8 | Optional, same range | Client 8 backend; defaults to PORT7 |
| stdin | Binary S-record stream | Sole transport RX owner |
| stdout | Binary S-record stream | Complete checked service/client records only |
| stderr | Text | Usage and setup/admission/cleanup diagnostics |

Whitespace, signs, extra arguments, zero and overflow are rejected. Equal ports still create two different connections. This executable uses pthread, poll and sockets; other environments use the library/codec integration paths.

## Exit status

0 means complete carrier EOF and successful cleanup, not success of every client RPC: inner 2 and ordinary Rerror carry their own outcomes. 1 means invalid CLI or SIGPIPE setup failure. 2 means startup, carrier/protocol or cleanup failure; it does not prove that no backend effect occurred.

## Build and reproducible run

```sh
cmake -S . -B build -DUNIBUS_QUEUES=ON -DBUILD_TESTING=ON
cmake --build build -j
python3 examples/bridge/check.py build/examples/unibus-9p-bridge
python3 tests/gateway_edges.py build/examples/unibus-9p-bridge
ctest --test-dir build -R 'unibus-(gateway|bridge-)' --output-on-failure
```

Run from the package root. The Python checks create real loopback listeners and binary pipes. The first checks the compatible one-port form; the second covers distinct ports, pending RPCs, errors and shutdown. Integrate through binary pipes; do not type text on stdin.

## Execution flow

1. [bridge_run](lifecycle.c#L136) creates a private wake pipe and two connections. [bridge_worker_start](lifecycle.c#L20) consumes each fd and unwinds partial initialization failures.
2. The service declares outer tags 7/8 under tag 0, schema example.9p.v1 version 1. The peer sends its assignments before client traffic; identical repeats are allowed, conflicts rejected by immutable bindings.
3. [bridge_receive](receive.c#L61) waits for stdin or an output-loss wake. [dispatch](receive.c#L25) handles service tag 0 inside RX; clients never receive it.
4. [bridge_enqueue](worker.c#L18) copies the 9P frame or an error marker into that client's FIFO. RX never calls [bridge_reply](io.c#L94) or waits for backend execution.
5. [bridge_worker](worker.c#L89) executes one RPC at a time for its own client and emits inner tag 1 (reply) or 2 (error) inside that outer client tag. Errors preserve FIFO order too.
6. [bridge_worker_stop](lifecycle.c#L57) ends admission; [bridge_worker_finish](lifecycle.c#L78) joins before releasing socket, remaining FIFO and synchronization. The fd number cannot be closed/reused before join.

## Failure scope and effects

| Event | Response | Operation effects |
| --- | --- | --- |
| Invalid inner frame on a known client | Inner 2 through only that client's FIFO | Never forwarded; later valid requests remain usable |
| Backend consumed request then disconnected/truncated reply/reported an invalid length | Sticky failure on that client; inner 2 for later requests | First effect is uncertain; no replay/reconnect. Other client progresses |
| Invalid outer frame or unknown transport tag/schema | Exit 2 | Previously admitted requests drain; disconnect does not prove absent effects |
| Queue-copy allocation failure | Admission diagnostic and exit 2 | No silent drop/arbitrary quota; earlier admitted requests remain admitted |
| EOF between records | Drain accepted FIFOs, join, exit 0 | Replies may arrive after stdin half-close |
| EOF inside a record | Exit 2 | Never publish a partial record or guess resynchronization |
| Lost stdout | Sticky terminal output; wake RX; stop further queued work, join, exit 2 | Partial/completed effects are not undone; queued work may remain unexecuted |
| Native join failure | Fatal _Exit(2) | Never reclaim memory an unretired worker might still use |

## Partial I/O and profile boundaries

[bridge_exact](io.c#L63) advances only the remaining span after a short operation/EINTR. Zero write means no progress. After a partial error, [bridge_record](io.c#L72) forbids every further write to that physical direction until process exit: another frame cannot restore its lost boundary.

Socket timeout is 3 seconds per syscall, not an absolute RPC deadline. A slow backend can extend an RPC through partial progress; the other worker remains independent. Physical stdout backpressure can block output workers; this is distinct from waiting on a client's queue and does not permit losing records. The private wake pipe changes only its own descriptor flags, never inherited stdin/stdout flags; already-nonblocking stdout waits through poll on EAGAIN, while backend EAGAIN remains a socket timeout, and adds no Unibus tag.

Maximum 9P frame is 8192 bytes. The gateway validates length, not 9P type/tag or application fields; the 9P client validates those. An ordinary Rerror is forwarded unchanged under inner 1, does not break the connection and does not prevent a later request. This example does not implement same-client concurrent Tflush, spontaneous backend notifications, automatic physical-lane scaling, authorization, TLS or the ordered-prefix profile. These need the corresponding project service/PHY mechanisms; a codec does not automatically enable them.

[Private contracts](bridge.h) · [Wire API](../../docs/index.md) · [Protocol](../../docs/protocol.md) · [SDK](../../docs/embedding.md) · [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
