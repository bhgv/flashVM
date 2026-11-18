/* Copyright (c) 2026 bhgv
 * EN: Bounded native launch admission with separate owner, submit-only endpoint and review capabilities.
 * RU: Ограниченный native admission запуска с отдельными owner, submit-only endpoint и review capabilities.
 */
#ifndef DVM_LAUNCH_SERVICE_H
#define DVM_LAUNCH_SERVICE_H
#include "ticket.h"
#define DVM_LAUNCH_SERVICE_LIMIT 256U /* Maximum outstanding capability pairs, including active reviews. */
typedef struct DVM_LaunchService DVM_LaunchService; /* Owner queue/close authority. */
typedef struct DVM_LaunchEndpoint DVM_LaunchEndpoint; /* Submit-only authority, no owner operations. */
typedef struct DVM_LaunchReview DVM_LaunchReview; /* One dequeued provider request, exclusively owned. */


/* EN/RU: Copied admission counters, not execution/process statistics.
 * Copied admission counters, не статистика запущенных процессов. */
typedef struct DVM_LaunchServiceInfo
{
    size_t outstanding; /* Reservations plus indexed pairs until both wrappers retire. */
    size_t queued; /* Indexed requests not yet reviewed or drained, including cancelled ones. */
    int closed; /* New submissions and owner next are refused. */
} DVM_LaunchServiceInfo;


/* EN/RU: Create owned service/NULL for canonical configured provider system ID and limit1..256.
 * No root, callback, listener, authentication or guest execution is created.
 * Создать owned service/NULL для canonical provider ID и limit1..256 без root/listener/auth/init. */
DVM_LaunchService *dvm_launch_service_create(const char system[DVM_ORIGIN_ID_BYTES], size_t limit);


/* EN/RU: Derive owned submit-only endpoint/NULL from live owner; closed refuses.
 * Получить owned submit-only endpoint/NULL из owner; closed отклоняет. */
DVM_LaunchEndpoint *dvm_launch_service_endpoint(DVM_LaunchService *owner);


/* EN/RU: Submit copied spec through borrowed endpoint; return owned requester ticket/NULL.
 * Expected provider must match service; capacity includes reservations and reviewed requests.
 * No provider I/O or implicit execution. Подать spec через endpoint; owned ticket/NULL,
 * provider должен совпасть, quota включает reservation/review; без I/O/init. */
DVM_LaunchTicket *dvm_launch_service_submit(DVM_LaunchEndpoint *endpoint, const DVM_LaunchSpec *spec);


/* EN/RU: Dequeue exactly one pending request as owned review/NULL, draining cancelled queue entries.
 * No provider I/O; no target polling. Owner cannot next after close.
 * Получить один pending request как owned review/NULL, убрать отменённые entries; без I/O/polling. */
DVM_LaunchReview *dvm_launch_service_next(DVM_LaunchService *owner);


/* EN/RU: Outcome of one blocking native queue operation; never a process/init completion status.
 * Результат одного blocking native queue operation, не статус исполнения process/init.
 */
typedef enum DVM_LaunchWaitResult
{
    DVM_LAUNCH_WAIT_REQUEST = 1, /* Exactly one owned review was dequeued. */
    DVM_LAUNCH_WAIT_CLOSED = 0, /* Service close observed without consuming another request. */
    DVM_LAUNCH_WAIT_CANCELLED = -1, /* This native wait's current cancellation scope fired. */
    DVM_LAUNCH_WAIT_FAILED = -2 /* Invalid input or native wait/subscription allocation failure. */
} DVM_LaunchWaitResult;


/* EN/RU: Block the calling native service-owner thread until one request, close or current-scope cancellation.
 * Borrow live owner through return; out is required and cleared before work, receives owned review only on REQUEST.
 * Pre-cancel consumes no queued request. Once dequeued, a review wins concurrent cancellation/close and remains owned.
 * Cancellation affects this wait only; cancelled queue entries drain as with next. FAILED consumes no pending review.
 * Multiple handlers may wait; each request has one winner, not a broadcast copy or promised handler fairness.
 * No provider I/O, guest callback, VM admission/ledger entry, pause, polling, timer or global execution stop.
 * Join all same-owner users before free; close may run concurrently. Call only outside managed admission.
 * Блокировать native поток service-owner до одного request, close либо current-scope cancellation.
 * Owner borrowed до return; out обязателен, очищается, owned review только при REQUEST.
 * Pre-cancel не потребляет очередь; принятый review сохраняется при racing cancellation/close.
 * Отмена касается лишь этого wait; один request получает один handler, fairness не обещается.
 * Без provider I/O, guest callbacks/паузы/polling/VM admission. Users join до free, close допускается параллельно.
 */
DVM_LaunchWaitResult dvm_launch_service_wait(DVM_LaunchService *owner, DVM_LaunchReview **out);


/* EN/RU: Borrow provider request only while review lives and its callers are joined before free.
 * Use existing describe/prepare/take; requester never obtains this capability through submit.
 * Заимствовать request до review free, использовать describe/prepare/take; submit не выдаёт это право. */
DVM_LaunchRequest *dvm_launch_review_request(DVM_LaunchReview *review);


/* EN/RU: Read owner counters under mutex;1 success/0 invalid, no I/O.
 * Читать owner counters под mutex;1/0, без I/O. */
int dvm_launch_service_info(DVM_LaunchService *owner, DVM_LaunchServiceInfo *out);


/* EN/RU: Return independent owned native change journal/NULL; hints require queue/status inspection.
 * Native notice recipients only signal readiness; no guest/owner execution in callbacks.
 * Вернуть independent owned journal/NULL; hints требуют проверки очереди, callbacks только readiness. */
DVM_Changes *dvm_launch_service_observe(DVM_LaunchService *owner);


/* EN/RU: Close once;1 changed/0 already closed. Deny unfinished indexed requests, cancel preparing I/O,
 * drain queued provider wrappers, retain active reviews and already transferred environments.
 * No provider I/O under service mutex; callers keep owner alive through close.
 * Закрыть один раз; unfinished requests отклонить, preparing I/O отменить, queue очистить;
 * active reviews/transferred environments сохранить, без provider I/O под mutex. */
int dvm_launch_service_close(DVM_LaunchService *owner);


/* EN/RU: Consume exclusive owner after its callers join; closes before releasing its native reference.
 * Поглотить owner после join callers; закрыть до release reference, NULL допустим. */
void dvm_launch_service_free(DVM_LaunchService *owner);


/* EN/RU: Consume submit endpoint after its callers join; existing tickets/reviews survive, NULL allowed.
 * Поглотить endpoint после join callers; tickets/reviews сохраняются, NULL допустим. */
void dvm_launch_endpoint_free(DVM_LaunchEndpoint *endpoint);


/* EN/RU: Consume review after preparation/take callers join; refuse unfinished provider request.
 * Quota retires after requester ticket also drops. NULL allowed.
 * Поглотить review после join; unfinished request отклонить, quota после удаления ticket, NULL допустим. */
void dvm_launch_review_free(DVM_LaunchReview *review);
#endif
