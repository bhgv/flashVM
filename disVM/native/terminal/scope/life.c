/* Copyright (c) 2026 bhgv
 * EN: Explicit scoped broker construction/retirement; host owns root and all distinct client handles.
 * RU: Явное создание/завершение scoped broker, host владеет root и отдельными client handles.
 */
#include "internal.h"


/* EN/RU: See internal.h; cancellation never runs under tree mutex or executes a guest.
 * См. internal.h; cancellation вне tree mutex и не исполняет guest.
 */
void scopeend(ScopeHub *hub, int reason)
{
    pthread_mutex_lock(&hub->lock);
    if (!hub->reason)
    {
        hub->reason = reason ? reason : STYX_CONTROL_READ_FAILED;
    }
    pthread_cond_broadcast(&hub->changed);
    pthread_mutex_unlock(&hub->lock);
    styxbidircontrolstop(hub->control);
}


/* EN/RU: See scope.h; only native root owner may stop the whole broker.
 * См. scope.h; весь broker останавливает лишь native root owner.
 */
void styxscopestop(StyxScope *root)
{
    if (root && !root->parent)
    {
        scopeend(root->hub, STYX_CONTROL_STOPPED);
    }
}


/* EN/RU: See scope.h; coherent first outcome, applications have no parent/root status authority.
 * См. scope.h; coherent first outcome, applications без parent/root status authority.
 */
int styxscopestate(StyxScope *scope)
{
    if (!scope)
    {
        return STYX_CONTROL_READ_FAILED;
    }
    pthread_mutex_lock(&scope->hub->lock);
    int reason = scope->hub->reason; /* Native-only copied outcome. */
    pthread_mutex_unlock(&scope->hub->lock);
    return reason;
}


/* EN/RU: See scope.h; external callers retire before root consumption, no free from workers/callbacks.
 * См. scope.h; external callers завершаются до root consumption, не free из workers/callbacks.
 */
void styxscopefree(StyxScope *root)
{
    if (root && !root->parent)
    {
        ScopeHub *hub = root->hub; /* Sole native owner through joined disposal. */
        styxscopestop(root);
        if (hub->started)
        {
            pthread_join(hub->worker, NULL);
        }
        scoperetirementdetach(hub);
        styxbidircontrolfree(hub->control);
        scopeclear(root);
        free(hub->scratch);
        pthread_cond_destroy(&hub->changed);
        pthread_mutex_destroy(&hub->lock);
        free(hub);
    }
}


/* EN/RU: See scope.h; do not reinterpret legacy flat bindings or infer any peer envelope/tag agreement.
 * См. scope.h; не переинтерпретировать legacy flat bindings и не угадывать peer envelope/tag agreement.
 */
StyxScope *styxbidirscopes(StyxBidirControl *control, unsigned limit)
{
    if (!control || !limit || limit > 65536)
    {
        styxbidircontrolfree(control);
        return NULL;
    }
    ScopeHub *hub = calloc(1, sizeof(*hub)); /* Private until full synchronization setup. */
    StyxScope *root = calloc(1, sizeof(*root)); /* Native root, never an application handle. */
    if (!hub || !root || pthread_mutex_init(&hub->lock, NULL))
    {
        free(hub);
        free(root);
        styxbidircontrolfree(control);
        return NULL;
    }
    pthread_condattr_t attributes; /* Temporary monotonic condition attributes. */
    int initialized = pthread_condattr_init(&attributes) == 0; /* Attribute cleanup guard. */
    int condition = initialized && !pthread_condattr_setclock(&attributes, CLOCK_MONOTONIC) &&
        !pthread_cond_init(&hub->changed, &attributes);
    if (initialized)
    {
        pthread_condattr_destroy(&attributes);
    }
    if (!condition)
    {
        pthread_mutex_destroy(&hub->lock);
        free(hub);
        free(root);
        styxbidircontrolfree(control);
        return NULL;
    }
    hub->root = root;
    hub->limit = limit;
    hub->control = control;
    root->hub = hub;
    hub->scratch = malloc(limit);
    if (!hub->scratch || !scoperetirementattach(hub) || styxscopestate(root) || pthread_create(&hub->worker, NULL, scoperun, hub))
    {
        styxscopefree(root);
        return NULL;
    }
    hub->started = 1;
    return root;
}
