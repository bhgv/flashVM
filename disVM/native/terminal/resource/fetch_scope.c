/* Copyright (c) 2026 bhgv
 * EN: Independent per-fetch cancellation sources forward lifetime/outer cancellation with native-only pins.
 * RU: Отдельные per-fetch sources переносят lifetime/outer cancellation только через native pins.
 */
#include "fetch_scope.h"
#include <string.h>


/* EN/RU: Request only the pinned child source; no receiver, provider or guest pointer is retained.
 * Отменить только удержанный child source, без receiver/provider/guest pointers.
 */
static void forward(void *source)
{
    dvm_cancel_request(source);
}


/* EN/RU: Release independent child pin after all admitted callbacks retire, even after fetch return.
 * Освободить child pin после callbacks, включая после возврата fetch.
 */
static void release(void *source)
{
    dvm_cancel_release(source);
}


/* EN/RU: Adopt one retained child pin on success; NULL leaves both sources caller-owned and unchanged.
 * Принять один child pin при успехе; NULL оставляет оба sources caller без изменений.
 */
static DVM_CancelTap *subscribe(DVM_Cancel *parent, DVM_Cancel *child)
{
    if (!dvm_cancel_retain(child))
    {
        return NULL;
    }
    DVM_CancelTap *tap = dvm_cancel_subscribe(parent, forward, child, release); /* Adopts only the new child pin. */
    if (!tap)
    {
        dvm_cancel_release(child);
    }
    return tap;
}


/* EN/RU: See fetch_scope.h; source subscriptions precede scope/RPC/sink publication, including sticky replay.
 * См. fetch_scope.h; subscriptions до scope/RPC/sink publication, включая sticky replay.
 */
int movie_fetch_scope_enter(MovieFetchScope *scope, DVM_Cancel *lifetime)
{
    DVM_Cancel *outer = dvm_cancel_current(); /* Borrowed through the existing caller's native scope. */
    memset(scope, 0, sizeof(*scope));
    scope->cancel = dvm_cancel_create();
    if (!scope->cancel)
    {
        return 0;
    }
    scope->lifetime = subscribe(lifetime, scope->cancel);
    if (outer && outer != lifetime)
    {
        scope->caller = subscribe(outer, scope->cancel);
    }
    if (!scope->lifetime || (outer && outer != lifetime && !scope->caller) ||
        !dvm_cancel_enter(&scope->current, scope->cancel))
    {
        movie_fetch_scope_leave(scope);
        return 0;
    }
    return 1;
}


/* EN/RU: See fetch_scope.h; callbacks reference no stack fields, no joined wait or parent cancellation.
 * См. fetch_scope.h; callbacks не используют stack fields, без join/отмены parents.
 */
void movie_fetch_scope_leave(MovieFetchScope *scope)
{
    if (scope->current.cancel)
    {
        dvm_cancel_leave(&scope->current);
    }
    dvm_cancel_unsubscribe(scope->caller);
    dvm_cancel_unsubscribe(scope->lifetime);
    dvm_cancel_release(scope->cancel);
    memset(scope, 0, sizeof(*scope));
}
