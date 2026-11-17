# Original Styx server over PHY / Исходный Styx-сервер через PHY

`infernostyx_phy` runs the **unchanged imported libstyx protocol core** on an
already granted [RemotePhy](../../../../remote_phy/README.md). It has no network
listener, IP address, socket requirement or raw-descriptor read loop. The original
POSIX backend remains available as the separate `infernostyx` target.

Серверная сторона теперь использует ту же границу физического транспорта, что и
native-клиент. Ядро libstyx компилируется из точного vendored source через небольшой
[wrapper](core.c); переименованы только init/process entry points для native owner
scoping. Сам vendored source, 9P-сообщения, codecs, tags/fids, права и callbacks не
меняются. Новые аппаратные драйверы и общий мультиплексор логических сессий этим
изменением не добавлены.

## Embedding / Подключение

Link **one** platform backend in a program: `infernostyx_phy` or `infernostyx`.
They intentionally implement the same original platform hooks. Native clients
can coexist with either server backend. A host that accepts TCP/other connections
itself can grant each connected descriptor/carrier to its own PHY server instance;
this backend does not create an implicit listener or choose peers.

```c
Styxserver server = {0};
Styxops ops = {0};
/* Configure original callbacks, authentication/admission and prepared exports. */
char *error = styxphyinit(&server, &ops, granted_phy, 0555, 1, 8);
/* On its one native I/O owner: */
while (!error) {
    error = styxwait(&server);
    if (!error) {
        error = styxprocess(&server);
    }
    /* Complete ready deferred application work only on this owner, respecting space. */
}
styxend(&server);
```

The sketch omits host termination policy and error handling after initialization.
Include original `lib9.h`, `styx.h`, `styxserver.h`, then this backend's
[`server.h`](server.h). The backend header itself needs no Inferno type definitions:
it uses forward declarations and the carrier API.

`styxphyinit` consumes the supplied carrier on **every** outcome. It requires a
fresh zeroed server shell, an original callback table retained through end, a
connected reliable ordered byte stream and a reply queue bound1..128 frames.
Native init/wait/process/deferred replies/end are serialized on one I/O owner.
Do not recursively process/end the same server from its callbacks.
Driver wake and wake-pin release are the only cross-thread operations here.
Calling old listener-style `styxinit` in this backend returns an explicit error.

Each instance admits one already connected peer, then the normal original 9P
version/attach/authentication path runs. Carrier possession or a user/name hint
never grants additional VFS authority. The host still selects the authorized
export and implements its existing admission policy. The original `Client.fd`
and `server.connfd` fields hold **owner-local internal tokens** in this backend;
applications must not pass them to OS fd functions.

## Bounded native I/O / Ограниченное native I/O

| Component | Behavior / Поведение |
| --- | --- |
| RX | One frame buffer of original MSGMAX bytes. Read the size prefix, reject lengths outside7..MSGMAX, then stage a complete frame before exposing input to the original core. |
| TX | Fixed ring of1..128 MSGMAX-sized slots. Copy each encoded reply and retain its precise confirmed prefix through short writes/AGAIN. Frames never interleave. |
| Wait | Pump bounded native I/O, then use `phywait`; return for complete input, terminal state, local wake or newly freed reply slots. No busy-empty guest loop. |
| Backpressure | Stop admitting new input while output slots are full. Application deferred completions must check available space. Exhausting reply capacity during an unchecked send fails the peer. |
| Failure | Uncertain/terminal output fails the carrier; no automatic replay or reconnect. A partial input cannot become an executed incomplete request. |

Queued reply acceptance is local buffering, analogous to successful socket send;
it does not certify peer receipt or execution. The original core still determines
permissions and request semantics. A physical error does not turn a failed VFS
operation into EOF. Queues contain encoded frames, not namespace/tree snapshots.
Live provider/namespace behavior remains with the existing callbacks.

`styxphyspace(server)` returns free reply slots after reserving capacity for
already staged/current original-core input. `styxphypending(server)` reports exact
unsent encoded bytes. The native owner calls `styxphyflush(server, timeout_ms)`
for explicit graceful output drain:1 drained,0 pending/failure. A timeout preserves
the known tail. The single monotonic budget is not renewed on partial progress.
`styxend` is an explicit close and discards any remaining queued bytes; flush first
when graceful delivery is required. No destructor waits for guest cooperation.

## Autonomous producer wake / Пробуждение от producer

The owner obtains an independent `StyxPhyWake*` with `styxphywakepin(server)`.
A native producer enqueues copied work in its own bounded native queue and calls
`styxphywake(pin)`. The waiting owner returns from PHY wait, processes its ready
work and invokes original `styxreadreply`/other completions itself. Producers
never receive original request/fid/client pointers and do not execute server or
guest callbacks.

Каждый producer удерживает собственный pin до завершения callbacks. `styxend`
освобождает серверные reads/fids/clients и закрывает wire, а оставшиеся pins
удерживают native wake context. Уже начатая доставка может безопасно вызвать wake
после end; она не возобновляет wire и не обращается к server shell. Последний
`styxphywakefree` освобождает carrier и staging storage. Это native lifetime,
без приостановки потоков гостей и без scheduler safe points.

The POSIX-specific `styxnotifyfd` cannot register descriptors here; -1 unregister
is harmless and other values refuse. Use native wake pins rather than assuming
the carrier wait is an OS poll set. A generic source subscription adapter may pass
an owned wake pin as its callback state and release it after in-flight callbacks
retire. Its source queue remains responsible for bounds and event ordering.

The original deferred-read API still owns Tflush/reset/clunk/disconnect retirement
and callback order. Cancelling one pending read does not consume a later source
event. An unused-oldtag Tflush used as the current heartbeat probe is handled by
the same original core, independently of the pending event read. No heartbeat
payload or new 9P opcode is introduced.

## Acceptance / Проверки

- Both actual native peers communicate over a raw PTY: **no TCP/UDP/socketpair**.
  Test adapters cap reads3/writes2, forcing real request/reply fragmentation.
- A native watch read remains pending while ordinary file RPC progresses. Native
  producer wake completes it; cancel and disconnect retire it through original
  deferred-read/fid/client cleanup.
- An independent Python 9P oracle checks metadata/rwx, denied write-open, ordinary
  reads alongside a pending read, unused-tag probe, Tflush/reissue, source event
  and oversized/malformed length refusal.
- The queue test uses the actual original protocol core with descriptor-free
  callbacks: bounded output pressure, exact encoded reply bytes, flush timeout,
  short writes, uncertain-prefix no-replay, constructor rejection and post-end
  wake-pin lifetime.
- Server end is called twice; deferred requests retire exactly once before client
  cleanup. The native producer deliberately signals after end before dropping
  its last context pin.

All 296 Linux64 SDK tests pass normally and under ASan/UBSan, LeakSanitizer
disabled. The public header also compiles as C89 and C++98. Vendored files remain unchanged. Existing POSIX/TCP VFS and actual
FlashVM player tests continue through their original selected backend. This does
not claim complete FlashVM-host-over-PHY integration, hardware validation or a
symmetric two-session mux on one physical carrier; those remain subsequent work.

## Two directions on one carrier / Два направления на одном носителе

The optional [native stream mux](../../../../remote_phy/mux/README.md) now supplies
two independent carriers for a native client and this original server at each
peer. Both sides explicitly choose the transport extension; 9P messages and
namespace permissions stay unchanged. See its guide for roles, bounded credits,
shutdown and actual simultaneous PTY acceptance.
