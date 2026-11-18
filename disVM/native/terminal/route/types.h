/* Copyright (c) 2026 bhgv
 * EN: Generic typed S-record mailboxes on the existing bidirectional terminal dispatcher.
 * RU: Общие типизированные очереди S-записей существующего bidir-диспетчера терминала.
 */
#ifndef STYX_BIDIR_ROUTE_TYPES_H
#define STYX_BIDIR_ROUTE_TYPES_H
#include "route.h"
#ifdef __cplusplus
extern "C" {
#endif
#define STYX_ROUTE_TYPES_MAX 8U /* Bound per-session typed queues independently of peer declarations. */


/* EN/RU: Borrowed constructor configuration, copied before any worker or I/O starts.
 * Конфигурация заимствована только до возврата конструктора, копируется до I/O и запуска workers.
 */
typedef struct StyxRouteType
{
    const char *type; /* Agreed printable type identity, 1..FVM_SBINDINGS_TYPE_MAX bytes. */
    unsigned long version; /* Agreed nonzero unsigned 32-bit schema version. */
} StyxRouteType;


/* EN: Create an owned dispatcher/NULL; borrow client/channels/types only through setup.
 * The same grant and binding-declaration RX profile as route.h are required. ui_type/version
 * optionally select the existing UI decoder; NULL/0 disables it for console/data-only sessions.
 * Copy count schemas (0..TYPES_MAX), indexed in caller order. Each gets slots entries with limit
 * payload bytes; total storage is bounded by (count+1)*slots*limit plus UI/transport storage.
 * Reject duplicate type/version pairs, including the UI pair, before I/O. Version mismatches,
 * unknown tags and all declarations remain general; queued records are never reclassified.
 * UI and typed queues have one consumer each, general still must be serviced. Full queues apply
 * lossless backpressure to this dispatcher, never a guest pause; they can delay later traffic.
 * RX associations confer no TX authority. External users must retire before routefree.
 * RU: Создать свой dispatcher/NULL, скопировать count схем до I/O. NULL/0 отключает UI.
 * Индексы — порядок types. Каждая очередь ограничена slots*limit; дубликаты запрещены.
 * Неизвестные записи/версии и объявления остаются общими, старые записи не переклассифицируются.
 * Полная очередь задерживает только native dispatcher, но может задержать последующий трафик.
 * RX не даёт TX-прав. Перед routefree завершить внешних пользователей.
 */
StyxBidirRoute *styxbidirbindingroutes(StyxClient *client, const DVM_TerminalChannels *channels,
    unsigned limit, unsigned slots, const char *ui_type, unsigned long ui_version,
    const StyxRouteType *types, unsigned count);


/* EN: Nonblocking copy of exactly one payload from indexed typed queue; same statuses as next.
 * Borrow all arguments; one consumer per queue may race other queues/send/stop. SMALL changes
 * only size and preserves head/tag/bytes. Empty/ended/invalid calls preserve all outputs.
 * Empty payload is a record, not EOF; its nonzero numeric tag is copied as received.
 * RU: Неблокирующая копия одной записи очереди index. SMALL меняет только size, сохраняя
 * head/tag/bytes; EMPTY/END/ошибка не меняют outputs. Пустая запись — не конец потока.
 */
int styxbidirroutetypednext(StyxBidirRoute *route, unsigned index, unsigned long *tag,
    void *bytes, size_t capacity, size_t *size);


/* EN/RU: Native-only readiness of indexed queue, 1 ready/0 timeout/end/invalid; consume nothing.
 * One monotonic 0..120000ms budget, route borrowed. No guest code or callbacks are dispatched.
 * Native-ожидание очереди index: 1 есть запись/0 timeout/end/ошибка, без потребления или guest-кода.
 */
int styxbidirroutetypedwait(StyxBidirRoute *route, unsigned index, unsigned timeout);
#ifdef __cplusplus
}
#endif
#endif
