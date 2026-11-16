# Deferred Rattach / Отложенный Rattach

Runtime0.144 connects [native publication admission](PEER.md) to original C
libstyx deferred replies. It uses the exact Inferno experiment library pinned by
`../UPSTREAM.json`. No message fields, private wire errors or client retries are
needed for a deferred successful attach. This is an embedding API extension;
rebuild native consumers with the matching `Client` layout and library.

## Native ownership / Native-владение

The service's socket owner runs `Styxops.attach`. A synchronous owner decision
continues to return the usual nil/error. For a pending publication ticket it calls
`styxdeferattach(server, context, release)` and returns nil. Libstyx retains the
original tag, fid and copied effective uname/aname, reserves the fid, and sends
no response until completion/cancellation. Committed client identity is unchanged
while pending. Each client has at most one pending attach; another is refused.
Other requests and other connections continue normally.

A successful defer returns one caller-owned `StyxAttach*` and adopts the release
context. A failed defer takes neither. `styxattachclient` borrows the live pending
client only on the socket owner; it returns nil after retirement. The owner
installs the selected root only after checking that the transaction remains live,
then calls `styxattachreply(request, nil)` for Rattach or an error string for
Rerror. Return1 means consumed/response attempted, not confirmation of receipt;
return0 is inactive/staged with no response. `styxattachfree` consumes the caller
reference. Releasing a caller reference alone does not answer a live request.

`release(context)` runs once on success, denial, flush, reset, disconnect or end,
before the client backend is destroyed. It may own and release the caller's
request handle while libstyx still pins its internal reference. The acceptance
adapter uses it to call `styxpeercancel(peer)`, release the handle and its copied
logical name. `styxpeercancel` removes only an outstanding publication ticket;
a previously selected root remains bound. A late owner reply cannot redeem a
cancelled ticket. Independently retained roots and opens keep normal lifetimes.

All request/session calls are serialized on the socket owner. A worker never
accesses `Client`, fid, `StyxAttach` or guest pointers. Cleanup does not wait for
external work, reenter dispatch/teardown, execute a guest or pause a guest.
A caller-owned inactive request may outlive its client and must be freed later.

## Completion notification / Уведомление о готовности

`styxnotifyfd(server, fd)` registers one borrowed POSIX readiness descriptor,
returning0/-1. Pass-1 to unregister. It rejects out-of-range descriptors and an
existing listener/client descriptor. The host owns draining and closing this fd;
libstyx never consumes its bytes or closes it. Nt and Plan9 return-1 until their
native readiness adapters are implemented.

The TCP fixture uses a dedicated native reviewer and a nonblocking pipe. The
publication's native notify callback signals a condition variable. The reviewer
receives copied requests, chooses a provider with ordinary next/reply, and writes
a pipe notification after its decisions. The socket owner wakes in its existing
native select, processes network input including flush/reset/disconnect, drains
the pipe and completes ready transactions. It inspects pending results only on
notification, never by running a Limbo polling loop or suspending a guest.
Shutdown joins the reviewer before publication callback-context retirement; pipe
closure follows socket-owner end/unregistration.

The fixture's explicit `grant` command is a deterministic test-owner gate so the
client can exercise cancellation before a decision. It is not a public VFS or
network admission command. The production extension supplies deferred lifetime
and readiness integration; the host still supplies authentication, its native
mailbox/reviewer and policy. See actual code in [wire.c](../tests/peer/wire.c),
[review.c](../tests/peer/review.c) and [backend.c](../tests/peer/backend.c).

## Protocol cancellation / Отмена по протоколу

Tflush always gets an immediate Rflush with the flush tag. If oldtag identifies
the pending attach, its request/ticket are retired first. No later Rattach or
Rerror with that oldtag can be sent. The client can then reuse the old tag and
fid. An unrelated flush leaves the pending transaction untouched. A completed
response may precede a later Rflush, following normal 9P ordering.

Accepted Tversion, disconnect and shutdown cancel pending attach before session
cleanup. Reusing an outstanding tag on another ordinary request violates the
protocol and closes that stream; the server cannot emit two ambiguous replies.
The pending fid is reserved against Twalk allocation, with a defensive fid check
again at successful completion. Legacy synchronous attach callbacks retain their
behavior. This change does not add async read/write/flush handling for other
future asynchronous operation types.

Native admission state and wire cancellation are separate. Cancelling a request
does not undo an already completed transaction or implicitly revoke earlier
provider grants. The service installs a new root only on live successful
completion. Provider policy still decides actual read/write/execute access.

## Acceptance / Проверки

The original dispatcher/codec test covers delayed success/error, exact tag/fid
reuse, reserved fids, duplicate defer, reset, immediate callback failure after
defer, duplicate pending tags and late completion after disconnect. TCP tests use
real original POSIX libstyx plus actual VFS publication tickets: progress while
pending, no premature response, completion solely from pipe wakeup, chosen root
content, flush/retry, version reset, disconnect/review and publication close.
A separate shutdown run leaves attach pending at server end. Factory/open counts
must retire exactly. The native library suite now contains17 tests with VFS;
ASan/UBSan runs use `detect_leaks=0` on this host, so no leak-detector claim is made.

Отложенный Rattach завершает native socket-owner по fd уведомления от отдельного
native reviewer. Повторять Tattach клиенту не нужно. Tflush/Tversion/disconnect/
shutdown отменяют незавершённый запрос; поздний ответ старого запроса невозможен.
Guest-потоки не вызываются и не приостанавливаются. Аутентификацию и доверенный
provenance по-прежнему предоставляет host; `uname` пакета не даёт полномочий.
