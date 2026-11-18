/* Copyright (c) 2026 bhgv
 * EN: Route retirement closes owned local channels even when native relays wait on full/empty queues.
 * RU: Route retirement закрывает local channels даже при ожидании native relays full/empty очередей.
 */
#define _POSIX_C_SOURCE 200809L
#include "internal.h"
#include "cancel.h"


/* EN/RU: Subscription owns state; private lock fences late callbacks against bridge destruction.
 * Subscription владеет state; private lock отделяет late callbacks от уничтожения bridge.
 */
struct UIRetirement
{
    pthread_mutex_t lock; /* Callback order: private lock, then bridge state lock. */
    StyxBidirUIBridge *bridge; /* Borrowed until detachment, never accessed after it. */
};


/* EN/RU: Latch REMOTE and close local queues, without recursively stopping route or entering a VM.
 * Зафиксировать REMOTE и закрыть local queues без recursive stop route и VM.
 */
static void retired(void *state)
{
    struct UIRetirement *notice = state; /* Independently pinned subscription context. */
    pthread_mutex_lock(&notice->lock);
    StyxBidirUIBridge *bridge = notice->bridge; /* Pinned by the private lifetime fence. */
    if (bridge)
    {
        pthread_mutex_lock(&bridge->lock);
        if (!bridge->reason)
        {
            bridge->reason = STYX_UI_BRIDGE_REMOTE;
        }
        pthread_mutex_unlock(&bridge->lock);
        dvm_ui_close(bridge->local);
    }
    pthread_mutex_unlock(&notice->lock);
}


/* EN/RU: Final private/subscription disposal after all admitted callbacks; no object access.
 * Final private/subscription disposal после callbacks, без доступа к объекту.
 */
static void release(void *state)
{
    struct UIRetirement *notice = state; /* Sole remaining context owner. */
    pthread_mutex_destroy(&notice->lock);
    free(notice);
}


/* EN/RU: See internal.h; local channels and state are initialized before sticky callback admission.
 * См. internal.h; local channels and state готовы до admission sticky callback.
 */
int uibridgeretirementattach(StyxBidirUIBridge *bridge)
{
    struct UIRetirement *notice = calloc(1, sizeof(*notice)); /* Private until successful subscription. */
    if (!notice)
    {
        return 0;
    }
    if (pthread_mutex_init(&notice->lock, NULL))
    {
        free(notice);
        return 0;
    }
    notice->bridge = bridge;
    struct DVM_CancelTap *tap = styxbidirroutewatch(bridge->route, retired, notice, release); /* Owns state only on success. */
    if (!tap)
    {
        release(notice);
        return 0;
    }
    bridge->retirement = notice;
    bridge->retired = tap;
    return 1;
}


/* EN/RU: See internal.h; detach access before nonwaiting unsubscribe, even with an admitted callback.
 * См. internal.h; отделить доступ до nonwaiting unsubscribe, включая admitted callback.
 */
void uibridgeretirementdetach(StyxBidirUIBridge *bridge)
{
    struct UIRetirement *notice = bridge->retirement; /* Host-only construction/final-join lifecycle. */
    if (notice)
    {
        pthread_mutex_lock(&notice->lock);
        notice->bridge = NULL;
        pthread_mutex_unlock(&notice->lock);
        bridge->retirement = NULL;
        dvm_cancel_unsubscribe(bridge->retired);
        bridge->retired = NULL;
    }
}
