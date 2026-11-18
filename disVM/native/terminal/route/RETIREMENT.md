# Retirement under backpressure / Завершение при заполненных очередях

A transport error must wake the native route even when its sole dispatcher is
waiting for room in a full general, UI or typed mailbox. Previously the public
state accessor could expose a TX failure while condition waits remained blocked
until their own deadlines: the dispatcher had not yet propagated that failure.

The route now subscribes to the transport's existing sticky native retirement
source during construction, before publishing its dispatcher. The callback copies
the transport outcome into the route's first terminal reason and signals the
route condition. It does not wait for queue capacity, dispatch guest code, consume
records, close the shared client or recursively stop the transport.

RU: Ошибка TX/завершение транспорта будит route независимо от заполнения очередей.
Раньше статус уже показывал ошибку, а native waits могли ждать собственного
тайм-аута. Теперь отдельное sticky-уведомление сразу будит условия route; гостевые
потоки и формат сообщений в этом не участвуют.

## Queue outcomes / Результаты очередей

| State at retirement | Behavior |
| --- | --- |
| Empty mailbox | Native general/UI/typed waits wake and report no record; the next call reports END. |
| Accepted queued records | Each remains readable, one record per call, then END. |
| Dispatcher waiting with an unaccepted record | It wakes and exits; returning capacity cannot admit this record after retirement. |
| Typed wait with caller cancellation | Route retirement wakes the wait without requesting the caller's cancellation source. |
| Resource receiver/server waiting for typed data | Existing native worker observes the route outcome and retires within its normal local lifecycle. |

Admission here means entry into a route mailbox. A verified transport frame still
waiting in the dispatcher or transport queue has not yet received that guarantee.
No notification creates a batch application event, new S-tag or Styx dialect.
Already accepted output retains the transport's existing precise-prefix and
no-replay rules. Failure does not grant another client, namespace or provider.
A local transport/route stop leaves the shared original 9P client with its host.

## Callback lifetime / Время жизни callback

The subscription owns a small independent native context. Its mutex protects a
borrowed route pointer until detachment. Callback lock order is context, then
short transport/route state locks; no route lock is held while subscribing,
unsubscribing or requesting transport stop.

Rollback and final disposal detach that pointer before nonwaiting unsubscribe
and before freeing transport or route storage. An already-admitted callback may
therefore run later, find no attached route and safely release its own context.
A failed subscription makes construction fail, cleaning up only private fids,
workers and storage. It does not silently publish a route with weaker wakeups.

A reason already latched by routing or explicit stop remains first. Native
notification callbacks are idempotent and do not join their own workers. External
callers must still retire before routefree; notification pinning does not make
arbitrary concurrent free/use valid.

RU: Callback имеет собственный context с mutex и отделяемым указателем на route.
Перед unsubscribe/free указатель обнуляется под этим mutex. Даже уже допущенный
поздний callback не обращается к освобождённому объекту. Отказ создания подписки
откатывает только частное создание route, сохраняя общий Styx-клиент.

## UI and resources / UI и ресурсы

The [UI relay](../bridge/README.md) also subscribes to route retirement. It latches
REMOTE and closes its **owned local channel pair**, waking relays waiting on
empty or full channels. It never recursively stops the route from that callback.
Queued local messages remain drainable, and pending diagnostic copies survive
until the ordinary joined disposal. An earlier LOCAL/STOPPED reason stays first.
Its callback uses the same independent-context/detach lifetime pattern.

The existing announced resource receiver, one-shot resource server and ordinary
queue callers need no guest polling or new API calls. They receive the route's
native condition wake. Provider callbacks already executing native I/O continue
to follow their existing cancellation/deadline contract; this change adds no
forced preemption of an executing provider or sink callback.

## Verification / Проверка

The [original 9P fixture](../tests/retirement/host.c) uses two opposite sessions
on one fragmented physical carrier. A condition-wait hook proves the real native
dispatcher is blocked on a full general/UI/typed mailbox. An actual acknowledged
Twrite prefix is then marked uncertain by a test-only wrapper, or the public
control stop is requested. Tests check prompt native queue/resource wakeups,
preserved queue heads, rejection of the waiting next record, unchanged caller
cancellation state and continued ordinary client use.

Additional cases refuse constructor subscriptions and deliver test-admitted
duplicate callbacks **after** route disposal, retaining callback state until
that late delivery. The real FlashVM channel tests cover idle and full local
queues, sticky retirement before relay construction, subscription failure and
late callbacks after relay/channel disposal. These injection hooks belong only
to fixtures; production does not delay callbacks or inspect private queues.

A local negative control linked the old route lifecycle into the same full-queue
fixture and failed at the expected prompt-resource-retirement check. Normal and
sanitizer results belong to the tested revision. This does not claim new physical
hardware drivers, heartbeat policy or a complete remote application launcher.
