# Native remote event feed / Native подписка на удалённые события

Runtime0.151 connects a configured remote event resource to the ordinary mounted
provider's local change journal. A dedicated native I/O thread holds one remote
read, decodes host-selected records and publishes invalidations automatically.
Existing namespace observers route that journal into per-thread native inboxes.
No Limbo loop, guest execution, guest pause, scheduler poll or provider/guest
pointer in the worker is required. Original Styx messages/codecs are unchanged.

## Explicit native configuration / Явная native-конфигурация

`styxmountproviderfeed(client, access, &config, &control)` is a native I/O constructor.
It consumes the owned client on every outcome and returns an ordinary provider
or NULL. Success adopts `config.state`; failure leaves that context caller-owned
and clears optional control output. A fixed READ ceiling and a live fd-owner
client with at least two RPC slots are required. Legacy callback transports and
write-only mount ceilings refuse setup. Original `styxmountprovider` keeps its
connection-health-only behavior.

The host explicitly selects a canonical relative endpoint inside the attached
root and a bounded native decoder. Setup uses ordinary walk/stat/open to validate
a byte file; actual Topen decides access. It does not scan guessed paths, dial
another peer, authenticate wire uname or widen the granted namespace. Path is
borrowed only through construction, then discarded. The constructor is native
I/O and must not be run as a blocking guest callback. Keep the subscription
endpoint private from competing ordinary readers with the prepared namespace/view,
or provide independent server-side subscription queues. This constructor does
not automatically hide a path or split a shared publication inbox.

```c
/* Host already owns a connected client and trusted decode policy/context. */
StyxFeedConfig cfg = {event_path, 8192,
    STYX_EVENT_RIGHTS|STYX_EVENT_NAMESPACE|STYX_EVENT_CONTENT,
    0, decode_event, context, release_context};
StyxFeed *control = NULL;
DVM_Provider *provider = styxmountproviderfeed(client, access, &cfg, &control);
/* On success: bind provider normally; events now reach its journal automatically. */
/* Optional native stop/status capability; provider owns an independent feed ref. */
```

This is configured subscription, not automatic discovery: there is no reserved
path, mandatory codec or new9P negotiation message. The server-side
[event resource](EVENTS.md) and this decoder must be selected consistently by the
native host/prepared connection policy. Guest Sys builtin discovery/binding and
a standardized record schema remain separate work.

## Decoder and source cursor / Декодирование и source cursor

Capacity is1..65536 bytes for bounded incremental assembly. The decoder receives
borrowed bytes and outputs one copied `StyxEvent` plus consumed length:

| Return | Required progress | Meaning |
|---|---|---|
|1 |1..available bytes | One complete record |
|0 |0 | Need more bytes |
|-1 or invalid result | None accepted | Fail the feed |

Need-more on a full buffer, zero progress on success, overconsumption, unknown
classes/flags, zero source epoch/generation and invalid resource scoping fail
closed. Partial records may span reads; several records in one reply are decoded
and published individually. The decoder must be bounded native code with no
guest calls, I/O, feed reentry or retained byte pointers. Its code/context stay
alive through release. There is no default text/binary encoding.

Source session/generation form an independent remote cursor. Older/duplicate
reports are ignored; gaps or epoch changes broaden to the configured class mask
with provider-wide scope. Valid unselected classes still advance that cursor,
avoiding false gaps. `resources=0` clears resource IDs before local publication;
1 explicitly accepts the decoder's provider-local ID mapping. Notifications grant
no access: later provider operations still decide permissions and availability.

Encoding capacity and transport iounit/read limits must suit the remote resource.
An all-or-nothing record resource can refuse a short request; the feed stops and
invalidates rather than guessing a new capacity or replaying uncertain reads.
A source incarnation must use a newer remote epoch when its generation resets.
No implicit reconnect, fid restoration or replay is introduced.

## Long-lived read and heartbeat / Долгое чтение и heartbeat

The native `styxclientreadwatch` primitive uses ordinary original/flush tag fences
but has no ordinary request deadline. Explicit cancellation requires a single-use
token and retains the normal bounded flush grace. Heartbeat and terminal socket
monitoring continue independently; a healthy idle event source does not get
periodically flushed/reopened. Configure the client's existing heartbeat policy
when silent network loss must be detected. Without that policy, observable
EOF/errors and explicit stop still retire the read.

A watch consumes one ordinary RPC slot, and admission limits concurrent watches
to at most limit-1. Thus subscriptions cannot take the last slot reserved for
ordinary operations. Quota remains bounded; no unbounded pending admission queue
is added. `styxclientwatchable` reports capability, not a reservation. Feed startup
and first-read admission are asynchronous; quota failure ends the feed and emits
an invalidation without poisoning a healthy connection. ACTIVE means configured
worker state, not proof of an admitted or acknowledged read.

## Local journal rebasing / Локальная нумерация журнала

`changespost(journal, kinds, flags, resource)` atomically assigns a new local
journal generation. It supports independent native producers without comparing
their unrelated sequence numbers, preserves history/watermarks and never wraps.
Zero means invalid input or exhausted generation space. Existing explicitly
sequenced `dvm_changes_notify` behavior remains available.

Mounted connection-health observations now keep their own source cursor and use
local append too. Remote generation100 and health generation2 therefore cannot
suppress one another. Remote epoch changes do not reset the mount's local journal
epoch or destroy unrelated connection-health watermarks. The remote cursor is
available separately through `styxfeedinfo`. As with existing push delivery,
concurrent callbacks can arrive out of order; recipients retain generation fences
and conservatively refresh after gaps.

## Stop, failure and lifetime / Остановка и lifetime

`styxfeedinfo` returns a copied ACTIVE/EOF/FAILED/STOPPED state, remote cursor,
accepted newer report count and native cleanup-finished flag. EOF means a clean
remote end; EOF with a partial record is FAILED. Decoder/protocol/quota/transport
failure is FAILED. The target journal receives a broad configured-class plus
CONNECTION invalidation when the feed ends unexpectedly or at remote EOF. That
reports event-source availability, not proof that the main TCP peer is dead.
The separate health observer retains its own transport/network meaning.

`styxfeedstop` is nonwaiting. It cancels the current native token or fences the
next admission; it does not wait for decoder return, Rflush, clunk or guest work.
An already admitted decode/publication may finish. Explicit stop itself does not
invent a network failure. Normal EOF/FAILED status is set before its final journal
notification; cleanup-finished may follow after native fid/client retirement.

The provider owns one control reference; optional returned control owns another.
Provider destruction requests stop regardless of extra control owners and returns
without joining. `styxfeedfree` consumes a control owner and requests stop on the
last owner. The detached worker pins only the client/fid and local journal, never
the provider. It releases protocol resources and journal on retirement; a retained
finished status handle does not keep a socket open. Final `release(context)` runs
once after worker and control references retire, outside feed locks. Use that hook
for decoder-state lifetime, not a status snapshot as a callback-quiescence fence.

## Acceptance / Проверки

The native suite has110 tests. New checks include unique local generation allocation
under concurrent producers, source/local epoch separation, scoped refresh, actual
incremental parser progress and malformed records, remote gap/epoch/duplicate fences,
class filtering without false gaps and watch admission preserving ordinary capacity.
Actual TCP tests use the real journal-backed readonly event resource and original
C libstyx. They wait through twelve real heartbeat exchanges beyond the ordinary
RPC timeout, execute ordinary mount metadata while subscribed, route content to
its journal, distinguish EOF/partial EOF/invalid decoding, and reject bad endpoint,
write-only and one-slot setup without adopting policy state.

Controlled fixtures hold Rflush or pause only a native test decoder to prove that
provider destruction returns without waiting. Those gates are not production
cooperation mechanisms; no guest is involved. Both held-flush and in-flight decode
cleanup exercise the actual worker lifetime. Another case combines remote data,
explicit connection shutdown and feed failure and requires independent local
journal generations for all sources. ASan/UBSan run with `detect_leaks=0`.

После native-настройки путь событий читается автоматически, выбранный decoder
выдаёт scalar records, а mount получает обычные local journal invalidations.
Гость ничего не опрашивает и не приостанавливается. Долгий Tread не имеет обычного
RPC timeout, но heartbeat и explicit cancel работают. Нумерация удалённых событий
не смешивается с нумерацией connection-health: общий журнал назначает local generation.
Unmount не ждёт Rflush/decoder; worker удерживает только независимые native resources.
Формат и путь по-прежнему выбирает host; автоматическое discovery этим шагом не добавлено.
