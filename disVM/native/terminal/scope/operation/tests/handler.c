/* Copyright (c) 2026 bhgv
 * EN: Native ordered fixture ACK adapter, atomic RX replies and safe self-replacement of an admitted handler.
 * RU: Native ordered fixture ACK adapter, atomic RX replies и безопасный self-replacement admitted handler.
 */
#include "handler.h"
#include <string.h>


/* EN/RU: Arm one native allocator refusal on this exact broker thread; no other worker is affected.
 * Включить allocator refusal на этом broker thread, не затрагивая другие workers.
 */
extern void operationallocation(int);


/* EN/RU: See handler.h; state pin must survive the entire admitted callback, even after replacement.
 * См. handler.h; state pin живёт до конца admitted callback даже после replacement.
 */
void operationrelease(void *state)
{
    OperationHandler *handler = state; /* Borrowed main-owned test context. */
    if (__atomic_load_n(&handler->inside, __ATOMIC_ACQUIRE))
    {
        __atomic_store_n(&handler->bad_release, 1, __ATOMIC_RELEASE);
    }
    __atomic_add_fetch(&handler->released, 1, __ATOMIC_RELEASE);
}


/* EN/RU: Exact fixture body match, no production version/schema guessed.
 * Exact fixture body match, без угадывания production version/schema.
 */
static int is(const void *bytes, size_t size, const char *value)
{
    return size == strlen(value) && !memcmp(bytes, value, size);
}


/* EN/RU: Receiver maps only its preauthorized native siblings and queues ACK atomically after commit.
 * Receiver меняет лишь preauthorized native siblings и queues ACK атомарно после commit.
 */
static int receiver(OperationHandler *handler, StyxScope *root, int invalid)
{
    unsigned long tag; /* Native-only current tag, not supplied by peer. */
    unsigned long long a; /* Exact local expected generation for A. */
    unsigned long long b; /* Exact local expected generation for B. */
    if (!styxscopebinding(handler->a, STYX_SCOPE_RX, &tag, &a) ||
        !styxscopebinding(handler->b, STYX_SCOPE_RX, &tag, &b))
    {
        return STYX_SCOPE_SERVICE_ERROR;
    }
    StyxScopeAssignment swap[] = {{handler->a, 12, a}, {handler->b, invalid ? 12UL : 11UL, b}}; /* Explicit host authorization. */
    if (STYX_OPERATION_CASE == 4 && !invalid)
    {
        operationallocation(0); /* Fail ACK copy before RX mutation, then native NACK must preserve prior map. */
    }
    const char *yes = invalid ? "rx-ok:5" : "rx-ok:4"; /* Fixture-only successful response body. */
    const char *no = invalid ? "rx-no:5" : "rx-no:4"; /* Fixture-only unchanged-map response body. */
    int committed = styxscoperxcommit(root, swap, 2, yes, 7); /* Never emits yes before the map commit. */
    if (!committed && !styxscopeservicesend(root, no, 7))
    {
        return STYX_SCOPE_SERVICE_ERROR;
    }
    return STYX_SCOPE_HANDLED;
}


/* EN/RU: See handler.h; unknown service stays in native inbox, recognized confirmations resolve exact operations.
 * См. handler.h; unknown service остаётся в native inbox, confirmations разрешают лишь exact operations.
 */
int operationhandler(void *state, StyxScope *root, const void *bytes, size_t size)
{
    OperationHandler *handler = state; /* Independently pinned by native handler registration. */
    __atomic_store_n(&handler->inside, 1, __ATOMIC_RELEASE);
    int result = STYX_SCOPE_PASS; /* Native observer messages stay ordinary root inbox records. */
    if (is(bytes, size, "ack:1"))
    {
        result = styxscopetxconfirm(root, 1, 1) ? STYX_SCOPE_HANDLED : STYX_SCOPE_SERVICE_ERROR;
    }
    else if (is(bytes, size, "ack-group:1"))
    {
        result = styxscopetxconfirm(handler->group, 1, 1) ? STYX_SCOPE_HANDLED : STYX_SCOPE_SERVICE_ERROR;
    }
    else if (is(bytes, size, "nack:2"))
    {
        result = styxscopetxconfirm(root, 2, 0) ? STYX_SCOPE_HANDLED : STYX_SCOPE_SERVICE_ERROR;
    }
    else if (is(bytes, size, "request-rx:4") || is(bytes, size, "request-rx:5"))
    {
        result = receiver(handler, root, is(bytes, size, "request-rx:5"));
    }
    else if (is(bytes, size, "replace-handler"))
    {
        result = handler->next && styxscopeservicehandler(root, operationhandler, handler->next, operationrelease) &&
            !__atomic_load_n(&handler->released, __ATOMIC_ACQUIRE) && styxscopeservicesend(root, "replacement-ready", 17) ?
            STYX_SCOPE_HANDLED : STYX_SCOPE_SERVICE_ERROR;
    }
    else if (is(bytes, size, "ack:invalid"))
    {
        result = STYX_SCOPE_SERVICE_ERROR;
    }
    __atomic_store_n(&handler->inside, 0, __ATOMIC_RELEASE);
    return result;
}
