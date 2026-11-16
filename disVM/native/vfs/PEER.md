# Per-connection publication admission / Admission публикации на соединение

Runtime 0.143 adds `peer.h`, with the trusted host constructor in
`peer_provider.h`. These are native adapters around the existing publication
request/prepare/reply API; the original libstyx codec and wire messages are unchanged.

## Ownership and authority / Владение и полномочия

The host creates `StyxPeer` while its borrowed publication owner is live. The
constructor copies a trusted peer label, provenance flags and a fixed access
ceiling. `uname` from Tattach must never supply any of those authority inputs.
`aname` may supply the logical publication name, subject to owner admission.
A logical name is not resolved as a host path.

`DVM_EXPORT_REMOTE` requires `DVM_EXPORT_AUTHENTICATED` attested by a verified
transport. This adapter does not implement transport authentication. Local test
connection slots are explicitly preauthorized fixture identities; they do not
prove that TCP authenticates clients. The provider's operations still make the
final access decision, and the publication grant restricts requested rights.

Хост создаёт `StyxPeer` с доверенным native-контекстом, provenance и пределом
доступа. `uname` пакета не удостоверяет пользователя. `aname` — логический запрос,
который владелец может отклонить или обслужить другим подготовленным provider.
Удалённый транспорт должен отдельно удостоверить peer. Здесь аутентификация
не реализуется и новые права не возникают.

| Function | Ownership and result |
|---|---|
| `exportendpoint(owner)` | Owned submit-only capability; create before owner close |
| `exportrequest(endpoint, request)` | Owned ticket or NULL; ordinary publication admission |
| `exportendpointfree(endpoint)` | Consume endpoint after its callers finish |
| `styxpeer(owner, trusted, flags, access)` | Owned connection context or NULL; inputs copied |
| `styxpeerattach(peer, name, kind, &root)` | 1 with owned root, 0 pending, -1 refused; clears output otherwise |
| `styxpeerfree(peer)` | Cancel outstanding ticket, release selected provider and endpoint |

The endpoint pins publisher storage without exposing next/reply/close authority.
Stop and join active publisher calls before owner close and callback-context
retirement. Idle endpoints may remain alive: subsequent requests refuse before
using borrowed callbacks. Existing tickets and grants retain their normal lifetime.
Endpoint release does not close the owner. Serialize peer calls; serialize endpoint
release against its submitters.

## Native use / Пример native API

```c
/* Called by a trusted host, before publication owner close. */
StyxPeer *peer = styxpeer(owner, verified_peer, provenance,
    FVM_ACCESS_READ | FVM_ACCESS_EXEC);
StyxBacking *root = NULL;
int status = styxpeerattach(peer, logical_name, 2, &root);
/* 1: caller owns root. 0: owner review pending. -1: refused. */
/* On connection retirement, after callbacks finish: */
styxbackingfree(root);
styxpeerfree(peer);
```

`kind` is 0 for unspecified, 1 for file, 2 for directory. A requested kind is
checked against the selected provider and cannot manufacture a directory/file.
A pending request cannot be replaced by another name or kind. Repeating it does
not allocate a second ticket. After grant, repeating the same request produces
an independent backing for the same selected provider, without reauthorization.
The selected namespace remains live; its contents are not copied or frozen.
Changing the connection's selected publication requires a new peer context after
its users retire. Tversion does not reset this native selection.

`styxpeerfree` cancels pending review; a late owner reply is refused. Closing the
publisher denies unanswered requests. Granted roots and independently retained
opens remain usable, subject to current provider policy. Publisher shutdown is
not a new revocation mechanism; a provider policy can revoke actual operations.

## libstyx boundary / Граница libstyx

The POSIX acceptance fixture uses `Client.u` for a session, creates its native peer
in `newclient`, admits a directory publication in `attach`, and releases it in
`freeclient` after libstyx retires fid contexts. Visited node capabilities are
per client, even if different providers return equal Qids. Each open still has
its own `styxsetaux`/`styxgetaux` context.

Runtime0.144 adds [deferred Rattach and native readiness integration](DEFERRED.md).
An attach callback can now explicitly defer with a native request handle. A separate
native reviewer signals the socket owner, which sends the original-tag response
without a client retry. Flush/reset/disconnect/shutdown cancel it first. Legacy
callbacks remain synchronous; authentication and a general mount service are not
provided by this adapter. The callback still returns a directory Qroot; projecting
a single file as the protocol root remains separate work.

Native `styxpeercancel(peer)` cancels only its outstanding publication ticket,
without generating a wire reply or dropping a previously selected root. The
libstyx retirement callback uses it when the transaction is cancelled.

Отложенный Rattach и native fd уведомления описаны в [DEFERRED.md](DEFERRED.md).
Продвижение не зависит от Limbo polling и не приостанавливает guest-потоки.

## Acceptance / Проверки

`vfs-peer-wire` exercises native pending review, one notification per request,
late replies after cancellation, close while pending, closed endpoints without
callbacks, kind mismatch, a prepared single-file grant, and retained opens after
all issuers retire. Real TCP then checks two clients with identical wire `uname`,
logical names and Qids against different host-selected roots; rejected attach
rollback; root binding across Tversion and attempted name/user substitution;
publisher close with an unadmitted endpoint; independent opens after owner close;
and exact open/factory retirement. See `../tests/peer/`.
