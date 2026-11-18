/* Copyright (c) 2026 bhgv
 * EN: Stage immutable native TX candidates, hold affected subtrees and emit one ordered root-service request.
 * RU: Staging native TX candidates, hold affected subtrees и один ordered root-service request.
 */
#include "../internal.h"


/* EN/RU: Check ancestor proposal overlap and accounting without changing any local mapping or hold.
 * Проверить overlap ancestor proposals и accounting без изменения mapping либо hold.
 */
static int available(StyxScope *parent, const StyxScopeAssignment *list, size_t count)
{
    if (parent->proposal)
    {
        return 0;
    }
    for (size_t i = 0; i < count; i++)
    {
        if (!scopetxmutable(list[i].scope))
        {
            return 0;
        }
        for (StyxScope *scope = list[i].scope; scope; scope = scope->parent)
        {
            if (count > (size_t)-1 - scope->tx_holds)
            {
                return 0;
            }
        }
    }
    return 1;
}


/* EN/RU: See operation.h; all copies allocated before atomic admission, successful proposal retires operation ID.
 * См. operation.h; все copies до atomic admission, успешная proposal навсегда расходует operation ID.
 */
int styxscopetxprepare(StyxScope *parent, unsigned long long operation,
    const StyxScopeAssignment *list, size_t count, const void *request, size_t size)
{
    if (!parent || parent->leaf || !operation || (count && !list) ||
        count > ((size_t)-1 - sizeof(ScopeProposal)) / sizeof(*list))
    {
        return 0;
    }
    ScopeHub *hub = parent->hub; /* Native authority and candidate handles pinned through return. */
    ScopeProposal *proposal = malloc(sizeof(*proposal) + count * sizeof(*list)); /* Private candidate copy. */
    ScopeServiceMessage *message = scopeservicecopy(hub, request, size); /* Private root0 request copy. */
    if (!proposal || !message)
    {
        free(proposal);
        free(message);
        return 0;
    }
    proposal->operation = operation;
    proposal->count = count;
    if (count)
    {
        memcpy(proposal->list, list, count * sizeof(*list));
    }
    pthread_mutex_lock(&hub->lock);
    int valid = !hub->reason && operation > parent->tx_last && hub->service_count != (size_t)-1 &&
        scopeassignlocked(parent, STYX_SCOPE_TX, proposal->list, count, 0) && available(parent, proposal->list, count);
    if (valid)
    {
        parent->proposal = proposal;
        parent->tx_last = operation;
        for (size_t i = 0; i < count; i++)
        {
            proposal->list[i].scope->tx_pending = parent;
            for (StyxScope *scope = proposal->list[i].scope; scope; scope = scope->parent)
            {
                scope->tx_holds++;
            }
        }
        scopeservicequeue(hub, message);
    }
    pthread_mutex_unlock(&hub->lock);
    if (!valid)
    {
        free(proposal);
        free(message);
    }
    return valid;
}


/* EN/RU: See operation.h; this function does not guess duplicate-request semantics or acknowledge rejected RX.
 * См. operation.h; не угадывать duplicate-request semantics и не подтверждать rejected RX.
 */
int styxscoperxcommit(StyxScope *parent, const StyxScopeAssignment *list, size_t count, const void *reply, size_t size)
{
    if (!parent || parent->leaf || (count && !list))
    {
        return 0;
    }
    ScopeHub *hub = parent->hub; /* Host pins all receiver-side capabilities. */
    ScopeServiceMessage *message = scopeservicecopy(hub, reply, size); /* Allocate ACK before any mapping mutation. */
    if (!message)
    {
        return 0;
    }
    pthread_mutex_lock(&hub->lock);
    int valid = !hub->reason && hub->service_count != (size_t)-1 && scopeassignlocked(parent, STYX_SCOPE_RX, list, count, 1);
    if (valid)
    {
        scopeservicequeue(hub, message);
    }
    pthread_mutex_unlock(&hub->lock);
    if (!valid)
    {
        free(message);
    }
    return valid;
}
