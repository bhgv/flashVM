# Native fd multiplexing and Tflush / Native fd-мультиплексирование и Tflush

Runtime0.146 adds `styxclientmountfd(fd, msize, user, tree, timeout_ms, limit)`.
The host transfers one exclusively owned connected POSIX stream socket on every
outcome. It selects the endpoint and trusted transport identity; this API does
not dial, authenticate uname or perform Tauth. The peer must accept NOFID attach
under the host's selected service/transport policy. There is no guest fd-table
lookup or new namespace authority. Rebuild consumers: the native `StyxTransfer`
layout adds `cancelled`; wire9P2000 and original Inferno codecs are unchanged.

The fd is made nonblocking and close-on-exec. Native notification pipes are also
close-on-exec; lingering protocol waits are disabled on final socket close. One
independent native OS thread owns all send/recv and poll work. It contains no
Prog/REG, VM heap, managed callbacks or guest execution. Native I/O callers wait
for their own completion; cancellation wakes the fd owner through a pipe. Guest
polling, guest suspension, executor migration and global exclusive/stop are not
used. The existing callback-transport constructor remains source-compatible and
single-flight because its shared blocking deadline contract is different.

## Bounded admission and replies / Ограниченный admission и ответы

`msize` is256..65536, `timeout_ms` is1..120000, and `limit` is1..128 outstanding
requests. Quota/OOM/token-binding rejection occurs before wire effects; it does
not break a healthy connection. There is no unbounded admission queue. Each
admitted transaction owns its encoded frame, reply buffer and native condition.
The table, per-request buffers and two engine frame buffers have bounded sizes.

The constructor's version transaction uses NOTAG. Ordinary requests lease unique
tags in1..limit, with distinct reserved flush tags in limit+1..2*limit. Flush needs
no additional admission slot, even when the ordinary quota is full. Fid numbers
remain independently allocated monotonically. The native engine serializes frame
bytes, not the whole RPC: different requests can await responses concurrently,
and responses route to their own native waiters in any order.

The caller's Client mutex protects allocation and reply copying. It is released
while a multiplexed RPC waits and reacquired before decoding/copying output.
Receive strings/data never escape into another request's mutable frame. One
fid still requires caller serialization and lifetime pinning; use separate fids
for simultaneous operations, including equal-Qid opens.

## Cancellation and deadlines / Отмена и deadlines

`styxcancelcreate` returns an owned sticky native token. `styxcancelrequest` is
thread-safe and does not wait for protocol completion. Each token is single-use and can bind to only one admitted operation. Reuse
refuses before wire effects, so a late control request cannot cancel a later
operation. Admission retains it independently until
collection. Keep the caller token alive through the operation unless the host
has independent proof that admission occurred; starting a thread alone is not
such proof. `styxcancelfree` consumes a caller reference and does not itself cancel.

`styxclientreadcancel` and `styxclientwritecancel` accept a token on an fd-owner
client. Their ordinary counterparts use the same multiplexer without explicit
cancellation. Mount-provider operations therefore gain concurrent RPCs and
request deadlines automatically when constructed from an fd-owner client. A
per-guest FD/broker cancellation binding is not added here.

```c
/* Native host transfers a connected fd and receives an owned client. */
StyxClient *client = styxclientmountfd(fd, 8192, user_hint, logical_tree, 5000, 32);
StyxCancel *cancel = styxcancelcreate();
/* Open/retain an independent fid using normal client walk/open. */

/* Native I/O worker; caller serializes and pins fid for the whole call. */
styxclientreadcancel(fid, offset, bytes, capacity, cancel, &result);

/* Concurrent native control owner, while it retains cancel: */
styxcancelrequest(cancel);

/* After worker retirement, consume the retained caller resources. */
styxcancelfree(cancel);
styxclientclunk(fid);
styxclientfree(client);
```

If cancellation/deadline wins before any original bytes were accepted, work is
retired locally: `cancelled=1`, confirmed count0, no uncertain effects, no Tflush.
After original bytes start, cancellation finishes that frame before sending its
Tflush. A partial original/flush frame never interleaves with another frame.
Timeout uses the same path; one extra nonrenewable timeout interval bounds
finishing the frame and receiving Rflush. Version negotiation failure is terminal
rather than trying to flush NOTAG.

| Observed order | Returned result and tag lifetime |
|---|---|
| Original reply before any flush bytes | Preserve original; no flush needed |
| Original reply after flush starts, before Rflush | Preserve original; hold both tags through Rflush |
| Rflush without original reply | Return cancelled; retire both leases |
| Original reply known, but Rflush is lost/invalid | Preserve known original result; fail connection |
| No original reply and flush grace expires | Return unacknowledged failure; fail all pending work |

The original acknowledgement always wins over cancellation intent. A confirmed
Rwrite count is not discarded. Without an original data acknowledgement, sent
operations conservatively retain uncertain stream/device effects; no effect is
replayed or automatically retried. `cancelled` distinguishes cancellation/timeout
without an original reply, and the VFS bridge maps it to CANCELLED. An attempted
uncertain data operation poisons that fid's later data calls. Other fids continue
after an acknowledged flush on a healthy connection.

Both tags stay leased until the required Rflush or terminal stream failure.
Standard9P requires the server not to send an old reply after Rflush; there is no
wire generation field to distinguish a violating server's fabricated old packet
after tag reuse. Unknown tags, duplicate replies, wrong kinds/counts, premature
replies to incomplete frames, and Rerror instead of Rflush break the connection.
The tests deliberately inject a late reply while its tag is inactive and require
terminal failure, preventing it from completing a future native waiter.

## Lifetime and shutdown / Lifetime и shutdown

`styxclientretain` takes another native owner reference. Ordinary provider/handle
pins continue to work. Final `styxclientfree` signals retirement and returns
without joining or waiting for a protocol acknowledgement. The detached native
owner closes its socket and releases its own engine reference; it retains no
Client/provider/guest pointer. Notification pipes stay alive until both native
owner and client references retire. This prevents a late cancel wake from using
an already freed engine or a recycled pipe descriptor.

For deterministic native teardown, retain a client capability and call
`styxclientshutdown` on an authorized native I/O worker. It rejects/wakes pending
work and can wait for the fd owner to retire; it does not consume the client.
Then drop the retained reference. This explicit waiting operation is separate
from nonwaiting provider destruction. It also works on legacy clients by
closing their transport once, after any serialized native exchange retires.

Engine failure fans out to every pending native caller. Known replies remain
known; unsent requests do not invent effects; sent unacknowledged operations
remain uncertain. No reconnection, request replay or remote-fid restoration is
implicit. Runtime0.147 wires idle terminal state automatically into mounted
provider journals; see [EVENTS.md](EVENTS.md). Heartbeat/change-source protocols
remain separate extensions. Explicit shutdown waits for fd retirement, not
completion of all native notification callbacks.

## Acceptance / Проверки

The opt-in native suite now has69 tests. Original C libstyx servers sit behind
independent deterministic response/flush gates. Coverage includes simultaneous
native callers, reversed responses, full quota plus cancellation, pre-admission
cancel, read/write cancel, original-response/Rflush races, failed flush after a
known acknowledgement, missing/invalid Rflush, failure fanout, token caller-ref
retirement, explicit shutdown, autonomous final-drop EOF before process exit,
and prepared per-connection publication. Existing provider scenarios also run
through the fd-owner client.

A deterministic unit includes the actual owner algorithm with two-byte sends
and would-block injection. It proves unsent cancellation, exact partial-frame
order, reserved flush priority, rejection of premature/duplicate responses and
preservation of an original acknowledgement through the flush fence. TCP send
boundaries alone are not used as proof of partial-send behavior. ASan/UBSan runs
use `detect_leaks=0` on this host; no LeakSanitizer/physical-device claim is made.

Native fd-owner обрабатывает несколько RPC, маршрутизирует ответы по tag и
отменяет через Tflush без участия гостя. Ответ, пришедший до Rflush, сохраняется.
При неизвестном исходе effects не повторяются. Последняя ссылка не ждёт сеть;
явный native shutdown может дождаться завершения owner. Авторизация транспорта,
heartbeat, guest-брокер отмен и восстановление remote fid остаются отдельными
расширениями; wire-протокол не менялся.


Runtime0.156 adds [cancellable walk/open/stat](CONTROL-CANCEL.md), including one
logical cancellation scope across fragmented Twalk and detached abandoned-fid
cleanup. It preserves the tag/Tflush rules above and the existing one-use public
token contract. An unacknowledged sent Topen cannot be replayed on the same fid.
