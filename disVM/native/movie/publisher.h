/* Copyright (c) 2026 bhgv
 * EN: Automatic native movie announcements through the existing bounded event outbox.
 * RU: Автоматические native movie announcements через существующий bounded event outbox.
 */
#ifndef STYX_MOVIE_PUBLISHER_H
#define STYX_MOVIE_PUBLISHER_H
#include "offer.h"
#include "../vfs/events.h"


/* EN/RU: Borrow trusted source and unused directional tag; owned outbox/NULL, no network/VM I/O.
 * Subscribes before initial snapshot, including launches that are still unpublished.
 * Read exactly one complete declaration+offer with styxeventsread; poll borrowed styxeventsfd.
 * Host retains a partially sent record until acknowledged, serializing all control traffic.
 * EMPTY is AGAIN, never EOF; CLOSED is delivered as a record, then host may finish the outbox.
 * Each call creates an independent subscriber for one authorized connection, not new authority.
 * Source/tag borrowed; свой outbox/NULL. Подписка до snapshot, одна declaration+offer за read.
 * EMPTY означает AGAIN; CLOSED — запись. Частично отправленный prefix дописывает I/O owner.
 * Очередь независима на connection; новые полномочия не создаются. */
StyxEvents *styxmovieofferwatch(DVM_MovieOffer *source, unsigned long tag);
#endif
