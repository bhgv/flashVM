/* Copyright (c) 2026 bhgv
 * EN: Consume launch requests from a dedicated typed terminal mailbox.
 * RU: Приём запросов запуска из отдельной типизированной очереди терминала.
 */
#ifndef STYX_BIDIR_ROUTE_LAUNCH_H
#define STYX_BIDIR_ROUTE_LAUNCH_H
#include "types.h"
#include "../../launch/request.h"

#define STYX_ROUTE_LAUNCH_REJECTED (-3) /* One record consumed, no ticket issued. */
#define STYX_ROUTE_LAUNCH_INVALID (-4) /* Invalid host inputs, no queue consumption. */


/* EN: Consume at most one record from the host's dedicated launch mailbox index.
 * Borrow the route, spec, paths, endpoint and scratch through
 * return. The terminal root comes from the same
 * client as the route. One consumer owns this mailbox. A nonzero tag selects the authorized
 * application. The scratch capacity is in bytes. On SMALL, size is the needed
 * capacity and the record stays queued. Other outcomes leave size unchanged
 * unless a record was consumed. The caller provides empty ticket storage.
 * RECORD returns an owned pending or subsequently advanced ticket. EMPTY, END,
 * SMALL, REJECTED and INVALID return no ticket. Metadata capture may block this
 * caller after dequeue, outside the dispatcher and mailbox lock.
 * RU: Забрать не более одной записи из выделенной очереди запуска index.
 * Route, spec, paths, endpoint и scratch заимствуются до возврата.
 * Очередь имеет одного потребителя, ненулевой tag выбирает разрешённое приложение.
 * Ёмкость scratch задаётся в байтах. SMALL сообщает нужный size и оставляет
 * запись в очереди. Иначе size меняется лишь при получении записи.
 * Вызывающий передаёт пустой ticket. RECORD выдаёт ticket, прочие исходы без него.
 * Чтение метаданных может блокировать вызывающего после dequeue вне замка. */
int styxbidirroutelaunchnext(StyxBidirRoute *route, unsigned index,
    const DVM_LaunchSpec *spec, unsigned access, const StyxTerminalPaths *paths,
    DVM_LaunchEndpoint *endpoint, unsigned long tag, void *scratch, size_t capacity,
    size_t *size, DVM_LaunchTicket **ticket);
#endif
