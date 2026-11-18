# Active provider cancellation / Отмена активного I/O провайдера

A resource server already inside a native provider callback now receives route
retirement through its **own existing cancellation scope**. Waking its RX/TX
queue conditions alone was insufficient while it was executing metadata, open,
read or consuming close. The server now subscribes to the route before publishing
its worker; no extra guest work, polling loop or per-request watcher thread is
introduced.

RU: Завершение маршрута теперь запрашивает отмену собственного native scope
сервера. Это будит не только очередь запросов, но и уже выполняемый native I/O,
если провайдер поддерживает отмену. Другие операции того же провайдера используют
свои scope и продолжают работать. Гостевые потоки не приостанавливаются.

## Cause and status / Причина и статус

| Cause | Scope and final status |
| --- | --- |
| Route/transport failure during active provider I/O | Request this server's cancellation; worker retires as FAILED. |
| External stop of the route | Request this server's cancellation; worker retires as FAILED. |
| Explicit `styxbidirmovieserverstop` | Latch STOPPED first, then request its native scope; STOPPED survives cleanup. |
| Route already retired before server construction | Sticky replay requests cancellation before provider admission; the returned worker retires as FAILED. |
| Another operation using the same provider | Its independently owned scope is not requested or replaced. |

As before, the first terminal state already published stays first. An explicit
server stop can publish STOPPED before the worker has completed native cleanup;
status is not join. Final free joins that worker and preserves shared route/client
ownership. A previously completed DONE/REFUSED still means terminal reply copied
to TX, not peer receipt or execution acknowledgement.

A route failure never becomes a fabricated explicit host stop or a successful
END merely because the provider returned after cancellation. Before final status
publication the worker checks its cancellation source; explicit stop has already
latched its own distinct terminal status. Accepted TX copies are not retracted
or replayed.

## Native provider contract / Контракт native-провайдера

The same `DVM_CancelScope` reaches the actual namespace-backed service and its
provider operations. A cancellation-aware native provider obtains the current
source with `dvm_cancel_current` and can subscribe using `dvm_cancel_subscribe`
to wake its native I/O mechanism. Its callback must be native, idempotent and
nonwaiting, with independently valid callback state. Remove the tap under the
ordinary nonwaiting-unsubscribe lifetime contract.

Existing capability declarations still apply: `DVM_PROVIDER_CANCEL_PATH` covers
factory path operations, `DVM_VFS_CANCEL_DATA` covers handle data I/O, and consuming
close requires its existing `DVM_VFS_CANCEL_CLOSE` or dynamic close capability.
This adapter does not infer such promises for an arbitrary provider, change
access rights, or add VM callbacks to a provider interface.

Cancellation is a native request, not forced interruption of arbitrary code.
Providers must honour their cancellation contract and bound their own I/O. The
server's original monotonic budget remains checked between operations and during
queue admission; this change adds no timer that preempts a provider or a sink.
Cleanup still consumes each successfully opened handle exactly once, including
when the source was cancelled before close.

RU: Используются прежние capability-флаги и native API отмены. Callback провайдера
будит его собственный механизм I/O; произвольный код не прерывается принудительно.
Сам провайдер ограничивает свой I/O. Закрытие остаётся consuming-операцией даже
после отмены. Правила доступа и формат Styx/S-записей не меняются.

## Subscription ownership / Владение подпиской

The route callback stores only an independently retained cancellation source.
It stores no server, route, provider, guest or VM pointer. Disposal unsubscribes
before releasing the server's own native pins. A callback already admitted by the
source may retire later, safely request the retained source again, and release
its own pin after server disposal. It cannot dereference freed server storage or
stop another connection.

Failure to create the retirement subscription rolls back the private constructor.
Failure to create the worker also removes its already-created subscription.
Caller-owned namespaces, providers and the shared client remain usable. No
weaker server is silently published after subscription failure.

## Verification / Проверка

The [original 9P fixtures](../tests/active_io/host.c) run the actual VFS resource
service over two bidirectional sessions on one fragmented physical carrier.
A native provider blocks separately inside metadata, open, read or close using
condition variables subscribed to its current native scope. While blocked, the
fixture injects a real uncertain acknowledged Twrite, stops the route, or stops
only the server. A simultaneous metadata operation uses the **same factory**
under a separate scope and remains blocked until its own explicit test release.

Checks cover FAILED versus STOPPED, no successful terminal reply after cancelled
active work, balanced consuming closes, shared-route usability after local stop,
ordinary-client survival, constructor subscription/worker rollback, sticky replay
and duplicate admitted callbacks delivered after server free. No fixture binary
is executed as SWF. The [existing managed-loader tests](../../movie/README.md)
retain their own execution scope.

A negative control linked the previous server lifecycle into the same fixture;
it failed at the expected prompt active-provider cancellation check. The test
hooks are not production provider APIs. No heartbeat schema, physical transport
restriction or guest suspension is added.
