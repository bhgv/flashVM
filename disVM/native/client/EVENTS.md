# Native connection events / Native события соединения

Runtime0.147 connects the native client's terminal state directly to each
mounted provider's change journal. With `styxclientmountfd`, the socket owner
detects idle EOF, transport and framing failure independently of the next
read/write/stat or guest execution. Existing namespace subscriptions then route
the journal event to the owning thread's native inbox. Guest suspension,
scheduler polling, a Limbo notification loop and extra wire requests are not
required. The existing one-event delivery interface and event format are unchanged.

## Native API and meaning / Native API и смысл

`styxclientwatch(client, notify, state, release)` returns an owned `StyxWatch*`
or NULL. State transfers only on success. Keep a client reference while
registering. The first terminal reason is sticky; a late subscription receives
it once, possibly before registration returns. A source accepts at most256 live
watcher nodes, including detached nodes whose caller handle is still retained.

`styxclientunwatch(watch)` consumes the watch without waiting. An already admitted
callback may finish afterward. Its state remains owned until `release(state)`
runs once after both unsubscribe and callback retirement. NULL is accepted.
Callbacks and state release run outside client, engine and source locks. They
may copy native messages and unsubscribe; they must not perform I/O, wait for a
recipient, execute guest code or retain cross-heap pointers. Registration replay
runs on its caller; fd-owner terminal delivery runs on the native socket owner.
Legacy callback transports publish on the native caller observing the failure.

| First observed transition | Client reason | Mounted journal generation2 |
|---|---|---|
| EOF, transport failure, invalid frame/response or terminal flush failure | `STYX_LINK_FAILED` | `CONNECTION \| NETWORK` |
| Explicit `styxclientshutdown` while otherwise live | `STYX_LINK_CLOSED` | `CONNECTION` |
| Normal final `styxclientfree` | Quiet retirement | No synthetic failure event |

Each mount has its own journal/session and subscription, even when several
providers share a retained client. Removing one provider detaches only its
subscription. Repeated failure paths do not duplicate a terminal event. A late
mount records the sticky event in its journal; normal journal cursor/refresh
rules remain in effect. Notifications invalidate cached assumptions and confer
no permission or new namespace authority.

```c
/* Consume an already connected client; the mount subscribes automatically. */
DVM_Provider *provider = styxmountprovider(client, FVM_ACCESS_READ|FVM_ACCESS_EXEC);
/* Existing namespace/inbox subscriptions receive native loss events. */
```

## Lifetime / Время жизни

The client and socket engine share a small native terminal source. The engine
contains no Client, provider, Prog/REG or guest pointer. The mount's callback
owns only a retained journal, preventing a provider/client reference cycle.
`changesretain(journal)` pins an otherwise borrowed native journal independently
of its provider; `dvm_changes_destroy` consumes one reference. The last reference
requires that external borrowers and subscriptions have been retired. A retain
must begin while the caller already pins a live journal.

Provider destruction unsubscribes without joining the native owner or waiting
for admitted callbacks. A callback already in flight keeps the journal alive
until its event publication returns. Socket retirement signals explicit shutdown
waiters before terminal fanout; `styxclientshutdown` guarantees fd retirement,
not that every notification callback has finished. Callback owners use their
own release hooks to retire their recipient storage.

## Limits and checks / Границы и проверки

Idle monitoring applies to the connected-fd owner. The legacy arbitrary
callback-transport API has no independent readiness primitive; it still detects
loss during an exchange or explicit shutdown. Standard9P2000 wire messages and
the original Inferno codecs are unchanged. No unsolicited heartbeat message,
periodic Tstat, reconnect or remote-fid restoration is introduced. A silent
network partition without observable EOF/error or active transaction timeout
still needs a future heartbeat policy. Provider content/rights/load changes
remain separate typed change sources using the existing journal machinery.

The native suite has69 tests, including source replay/quiet/capacity,
self-unsubscribe, detached chain cancellation, last-owner retirement during an
in-flight callback, real idle EOF, explicit shutdown, late mount replay, shared
clients, independent unmount and provider destruction during delivery. The TCP
fixture uses the original C Styx server and verifies that the tested client
sends only Tversion/Tattach: notification itself generates no wire traffic.
ASan/UBSan use `detect_leaks=0` on this host; no LeakSanitizer certification is claimed.

При `mountfd` разрыв обнаруживает native socket-owner даже на простаивающем
соединении. Mount сам подписывается на источник; существующий маршрут namespace
передаёт событие в native inbox потока без участия гостя. События идут по одному,
формат не менялся. У каждого mount свой журнал, удаление одного не отключает
другие. Callback удерживает журнал независимо от provider и завершается безопасно
после unmount; удаление подписки не ждёт callback. Обычное освобождение ресурса
не выдаётся за сетевую ошибку. Heartbeat для молча пропавшей сети и сообщения об
изменении прав, файлов или нагрузки этим шагом не добавлены.
