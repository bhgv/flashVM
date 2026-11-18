# Autonomous resource server / Автономный сервер ресурсов

[`styxbidirmovieserver`](server.h) starts one native worker that receives one
existing `flashvm.movie` version 1 request from an exclusively assigned typed
Unibus queue, executes it through the **actual VFS resource service**, and sends
its response bodies in S-records. No per-application server program, Limbo poll
loop, guest pause, new opcode or new numeric tag is required.

RU: Готовый native worker принимает существующий запрос из typed-очереди,
обращается к настоящему VFS resource service и выдаёт S-ответы. Обработчик
не требует отдельной программы приложения, Limbo polling или остановки guest.

## Authority and lifetime / Права и время жизни

The trusted host supplies a prepared `DVM_Namespace`, the exclusive resource RX
queue index, a separately granted outgoing resource tag and a finite budget.
RX registration must match `MOVIE_TYPE/MOVIE_VERSION`. The route must allow
`MOVIE_FRAME + 32` bytes per record; timeout is `1..120000` milliseconds.
Construction retains the namespace in a real service and starts one native
worker; it performs no transport setup or implicit announcement.

The group stays **live**. The constructor does not copy/freeze a tree or its
permissions. Bind/mount/provider changes before actual VFS operations are seen
normally; held handles retain their existing identity semantics. The provider
makes the final access decision, including refusing an existing named resource.
Request names and returned origins confer no native authority or host-path access.

The host keeps the borrowed route alive through final server free and services
UI/general/other queues separately. It may release its original group reference
after successful construction. Failure preserves caller ownership and leaves no
surviving worker. One server consumes at most one request, without automatically
consuming a second queued request. Retire the logical resource channel before
reuse after an incomplete exchange.

Do not install both a client and a server as consumers of the same RX queue.
This existing protocol has no request correlation IDs or role dispatcher. Use
separate granted logical resource channels for simultaneous opposite resource
exchanges. Other agreed types can share each general Unibus connection normally.

RU: Host задаёт уже разрешённый namespace и отдельные RX/TX grants. Namespace
удерживается живым, не превращается в снимок. Worker принимает один запрос;
повтор остаётся очереди и не открывает второй ресурс. Один RX-потребитель на
очередь; одновременные встречные resource exchanges требуют отдельных channels.

## Native API / Native-интерфейс

| API | Result |
| --- | --- |
| `styxbidirmovieserver` | Owned server/NULL; starts the native owner after all state is initialized. |
| `styxbidirmovieserverstate` | WAITING, WORKING, DONE, REFUSED, STOPPED, FAILED; -1 invalid. |
| `styxbidirmovieserverwait` | Bounded native terminal-state wait, not a guest loop or worker join. |
| `styxbidirmovieserverstop` | Sticky native cancellation; shared route remains owned by the host. |
| `styxbidirmovieserverfree` | Stop/join only the owned worker, then release service/state; 1 consumed/0 join failure, NULL succeeds. |

For an already prepared group and route:

```c
StyxBidirMovieServer *server = styxbidirmovieserver(route, resource_index,
    granted_reply_tag, prepared, 5000);
if (server != NULL)
{
    dvm_namespace_release(prepared);
    /* Other native consumers service UI/general while this owner waits. */
    if (styxbidirmovieserverwait(server, 5000))
    {
        /* Inspect styxbidirmovieserverstate(server): queued result or failure. */
    }
    /* Retire external status/stop callers before final free. */
    styxbidirmovieserverfree(server);
}
```

WAITING means no provider operation has been admitted. A valid request moves to
WORKING; malformed input fails before opening the service. DONE means success
END has been **copied to the shared TX queue**, not peer execution/receipt ACK.
REFUSED means the existing provider failure END has been copied. Independent TX
copies survive server free. STOPPED can be published before provider cleanup;
free still joins the worker before consuming its state. The first terminal
reason is preserved.

## Backpressure and cancellation / Backpressure и отмена

One monotonic budget covers request waiting, VFS processing and response queue
admission. The server streams bounded chunks; it never buffers the whole source.
Every reply is validated against the native service's private binding and then
reframed using the separately granted external TX tag. Private binding records
do not leak into the shared outgoing control profile.

`styxbidirroutesendwait` / `styxbidircontrolsendwait` wait for TX capacity before
atomic one-record acceptance. Timeout zero polls. Optional `DVM_Cancel` wakes
only that native waiter; it neither stops the shared transport nor retracts or
replays an accepted frame. FIFO applies to accepted records; simultaneous
producers have no priority/fairness reservation before acceptance. Physical
partial writes retain the existing precise-prefix/no-replay behavior.

The worker propagates an independent native cancellation scope to VFS callbacks.
Callbacks must honour/bound their own I/O; the server checks cancellation and
budget between operations and never forcibly preempts native provider code.
Stop wakes native RX/TX queue waits. It does not pause a guest, join foreign
workers, close the shared client or discard another producer's queued messages.

RU: Весь обмен имеет один deadline. При полном TX worker ждёт свободное место,
а не повторяет уже принятую запись и не накапливает файл в памяти. Отмена будит
только его native wait. Native provider ограничивает собственный I/O; чужой код
не прерывается принудительно. Принятые TX-копии остаются у общего transport.

## Acceptance and remaining work / Проверки и дальнейшая работа

The [host fixture](../tests/server_host.c) runs the production server with the
[real prepared VFS provider](../tests/exchange_provider.c). The independent
[original libstyx peer](../tests/server_wire.c) sends known request vectors and
validates binary replies over fragmented PTY. Cases cover one-slot streaming,
permissions, malformed requests, idle cancellation/timeout, blocked TX, uncertain
prefixes, a second request, constructor thread failure, live namespace changes
and data-only operation. UI/general traffic and ordinary 9P remain independently
usable. Fault gates are test-only, not new physical drivers.

[TX wait tests](../tests/send_wait.c) separately cover full queue timeout,
cancellation races, sticky pre-cancel, capacity release and invalid input before
waiting. Actual FlashVM loading remains in the existing managed-player tests;
the binary source used here is not executable SWF content.

This supplies the native provider-side handler for an already granted channel.
Automatic channel negotiation/advertisement, correlated parallel requests and
the proposed `file/state` VFS control nodes remain separate integration work.
