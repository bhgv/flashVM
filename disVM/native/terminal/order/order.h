/* Copyright (c) 2026 bhgv
 * EN: Independent native per-tag sequence/reorder queues and acknowledged local rollover, no generation per packet.
 * RU: Независимые очереди восстановления порядка по тегам и подтверждённый сброс без поколения в каждом пакете.
 */
#ifndef STYX_ORDER_H
#define STYX_ORDER_H
#include "flashvm/sorder.h"
#ifdef __cplusplus
extern "C" {
#endif
#define STYX_ORDER_RX 0U /* Independent receiver counter in this namespace/direction. */
#define STYX_ORDER_TX 1U /* Independent transmitter counter, never inferred from RX. */
#define STYX_ORDER_ACCEPTED 1 /* Complete verified packet copied, ready or held behind its tag's gap. */
#define STYX_ORDER_DUPLICATE 2 /* Old or identical held sequence, never executed twice. */
#define STYX_ORDER_WINDOW 3 /* Future gap exceeds configured transport reorder window; other tags unaffected. */
#define STYX_ORDER_REFUSED 0 /* Invalid/stopped input, or pending boundary would be crossed. */
#define STYX_ORDER_NOMEM (-1) /* Exact packet copy/counter allocation failed, no partial acceptance. */
#define STYX_ORDER_CONFLICT (-2) /* Different payload claimed an already held sequence. */
#define STYX_ORDER_RECORD 1 /* Exactly one local copied payload. */
#define STYX_ORDER_EMPTY 0 /* No ready record for selected tag while open. */
#define STYX_ORDER_END (-1) /* Selected queue empty after connection owner closed. */
#define STYX_ORDER_SMALL (-2) /* Required size only, ready head/ordinal preserved. */
typedef struct StyxOrder StyxOrder; /* Native owner of one namespace's independent directional tag counters. */


/* EN/RU: Owned engine/NULL, no worker or transport; body limit 1..S_BODY_MAX, reorder window 1..2^63-1.
 * Host feeds validated lanes on native broker threads; clients wait for their own ready tag.
 * Вернуть собственный engine либо NULL, без worker/transport; limit ограничивает тело записи, window — перестановку номеров, а не длину очереди клиента.
 */
StyxOrder *styxordercreate(unsigned limit, unsigned long long window);


/* EN/RU: Native-only seed an unused directional counter before peer traffic,1/0; default starts at0.
 * Existing/used direction cannot be overwritten. Caller agrees initial value with peer; no inferred wire command.
 * Задать first для неиспользованного tag/direction: 1/0; по умолчанию ноль. Начальное значение явно согласуется с другой стороной.
 */
int styxorderstart(StyxOrder *order, unsigned long tag, unsigned direction, unsigned long long first);


/* EN/RU: Encode one packet into caller-owned disjoint spool, advance TX only on OK; no transport I/O.
 * Returned packet must be retained/retried as the SAME bytes until sent or connection retired, not re-encoded.
 * Pending rollover/MAX exhaustion returns CONFLICT for only this tag. Output/used unchanged on failure.
 * Кодировать пакет в буфер caller; TX меняется только при OK. Для повторной передачи сохранять те же байты, не назначать новый номер.
 */
FVM_SResult styxorderencode(StyxOrder *order, unsigned long tag, const void *bytes, size_t size,
    void *output, size_t capacity, size_t *used);


/* EN/RU: Validate and copy exactly one complete prefixed packet, return acceptance/duplicate/window/error above.
 * Caller retains input through return; no direct client callback, gap in one tag never blocks others.
 * Проверить и скопировать один пакет с префиксом; результаты перечислены выше. Пропуск одного тега не блокирует другие.
 */
int styxorderfeed(StyxOrder *order, const void *packet, size_t size);


/* EN/RU: Copy one ready selected-tag record/SMALL/EMPTY/END; successful pop advances consumed checkpoint.
 * One consumer per tag, output/ordinal unchanged except successful copy; SMALL changes only size.
 * Выдать одну готовую запись выбранного тега и отметить выданный номер; SMALL меняет только size, не потребляя запись.
 */
int styxordernext(StyxOrder *order, unsigned long tag, unsigned long long *ordinal,
    void *bytes, size_t capacity, size_t *size);


/* EN/RU: Block only selected-tag native consumer under one monotonic budget 0..120000ms, 1 ready/0 otherwise.
 * Ждать только выбранный тег: 1 — готов, 0 — иначе. Без потребления пакета и остановки брокера.
 */
int styxorderwait(StyxOrder *order, unsigned long tag, unsigned timeout);


/* EN/RU: Native queued-reset proposal for ONE counter; strictly increasing nonzero local operation ID.
 * TX boundary must equal last spooled ordinal. RX boundary may wait for its old prefix to arrive/be consumed.
 * 1/0, no wire traffic; host queues service request after old packets and ACKs only when ready returns1.
 * Подготовить сброс одного счётчика: 1/0. Host сам передаёт служебные request/ACK; остальные теги не затронуты.
 */
int styxorderprepare(StyxOrder *order, unsigned long tag, unsigned direction,
    unsigned long long operation, unsigned long long boundary);


/* EN/RU: Copy readiness of this exact pending rollover,1/0; RX requires consumed boundary and empty held/ready queues.
 * Готовность точно выбранного сброса: 1/0. RX требует выдачи boundary и пустых очередей held/ready.
 */
int styxorderready(StyxOrder *order, unsigned long tag, unsigned direction, unsigned long long operation);


/* EN/RU: Apply matching accepted ACK(1)/explicit NACK(0),1/0; exact most-recent repeats idempotent.
 * ACK resets only this directional counter to0. No automatic wrap; wrong/not-ready confirmation changes nothing.
 * Native host authenticates session/operation and retires old lane deliveries before accepting rollover.
 * Точное ACK сбрасывает лишь этот tag/direction в ноль: 1/0. Ошибочное либо преждевременное подтверждение ничего не меняет.
 */
int styxorderconfirm(StyxOrder *order, unsigned long tag, unsigned direction,
    unsigned long long operation, int accepted);


/* EN/RU: Non-consuming old-prefix checkpoint,1/0; packet merely held/ready is not considered consumed.
 * Native host uses this per-tag watermark before applying assignment requests across lanes.
 * Проверить отметку выданного номера тега: 1/0. Записи в held/ready ещё не выданы; host использует отметку при переназначении.
 */
int styxorderreached(StyxOrder *order, unsigned long tag, unsigned long long boundary);


/* EN/RU: Close connection owner, wake all native waits, keep ready records individually drainable.
 * Закрыть соединение и разбудить ожидающих; готовые записи остаются доступны для поштучного чтения.
 */
void styxorderclose(StyxOrder *order);


/* EN/RU: Consume after all feed/encode/consumer calls retire; NULL harmless, no owned worker to join.
 * Освободить после завершения всех вызовов; NULL безопасен, собственных workers нет.
 */
void styxorderfree(StyxOrder *order);
#ifdef __cplusplus
}
#endif
#endif
