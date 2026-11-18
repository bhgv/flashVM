/* Copyright (c) 2026 bhgv
 * EN: Private dynamic copied FIFO; receiving never waits for application consumption.
 * RU: Частная динамическая FIFO копий; приём не ждёт потребления приложением.
 */
#ifndef STYX_ROUTE_MAILBOX_H
#define STYX_ROUTE_MAILBOX_H
#include <stddef.h>


/* EN/RU: One FIFO owned by its route; all pointers/counters protected by the route mutex.
 * FIFO принадлежит route; все pointers/counters защищены mutex route.
 */
typedef struct RouteMailbox
{
    struct RouteRecord *head; /* Owned oldest accepted record, NULL when empty. */
    struct RouteRecord *tail; /* Borrowed newest record in the owned chain. */
    size_t count;             /* Unread records; empty payload still counts once. */
    size_t bytes;             /* Unread copied payload bytes, excluding allocation metadata. */
    size_t peak_count;        /* Historical maximum occupancy. */
    size_t peak_bytes;        /* Historical maximum copied payload bytes. */
} RouteMailbox;
struct StyxBidirRoute; /* Owner pins FIFO and native synchronization. */


/* EN/RU: Copy one borrowed record into owned mailbox; 1 accepted/0 stopped or NOMEM, no capacity wait.
 * Route/mailbox pinned through return; real allocation failure retires route, never silently drops.
 * Скопировать borrowed record в FIFO; 1 принято/0 stop либо NOMEM, без capacity wait.
 */
int routeenqueue(struct StyxBidirRoute *route, RouteMailbox *mailbox, unsigned long tag,
    const void *bytes, size_t size);


/* EN/RU: Caller holds route mutex; copy/pop one record, SMALL preserves all but required size.
 * Borrow output through return; terminal controls empty result only. No I/O or callbacks.
 * Под mutex route выдать одну запись; SMALL меняет только required size, terminal лишь для empty.
 */
int routemailboxnext(RouteMailbox *mailbox, int terminal, unsigned long *tag,
    void *bytes, size_t capacity, size_t *size);


/* EN/RU: Consume all records after worker/external users retire; NULL not accepted, zero state harmless.
 * Поглотить все records после завершения worker/users; zero state безопасно, NULL не допускается.
 */
void routemailboxclear(RouteMailbox *mailbox);
#endif
