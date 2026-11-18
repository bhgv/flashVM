/* Copyright (c) 2026 bhgv
 * EN: Explicit bidirectional ordered broker; native root authority and isolated fixed-tag client handles.
 * RU: Явный двунаправленный брокер порядка; native-полномочия владельца и изолированные handles клиентов.
 */
#ifndef STYX_ORDERED_BROKER_H
#define STYX_ORDERED_BROKER_H
#include "control.h"
#include "order.h"
#ifdef __cplusplus
extern "C" {
#endif
struct DVM_Cancel; /* Borrowed native cancellation capability, never a guest pointer. */
typedef struct StyxOrdered StyxOrdered; /* Native owner, never an application capability. */
typedef struct StyxOrderedClient StyxOrderedClient; /* Borrowed fixed inner-tag capability until owner free. */


/* EN: Consume control on every outcome; own broker/NULL. Predeclare unique permitted inner tags before workers start.
 * limit1..control_limit-64, control_limit>=256; window1..2^63-1. No implicit upgrade of another live broker.
 * RU: Поглотить control всегда; вернуть свой брокер/NULL. Разрешённые внутренние теги задаются до запуска workers.
 * limit и window ограничивают тело/перестановку. Другой работающий брокер автоматически не обновляется.
 */
StyxOrdered *styxbidirordered(StyxBidirControl *control, unsigned limit, unsigned long long window,
    const unsigned long *tags, size_t count);


/* EN: Native-only look up predeclared tag, borrowed capability/NULL; tag0 here remains application-local.
 * RU: Native-поиск объявленного тега; заимствованный handle/NULL. Внутренний тег 0 остаётся клиентским.
 */
StyxOrderedClient *styxorderedclient(StyxOrdered *peer, unsigned long tag);


/* EN: Propose this TX direction once, queue named root0 service; strictly increasing nonzero operation and lane!=0.
 * 1 admitted/0 refused; only ACCEPT activates TX. Explicit REJECT permits a later proposal, timeout never cancels.
 * RU: Предложить TX-направление, поставить службу root0 в очередь: 1/0. Только ACCEPT включает TX.
 */
int styxorderedoffer(StyxOrdered *peer, unsigned long long operation, unsigned long lane);


/* EN: Wait for this exact latest offer's ACCEPT/REJECT, 1 resolved/0 timeout, unknown or terminal.
 * Does not imply acceptance; inspect TX readiness. Timeout leaves the proposal untouched.
 * RU: Ждать ACCEPT/REJECT точного последнего предложения: 1 — операция завершена, 0 — timeout/неизвестно/соединение завершено.
 * Результат не означает согласия; проверить готовность TX. Timeout не отменяет предложение.
 */
int styxorderedofferwait(StyxOrdered *peer, unsigned long long operation, unsigned timeout);


/* EN: Copy directional readiness or wait under one monotonic timeout0..120000ms, 1 ready/0 otherwise.
 * Native callers retain peer through return; neither function consumes records.
 * RU: Проверить готовность направления либо ждать с единым timeout: 1/0. Записи не потребляются.
 */
int styxorderedready(StyxOrdered *peer, unsigned direction);


/* EN: Native snapshot of accepted-once directional agreement,1/0, including after retirement.
 * Diagnostic/history only: not liveness, transmit permission or permission to restart an ended session.
 * RU: Native-снимок ранее принятого согласования:1/0, в том числе после завершения.
 * Только история: не liveness, не разрешение TX и не право возобновить завершённую сессию.
 */
int styxorderedagreed(StyxOrdered *peer, unsigned direction);


/* EN: Wait for one directional activation, 1 ready/0 timeout or terminal; no record consumption.
 * RU: Ждать включения одного направления: 1 — готово, 0 — timeout/завершено; записи не потребляются.
 */
int styxorderedwait(StyxOrdered *peer, unsigned direction, unsigned timeout);


#define STYX_ORDERED_SENT 1 /* One immutable packet admitted, not peer delivery. */
#define STYX_ORDERED_WAIT 0 /* Selected profile/counter still pending at deadline. */
#define STYX_ORDERED_END (-1) /* Broker retired or invalid input. */
#define STYX_ORDERED_SIZE (-2) /* Payload exceeds negotiated body capacity, no ordinal consumed. */
#define STYX_ORDERED_NOMEM (-3) /* Spool allocation refused before numbering. */
#define STYX_ORDERED_REFUSED (-4) /* Explicit offer rejection or exhausted counter after NACK. */


/* EN: Copy one payload into retained TX spool, 1 accepted/0 refused; no I/O or delivery promise.
 * Fixed client tag only, no root0 access. At MAX queue automatic renewal after the last old packet.
 * RU: Скопировать payload в очередь TX: 1/0, без I/O и обещания доставки. При MAX автоматически запросить сброс.
 */
int styxorderedclientsend(StyxOrderedClient *client, const void *bytes, size_t size);


/* EN: Native-only send with one monotonic budget0..120000ms, detailed status above.
 * Waits for TX profile/this tag's renewal only, then copies once under the same lock.
 * Payload/client borrowed through return. Timeout preserves negotiation and consumes no number.
 * RU: Native-отправка с единым timeout и подробным статусом. Ждать только профиль TX/сброс своего тега,
 * затем однократно скопировать под тем же mutex; timeout не отменяет согласование и не расходует номер.
 */
int styxorderedclientsendwait(StyxOrderedClient *client, const void *bytes, size_t size, unsigned timeout);


/* EN: Immutable maximum payload for this fixed tag under local ceiling, zero for invalid client; no I/O.
 * RU: Неизменяемый максимум payload своего тега по локальному ceiling; ноль при неверном client, без I/O.
 */
size_t styxorderedclientceiling(StyxOrderedClient *client);


/* EN: Native sticky retirement subscription, owned tap/NULL; state adopted only on success.
 * Callback may replay before return, must be nonwaiting/native, no guest/provider I/O or recursive peer stop/free.
 * Release follows all admitted callbacks; unsubscribe with dvm_cancel_unsubscribe. Peer pinned during setup.
 * RU: Native-подписка на завершение, свой tap/NULL; state принят только при успехе. Callback может прийти
 * до возврата, не ждёт и не исполняет гостя/I/O/free. Release после callbacks; peer удерживается при setup.
 */
struct DVM_CancelTap *styxorderedwatch(StyxOrdered *peer, void (*receive)(void *), void *state, void (*release)(void *));


/* EN: One copied record, order result codes; SMALL changes only size. Native wait only this client's tag.
 * Inputs borrowed, one consumer per handle; next may queue a ready renewal ACK without I/O.
 * RU: Одна копия записи; SMALL меняет только size. Ожидание блокирует лишь поток этого клиента.
 */
int styxorderedclientnext(StyxOrderedClient *client, unsigned long long *ordinal, void *bytes, size_t capacity, size_t *size);


/* EN: Block only this client's native consumer, 1 queued/0 timeout or terminal; owner pinned through return.
 * RU: Блокировать только native-потребителя этого клиента: 1 — запись есть, 0 — timeout/завершено.
 */
int styxorderedclientwait(StyxOrderedClient *client, unsigned timeout);


/* EN: Native-only early renewal of one TX tag; 1 request queued/0 refused. RX/other TX tags continue.
 * Existing reliable ordered carrier provides the old-delivery barrier; additional lanes need their own barrier.
 * RU: Native-запрос раннего сброса одного TX-тега: 1/0. Остальные теги и RX продолжают работу.
 */
int styxorderedrenew(StyxOrdered *peer, unsigned long tag);


/* EN: Native-only read one unrecognized root service, RECORD/EMPTY/END/SMALL; no application access.
 * RU: Native-чтение одной неизвестной службы root0; клиентские handles сюда доступа не имеют.
 */
int styxorderedservicenext(StyxOrdered *peer, void *bytes, size_t capacity, size_t *size);


/* EN: Native-only wait for root-service inbox, timeout0..120000ms, 1 queued/0 timeout or terminal.
 * Ready notices remain visible after stop; peer pinned, no consumption or transport access.
 * RU: Native-ожидание root-службы: 1 — есть запись, 0 — timeout/завершено; без потребления и I/O.
 */
int styxorderedservicewait(StyxOrdered *peer, unsigned timeout);


/* EN: Copy sticky control reason; stop wakes all waits and preserves ready records. Free after all external users retire.
 * Never free from broker workers; consumes owned control and clients, NULL harmless.
 * RU: Скопировать причину завершения; stop будит ожидания и сохраняет готовые записи. Free после внешних users.
 */
int styxorderedstate(StyxOrdered *peer);


/* EN: Native nonjoining stop, wakes waits and preserves ready payloads; NULL harmless.
 * RU: Native-stop без join; будит ожидания и сохраняет готовые записи. NULL безопасен.
 */
void styxorderedstop(StyxOrdered *peer);


/* EN: Consume owner/control/client handles after external users retire, joining both broker workers.
 * Never call from workers. NULL harmless. No forced guest suspension or payload draining.
 * RU: Поглотить владельца/control/handles после внешних users, завершить workers через join.
 * Не вызывать из workers. Без принудительной остановки гостей и потребления их записей.
 */
void styxorderedfree(StyxOrdered *peer);

/* EN/RU: Borrow pinned client/source; wait only its inbox,1 ready/0 cancel, timeout, end or allocation failure.
 * Cancellation consumes no message and never closes the broker. timeout0..120000ms, NULL source allowed.
 * Заимствовать client/source; ждать лишь свой inbox:1 готово/0 отмена, timeout, end либо OOM.
 * Отмена не потребляет запись и не закрывает broker, timeout0..120000ms, NULL source допустим.
 */
int styxorderedclientwaitcancel(StyxOrderedClient *client, unsigned timeout, struct DVM_Cancel *source);


#define STYX_ORDERED_CANCELLED (-5) /* Caller cancellation won before packet admission; no ordinal consumed. */


/* EN/RU: Same sendwait contract with native cancellation; CANCELLED affects only this call, never peer/client lifetime.
 * Borrow source/client/bytes through return; one monotonic deadline, NULL source preserves ordinary behavior.
 * Тот же sendwait с native-отменой; CANCELLED относится лишь к вызову, не lifetime peer/client.
 * Source/client/bytes borrowed до возврата; один monotonic deadline, NULL source сохраняет обычное поведение.
 */
int styxorderedclientsendwaitcancel(StyxOrderedClient *client, const void *bytes, size_t size,
    unsigned timeout, struct DVM_Cancel *source);


#define STYX_ORDER_CANCELLED (-3) /* Operation-local RX cancellation, head remains unread. */
#define STYX_ORDER_WAIT_NOMEM (-4) /* Cancellation subscription refused, no receive effect. */


/* EN/RU: Atomically wait/copy one selected record under one deadline; ordinary next results plus CANCELLED/WAIT_NOMEM.
 * Borrow client/output/source through return. SMALL preserves the head; timeout gives EMPTY. No broker stop or guest servicing.
 * Атомарно ждать/копировать одну выбранную запись за один deadline; статусы next плюс CANCELLED/WAIT_NOMEM.
 * Client/output/source borrowed; SMALL сохраняет head, timeout даёт EMPTY. Без stop broker и обслуживания guest.
 */
int styxorderedclientnextwaitcancel(StyxOrderedClient *client, unsigned long long *ordinal,
    void *bytes, size_t capacity, size_t *size, unsigned timeout, struct DVM_Cancel *source);

#ifdef __cplusplus
}
#endif
#endif
