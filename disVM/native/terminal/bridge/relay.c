/* Copyright (c) 2026 bhgv
 * EN: Blocking waits belong only to native relays, never to guest or UI execution owners.
 * RU: Блокирующие ожидания принадлежат только native-потокам моста, а не guest/UI owner.
 */
#include "internal.h"


/* EN: Publish or clear a copied pending record under the state lock.
 * bridge is borrowed until join, direction selects RX[0]/TX[1], NULL message clears acceptance.
 * RU: Скопировать или снять pending под lock; direction выбирает RX[0]/TX[1], NULL — принято.
 */
static void pending(StyxBidirUIBridge *bridge, int direction, const DVM_UIMessage *message)
{
    pthread_mutex_lock(&bridge->lock);
    if (message)
    {
        bridge->messages[direction] = *message;
    }
    bridge->pending[direction] = message != NULL;
    pthread_mutex_unlock(&bridge->lock);
}


/* EN/RU: See internal.h; one copied route message is held until the real local channel accepts it.
 * См. internal.h; одна копия удерживается до принятия настоящим локальным каналом.
 */
void *uibridgereceive(void *opaque)
{
    StyxBidirUIBridge *bridge = opaque; /* Native owner retained through join. */
    while (!styxbidiruibridgestate(bridge))
    {
        DVM_UIMessage message; /* Private copy, never a pointer into either VM. */
        int status = uibridgetransportreceive(bridge, &message); /* One mailbox item. */
        if (status == STYX_CONTROL_EMPTY)
        {
            uibridgetransportwait(bridge);
            continue;
        }
        if (status != STYX_CONTROL_RECORD)
        {
            uibridgeend(bridge, STYX_UI_BRIDGE_REMOTE);
            return NULL;
        }
        pending(bridge, 0, &message);
        if (dvm_ui_send(bridge->local, bridge->side, &message, 1) != FVM_OK)
        {
            uibridgeend(bridge, STYX_UI_BRIDGE_LOCAL);
            return NULL;
        }
        pending(bridge, 0, NULL);
    }
    return NULL;
}


/* EN/RU: See internal.h; never consume another local frame before queue acceptance of this one.
 * См. internal.h; следующий локальный кадр не потребляется до принятия текущего TX-очередью.
 */
void *uibridgetransmit(void *opaque)
{
    StyxBidirUIBridge *bridge = opaque; /* Native owner retained through join. */
    while (!styxbidiruibridgestate(bridge))
    {
        DVM_UIMessage message; /* Private local-channel copy, retained through pressure/failure. */
        if (dvm_ui_receive(bridge->local, bridge->side, &message, 1) != FVM_OK)
        {
            uibridgeend(bridge, STYX_UI_BRIDGE_LOCAL);
            return NULL;
        }
        pending(bridge, 1, &message);
        int accepted; /* Detailed backend result; ordered send waits only this client's TX predicate. */
        while ((accepted = uibridgetransportsend(bridge, &message)) != 1)
        {
            if (styxbidiruibridgestate(bridge))
            {
                return NULL;
            }
            if (accepted < 0 || uibridgetransportstate(bridge))
            {
                uibridgeend(bridge, STYX_UI_BRIDGE_REMOTE);
                return NULL;
            }
            /* EN/RU: Native condition wait, not retry polling; limits guarantee the frame fits.
             * Ожидание native-condition, не опрос; лимиты гарантируют вместимость кадра.
             */
            if (bridge->route)
            {
                styxbidirrouteflush(bridge->route, 120000);
            }
        }
        pending(bridge, 1, NULL);
    }
    return NULL;
}
