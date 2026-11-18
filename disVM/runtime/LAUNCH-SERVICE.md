# Bounded launch admission / Ограниченный admission запуска (runtime0.167)

[Native blocking admission wait](LAUNCH-SERVICE-WAIT.md) now connects one-request
queue delivery to independent native handlers, with per-wait cancellation and close.

This native SDK connects [launch tickets](LAUNCH-TICKETS.md) to a bounded owner
queue. It supplies separate owner, submit-only endpoint and review capabilities,
FIFO request selection, native readiness and service close. It does not add a
network listener, authenticated wire exchange, module execution, FD transfer or
FlashVM terminal proxy. No specialized client application is required by this
API, and provider code is not downloaded to the terminal.

Добавлена ограниченная native очередь запросов для владельца сервиса. Подготовка
окружения по-прежнему происходит после решения владельца и до init. Сетевой
адаптер и remote execution остаются следующими этапами.

[Pinned VFS routing and immutable launch addresses](LAUNCH-ROUTE.md) now submit
through the selected owner's capability, preserving owner labels and queue acknowledgement.

## Capabilities / Полномочия

[`vfs/launch/service.h`](vfs/launch/service.h) exposes:

| Capability | Operations |
| --- | --- |
| `DVM_LaunchService` | Derive endpoint, inspect counters/journal, obtain next review, close |
| `DVM_LaunchEndpoint` | Submit a copied launch spec only |
| `DVM_LaunchReview` | Borrow exactly one provider request for describe/prepare/take, then release |
| `DVM_LaunchTicket` | Existing requester state/observe/cancel authority |

Create takes a configured canonical provider system ID and limit1..256.
Submission must name that expected provider. This is an association check, not
authentication: a transport adapter must route an endpoint only through its
proper authenticated admission context and supply already granted terminal roots.
The owner selects its permitted provider root during the existing preparation
gap; no server tree must be transferred to the terminal to make this decision.

A request is still only a copied logical module label, startup bytes and terminal
grant association. No host module path is opened and no code runs during submit,
review, close or take. Resources remain live VFS objects behind names; backend
files/channels/sockets/devices are not classified by the admission mechanism.

## Bound and queue / Квота и очередь

Reservation happens under the service mutex **before** slot or request allocation.
The count includes in-progress reservations, queued requests, active reviews and
taken pairs whose capabilities have not both retired. It is an outstanding
request bound, not a count of running processes or a CPU/memory execution budget.
Copied startup bytes keep the existing per-request bound65536.

Construction happens unlocked, without provider I/O. A second closed-state check
precedes index publication. If close won during construction, submit returnsNULL
and rolls back all request/slot/quota ownership. Allocation refusal also rolls
back the reservation. No partial request becomes visible to the owner.

`next` returns one pending review in FIFO order. Cancelled/refused queue entries
are drained without returning them. A dequeued review retains quota until both
requester and provider capability ownership end. Cancelling a ticket alone does
not let a client create an unbounded number of abandoned active reviews. The
owner must release reviews after joining their preparation/take callers; a
network adapter must release requester tickets when its peer drops them.

Квота учитывает весь outstanding lifecycle. Отмена не создаёт обход лимита для
уже выданного на рассмотрение запроса. `next` возвращает один pending запрос;
отменённые записи очищаются, без выполнения гостевого кода и provider I/O.

## Lifetime and close / Lifetime и закрытие

Slots hold native service references independently from owner/endpoints. A
private final-request hook removes the indexed slot and returns quota before
request storage is freed. Index guards and live-reference checks prevent a
finalizing request from being resurrected by close. Request hooks run outside
request locks and are not native cancellation callbacks; cancellation receivers
still retain only independent native signal/source state.

Close marks the service closed once, then processes indexed requests using
short-lived native pins. No service mutex is held during request cancellation,
namespace cleanup, notifications or root operations. Queued provider wrappers
are drained. Active reviews remain owned by their callers, but unfinished
requests become Denied and preparing native I/O is signalled through the existing
per-request source. Callers must join same-wrapper operations before free.

A take that wins before refusal remains Taken. Its independent handoff survives
service close and destruction, endpoint/requester/review release and later queue
cleanup. Close is not process termination and Taken is not proof that init ran.
Separate run/supervision capabilities remain future work.

Endpoint release does not revoke already submitted requests. Owner release closes
before dropping its own native reference. Remaining endpoints and indexed pairs
keep closed service storage alive until their references retire. A reservation
already in construction completes only to rollback after close.

## Native readiness / Native readiness

`observe` returns an independently retained existing change journal. Submission,
queue changes, request state changes, quota retirement and close post native
invalidation hints outside the service/request mutexes. Native receivers copy
readiness into independent recipient state; they do not execute, pause or poll a
guest. The owner checks queue/counters in its ordinary native work handler.
Hints can coalesce and do not replace authoritative queue/request state.

No event wire format or per-thread device registration is introduced here.
The journal can outlive the service without retaining service/request/root
pointers. Automatic guest delivery will belong to the eventual native launch
adapter/VFS service projection, not a cooperative guest cancellation loop.

## Verification / Проверки

The native service test covers FIFO after cancelled-entry cleanup, aggregate
limits through queue/review/take, wrong destination, native readiness, independent
journal lifetime, closed endpoints and transferred namespace survival. Thirty-two
rounds of eight concurrent submitters exercise limit1 and close races. A private
constructor gate deterministically closes after reservation and before index
publication, requiring complete rollback with no returned ticket. Another
private gate closes while a zero-reference indexed record is finalizing, proving
that close cannot resurrect it or use stale slot storage.

Native blocked-root tests now also close a service while its active review is
preparing: I/O interrupts, no later root is queried and no environment is issued.
Allocation tests inject failure in service creation and40 submission allocation
positions. Quota and queue must return to zero; the existing native allocation/free
index includes service and slot storage. Private controls removing the quota or
closed-publication guard fail in actual service scenarios.

The real interpreter/JIT bootstrap fixture now submits through an endpoint,
reviews one request, prepares and takes its environment, retires the admission
service/capabilities and only then calls appinit. It remains a local fixture, not
evidence of authenticated network launch. Existing namespace/source identities,
per-name ordering and open-handle ownership are unchanged.


The full native suite has275 cases and passes normally and under ASan/UBSan with
leak detection disabled. Both dynamic/static builds pass29 selected VFS/process
cases and41 host-unit cases; the interpreter/JIT pre-init fixture passes ten
repeats. Allocation ownership is checked by the native allocation/free index;
no successful LeakSanitizer run is claimed in this environment.
