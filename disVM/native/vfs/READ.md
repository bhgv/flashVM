# Deferred byte reads / Отложенные byte reads

Runtime0.149 imports original C libstyx support for a pending ordinary Tread.
This is the transport prerequisite for a provider event resource: native producers
can notify the socket owner, which sends one Rread when data is ready. Other
messages and other clients continue normally. No guest cooperation, scheduler
poll, guest suspension, new9P message or private retry/error protocol is required.

The implementation is maintained in Inferno experiment and imported byte-for-byte.
[Complete native API and lifecycle](../src/tools/libstyx/tests/README.deferred-read.md)
are documented with the owning source. Rebuild library and consumers: Client
layout adds pending/staged reads, while existing Styxops callbacks stay unchanged.

## Embedding / Встраивание

A native `Styxops.read` callback can call
`styxdeferread(server, context, release)`. Success adopts context and returns one
caller-owned StyxRead reference; failure adopts nothing. On success set `*n=0`
and return nil. A synchronous read keeps its original behavior and allocates no
deferred request. One pending read per fid and128 per connection bound storage.
Directory, zero-capacity, NOTAG and duplicate defer requests cannot defer.

The socket owner later queries `styxreadclient`, `styxreadaux`, `styxreadlimit`
and `styxreadoffset` for the captured target, held fid context, negotiated capacity
and complete64-bit offset. Completion uses the captured client/fid, even if other
clients or requests have run since admission. Workers never receive these
pointers; they send copied native messages/readiness instead.

`styxreadreply(request, bytes, count, error)` sends the original-tag Rread or
Rerror. Invalid count/missing nonempty buffer returns0 and leaves the request
pending. Valid completion returns1 for retirement/response attempt, not peer
acknowledgement. Borrowed bytes need live only through this call. Accessors and
completion refuse staged/inactive requests. `styxreadfree` consumes the caller
ref; freeing it alone does not cancel live dispatcher work.

The release hook runs once after detachment and before fid/client cleanup. It
may free the caller request reference itself; its native context remains valid
through the hook. No waiting for producers, guest execution or dispatch reentry
is permitted in release. Inactive handles may outlive their client safely.

## Native producer and one-record delivery / Producer и одна запись

Register a borrowed native readiness fd with `styxnotifyfd`. A producer writes
copied readiness/data, waking the owner from `styxwait`. The owner drains its
mailbox/pipe and completes pending reads; it never needs a guest callback.

An event backend keeps its queue head until it can provide one complete record.
If the request buffer is too short, it can send an ordinary Rerror and retain
the record for a subsequent read. It sends exactly one record on success even
when more records are queued. An empty live source leaves Tread pending; a zero
Rread deliberately means EOF. The library does not invent an event encoding or
consume backend queues automatically. Delivery/replay acknowledgement policies
remain the backend's responsibility; response attempt does not prove receipt.

[Actual C TCP producer example](../src/tools/libstyx/tests/deferred-read-server.c)
uses an independent native producer pipe, bounded synthetic queue, one three-byte
fixture record per read and short-buffer preservation. Its arbitrary strings and
`events` fixture name are test data, not a reserved path or a public event API.

## Cancellation / Отмена

Matching Tflush retires pending read before Rflush; no late original response can
be sent. Unrelated flushes remain usable for heartbeat. Accepted Tversion,
disconnect and server end retire requests before freeing fid/client state.
Invalid version negotiation leaves pending work intact. Clunk/remove of a fid
first sends Rerror to its pending read, then retires fid context and replies to
the clunk/remove. Reusing a pending tag, including as a Tflush envelope, closes
that offending connection rather than sending ambiguous responses.

Ordinary stream/fid semantics and actual operation permissions remain unchanged.
No walk/open authority, prepared root, directory tree snapshot or authentication
is added by deferred read. Native producer events may extend future load/change
reporting without altering this transport lifetime contract.

## Acceptance and remaining scope / Проверки и дальнейшие границы

The native suite has88 tests. Original dispatcher/codec tests cover full-width
fid/Qid/offset, msize, invalid nonconsuming completion, delayed success/error,
independent fids, quota, flush, clunk, invalid/accepted reset, duplicate tags,
context-before-fid cleanup and safe late handles after disconnect. Real TCP tests
cover queued records, short buffers, other operations while pending, cancellation,
reset, clunk, reconnect, server end and duplicate flush tags.

Actual native client/server integration verifies repeated native heartbeat replies
while the server has a real pending Tread, explicit token/Tflush cancellation,
and a fresh read receiving the still-unconsumed record. These are actual server
requests, not delayed replies simulated by a proxy. The proxy forwards unchanged
frames and records tags. ASan/UBSan use `detect_leaks=0` on this host.

This step provides deferred transport and examples, not the final provider event
resource or its public format. Remote subscription/discovery, filtered per-client
change/load records and their access/publication policy remain separate work.
The selected Inferno snapshot also includes the current upstream loopback fixture's
synthetic create/remove support; no graphical implementation is imported.

Теперь обычный Tread может оставаться pending до появления данных. Native producer
будит владельца сокета через pipe/mailbox; владелец продолжает обслуживать heartbeat
и другие запросы. Формат записи задаёт backend. Тест доказывает выдачу одной записи
и сохранение очереди при коротком буфере; общедоступный формат событий не вводится.
Tflush/reset/disconnect/end исключают поздние ответы, а clunk завершает ожидающий
read ошибкой до освобождения fid. Изменения сделаны в исходном Inferno libstyx и
импортированы точно; wire Styx и права доступа не менялись.
