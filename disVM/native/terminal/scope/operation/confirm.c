/* Copyright (c) 2026 bhgv
 * EN: Exact ACK/NACK correlation, atomic TX activation and independent native transaction-owner waits.
 * RU: Точная ACK/NACK correlation, atomic TX activation и независимые native transaction-owner waits.
 */
#include "../internal.h"


/* EN/RU: See operation.h; no old/foreign/mismatched confirmation can alter candidate state or release holds.
 * См. operation.h; old/foreign/mismatched confirmation не меняет candidate state и не снимает holds.
 */
int styxscopetxconfirm(StyxScope *parent, unsigned long long operation, int accepted)
{
    if (!parent || parent->leaf || !operation || (accepted != 0 && accepted != 1))
    {
        return 0;
    }
    ScopeHub *hub = parent->hub; /* Pinned native transaction owner. */
    pthread_mutex_lock(&hub->lock);
    if (hub->reason)
    {
        pthread_mutex_unlock(&hub->lock);
        return 0;
    }
    if (parent->tx_done == operation)
    {
        int repeat = parent->tx_accepted == accepted; /* Exact completed ACK/NACK repeats are harmless. */
        pthread_mutex_unlock(&hub->lock);
        return repeat;
    }
    ScopeProposal *proposal = parent->proposal; /* Private immutable candidate while held. */
    if (!proposal || proposal->operation != operation)
    {
        pthread_mutex_unlock(&hub->lock);
        return 0;
    }
    int valid = !accepted || scopeassignlocked(parent, STYX_SCOPE_TX, proposal->list, proposal->count, 1);
    if (valid)
    {
        parent->proposal = NULL;
        parent->tx_done = operation;
        parent->tx_accepted = accepted;
        for (size_t i = 0; i < proposal->count; i++)
        {
            proposal->list[i].scope->tx_pending = NULL;
            for (StyxScope *scope = proposal->list[i].scope; scope; scope = scope->parent)
            {
                scope->tx_holds--;
            }
        }
        pthread_cond_broadcast(&hub->changed);
    }
    pthread_mutex_unlock(&hub->lock);
    if (valid)
    {
        free(proposal);
    }
    else
    {
        scopeend(hub, STYX_CONTROL_PROTOCOL); /* Peer committed but local map could not; never resume uncertain traffic. */
    }
    return valid;
}


/* EN/RU: See operation.h; state is copied coherently and does not consume any incoming client/service records.
 * См. operation.h; coherent copied state не потребляет client/service records.
 */
int styxscopetxstate(StyxScope *parent, unsigned long long *operation)
{
    if (!parent || parent->leaf || !operation)
    {
        return STYX_SCOPE_OPERATION_TERMINAL;
    }
    ScopeHub *hub = parent->hub; /* Host pins parent through return. */
    pthread_mutex_lock(&hub->lock);
    *operation = parent->proposal ? parent->proposal->operation : parent->tx_done;
    int state = hub->reason ? STYX_SCOPE_OPERATION_TERMINAL : parent->proposal ? STYX_SCOPE_OPERATION_WAITING :
        !parent->tx_done ? STYX_SCOPE_OPERATION_IDLE : parent->tx_accepted ? STYX_SCOPE_OPERATION_ACCEPTED : STYX_SCOPE_OPERATION_REJECTED;
    pthread_mutex_unlock(&hub->lock);
    return state;
}


/* EN/RU: See operation.h; one native wait budget, timeout does not imply peer rejection or release TX holds.
 * См. operation.h; один native wait budget, timeout не означает peer rejection и не снимает TX holds.
 */
int styxscopetxwait(StyxScope *parent, unsigned long long operation, unsigned timeout)
{
    if (!parent || parent->leaf || !operation || timeout > 120000)
    {
        return 0;
    }
    struct timespec deadline; /* Single monotonic deadline. */
    if (clock_gettime(CLOCK_MONOTONIC, &deadline))
    {
        return 0;
    }
    deadline.tv_sec += timeout / 1000;
    deadline.tv_nsec += (long)(timeout % 1000) * 1000000L;
    if (deadline.tv_nsec >= 1000000000L)
    {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }
    ScopeHub *hub = parent->hub; /* Borrowed native owner through wait. */
    pthread_mutex_lock(&hub->lock);
    while (parent->proposal && parent->proposal->operation == operation && !hub->reason && timeout)
    {
        if (pthread_cond_timedwait(&hub->changed, &hub->lock, &deadline))
        {
            break;
        }
    }
    int resolved = !hub->reason && parent->tx_done == operation; /* Only an actual local resolution satisfies wait. */
    pthread_mutex_unlock(&hub->lock);
    return resolved;
}
