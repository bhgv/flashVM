# Independent recipients / Независимые адресаты

The native Unibus broker blocks waiting for incoming S-records. Each application
consumer runs on its own DisVM/FlashVM execution thread or native adapter and
blocks waiting for its own recipient queue. A waiting consumer does not own the
shared channel reader. An unread recipient backlog never makes the dispatcher
wait for that consumer to release capacity.

RU: Блокирующее поведение сохраняется отдельно у брокера и у клиентов. Брокер
ждёт входящую S-запись, клиент — запись именно в своей очереди. Очереди растут
динамически; медленный клиент не задерживает служебную часть и других адресатов.

## Admission and waiting / Приём и ожидание

General, UI, typed and native service inboxes are independent dynamic FIFOs.
Each accepted record owns an exact payload copy and native metadata. The sole
dispatcher appends it under a short mutex and wakes consumer conditions. It
does not call the application, inspect execution state or wait for consumption.
Empty-payload records are retained as individual messages.

Existing `next` APIs copy at most one message. SMALL changes only the required
size and preserves the head. Existing native `wait`/cancelable-wait APIs sleep
on queue conditions until their selected queue has data, retires, times out or
the caller cancels. Waiting is ordinary consumer blocking, not polling or forced
guest suspension. Consumers never read the underlying Unibus transport directly.

The constructor's `slots` still selects the small bounded native RX/TX transport
staging ring. It no longer caps a recipient FIFO. The dispatcher drains RX into
recipient-owned copies without a consumer-capacity wait. Transport/native I/O
staging may briefly wait for its own broker to run; it does not wait for an
application's recipient queue to shrink.

Stop and failure preserve every already accepted recipient record, not merely
the former fixed-slot head. They drain one per call before END. Final joined
disposal releases any remaining nodes. Genuine allocation failure or counter
exhaustion latches `STYX_CONTROL_NOMEM`, retires only the route/owned transport
and leaves the shared client with its host. There is no silent dropping, replay
or inference that an application has hung.

## Tag zero / Нулевой тег

Tag zero is channel-service traffic. The existing canonical first payload integer
identifies its subservice/version. Version1 binding declarations are validated
and applied immediately on the broker. Their existing general-queue copies are
only native-host observations; leaving those copies unread does not delay later
bindings or channel reception.

Other subservice versions go into a separate native system-service inbox.
[`styxbidirrouteservicenext`](traffic.h) reads one unchanged body and
`styxbidirrouteservicewait` blocks only that native handler. Such records do not
go to application typed/UI queues or fail merely for being a different service.
The channel-service owner must validate its agreed payload before acting; opaque
admission does not execute an unknown command or grant any authority.

No new command IDs or wire schemas are assigned. This change does not implement
automatic extra-channel negotiation, change media formats or make every existing
tag-zero codec an automatic service handler.

RU: Bindings version1 обрабатывает сам broker. Остальные versioned tag0 records
получает native владелец системных служб отдельной очередью. Приложения получают
свои nonzero-теги; wire-схемы служебных команд здесь не изобретаются.

## Diagnostics / Диагностика

[`styxbidirroutequeueinfo`](traffic.h) copies a coherent snapshot of a chosen
general/UI/service/typed queue: current records and payload bytes plus historical
peak records and bytes. Invalid queries leave output untouched. Bytes exclude
allocator metadata; UI bytes measure the copied decoded native message. A
snapshot does not consume data or change the broker's behavior.

These counters provide a basis for future growth warnings in `sysmessages` on
both ends. Threshold policy, warning format, side/client identity, oldest-message
age and rates are not implemented here. Debugger mechanisms decide execution
hangs; Unibus must not turn a slow queue into a forced application stop.
Balancing recipients and privileges remain separate policy. Additional physical
or logical channels increase transport throughput, not consumer execution speed.

## Verification / Проверка

The [six original-Styx stress scenarios](../tests/dynamic/cases.cmake) leave
all queues unread while 512 messages accumulate for a selected general/UI/typed
recipient. They then admit 64 other tag-zero service records, apply a late
binding and route its marker to an independent typed recipient. This completes
before any consumer drains its backlog. Exact binary FIFO order, service
isolation, SMALL, counts/peaks and individual draining are checked.

Three scenarios inject a real malloc refusal only at the late marker's native
record allocation. They require NOMEM, preserve all earlier accepted records
and verify that the shared original client remains live. Existing retirement
tests now checkpoint committed backlog instead of waiting for the removed
recipient-capacity block. Raw native transport-ring retirement is still tested
separately from application recipient queues.

The stress fixture also starts two independent native waiters before publication:
service readiness and the late typed recipient. Each blocks only on its selected
queue and wakes without draining the accumulated recipient. A negative control
restoring the former recipient-capacity wait fails UI admission while the initial
general backlog remains unread.
