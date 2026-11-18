# Native admission wait / Native ожидание admission

[`dvm_launch_service_wait`](vfs/launch/service.h) lets an independent native
service handler block until it owns **one** pending launch request, its operation
is cancelled or the service closes. It extends the existing bounded
[admission service](LAUNCH-SERVICE.md); `next` remains the nonblocking alternative.
No guest, Unibus broker or unrelated handler is paused while this handler waits.

Отдельный native-поток обработчика ждёт один запрос, готовит разрешённое окружение
и передаёт результат контроллеру исполнения. У каждого обработчика своё ожидание;
отмена одного не закрывает сервис, его очередь или ожидания остальных. Очередь
запросов запуска и очереди трафика Unibus остаются разными механизмами.

## API and ownership / API и владение

```c
DVM_LaunchReview *review = NULL;
DVM_LaunchWaitResult result = dvm_launch_service_wait(service, &review);
if (result == DVM_LAUNCH_WAIT_REQUEST)
{
    DVM_LaunchRequest *request = dvm_launch_review_request(review);
    /* Inspect, authorize, prepare and optionally take using the existing API.
     * request is borrowed only while review lives. */
    dvm_launch_review_free(review);
}
```

| Result | Meaning and ownership |
| --- | --- |
| `DVM_LAUNCH_WAIT_REQUEST` | Exactly one owned review; caller joins its users and frees it |
| `DVM_LAUNCH_WAIT_CLOSED` | Service close observed; output is NULL |
| `DVM_LAUNCH_WAIT_CANCELLED` | This wait's current native cancellation scope fired; output is NULL |
| `DVM_LAUNCH_WAIT_FAILED` | Invalid input or native setup allocation/subscription failure; output is NULL |

`service` is borrowed throughout the call. The output pointer is required and is
cleared before work. Owner close may run concurrently, but **all** owner users,
including waiting handlers, must join before the owner wrapper is freed.
Cancellation uses the caller's existing `DVM_CancelScope`; native threads do not
implicitly inherit a parent's scope. Without a scope, wait ends on request/close.
There is no periodic timer or polling interval.

Pre-cancellation consumes no queued request. A successfully dequeued review wins
a racing cancellation and remains owned even if the source later fires. Service
close can still deny the review's unfinished request under existing admission
rules; receiving a review is not permission to skip status/authorization checks.
Close never restores a consumed environment or proves application init ran.

Several handlers may wait on the same owner. Each request has one queue-claim
winner; readiness is not a broadcast copy of the request. Queue request order
keeps the existing FIFO contract, while handler fairness is not promised.
Cancelled queue entries drain without delivery. Setup failure leaves pending
requests available for a later call. The existing outstanding capability bound
and preparation gap are unchanged; no new Unibus traffic quota is introduced.

Получение review не запускает код и не выдаёт лишних прав. Проверка запроса,
подготовка namespace и одноразовый take остаются обязанностью владельца сервиса.
После take контроллер запускает принятый код на выбранной стороне исполнения.

## Wake protocol / Протокол пробуждения

The wait subscribes to the existing service journal **before** checking its queue.
An independent native recipient contains only its lock, reference count, one
coalesced readiness bit and one sticky cancellation bit. It contains no service,
request, provider, process, heap or guest pointer. Notice callbacks only update
these bits and signal the portable native condition.

Each loop clears the readiness hint before authoritative queue/close inspection.
A publication during that inspection therefore stays visible before sleep. A
publication after taking the wait mutex is signalled after the condition's atomic
unlock/sleep. Spurious wakeups recheck predicates. No service lock is held across
sleep or callbacks, and no provider I/O occurs during wait setup/selection.

The caller and each subscription independently pin the recipient. Unsubscribe
never joins already admitted callbacks. After cancellation the waiting thread can
return and its caller can retire the owner while a previously admitted readiness
callback finishes. Its independent pin keeps the recipient/condition alive;
final native release frees them after all admitted callbacks retire.

Callbacks не исполняют guest, не обращаются к VM admission/host wait ledger и не
останавливают чужие потоки. Это SDK для native service handler **вне managed
admission**, а не новый blocking builtin, который можно вызывать из managed-кода
без его штатной границы native I/O/cancellation.

## Actual verification / Реальная проверка

Native tests use deterministic condition/queue gates, with no timing sleeps:

- Pre-cancel with two queued requests; exact one-request FIFO delivery.
- Cancelled queue head followed by one pending request.
- Submission, close and cancellation at an actual native sleep and in the exact
  empty-queue-check/sleep gap.
- Cancellation/close after an acknowledged queue claim, preserving its review owner.
- Eight waiting handlers; cancelling seven leaves the eighth functional.
- Close waking all eight handlers.
- Thirty-two rounds of eight handlers competing for one request, requiring one
  actual review owner and seven closed outcomes.
- Refusal of all six actual wait setup allocation positions; pending work and
  quota survive, subsequent wait succeeds and the native allocation index drains.
- An admitted journal callback held before delivery while the wait is cancelled,
  joined and its service owner freed. Late real delivery must remain valid and
  free exactly one recipient allocation afterward.

The existing32-case source-built provider-module test now uses this actual
native handler: service submit → blocking receive → prepare → take → controller
startup → real Dis application/dependency calls. Both interpreter and JIT retain
their namespace, cancellation, parser failure and image-ownership checks.
Admission itself never receives a Module/Prog or executes foreign guest code.

Two privately compiled negative controls also fail semantically in these actual
fixtures: discarding a claimed review after cancellation is rejected, and an
extra unretired native recipient pin fails the exact free observation. Compile/
link failures, crashes and timeouts do not count as those semantic detections.

The full native SDK passes729/729 tests ordinarily and under ASan/UBSan.
Four affected runtime cases also pass with the fully instrumented runtime.
ASan/UBSan runs use matching fully instrumented native SDK/runtime builds.
LeakSanitizer is disabled in the restricted environment; explicit allocation,
recipient-free and provider-retirement observations cover the new paths.

## Scope / Граница этапа

This is the native serving loop boundary, not completion of transparent remote
`load`/typed calls. It adds no network listener, message dialect, physical carrier,
serialized pointer, module-interface marshaller or source-role restriction.
A provider process may call VFS-visible modules on the terminal or third systems;
those callees must execute on their owning side with their prepared environments.
The remaining routing/marshalling work must preserve that symmetric model.
