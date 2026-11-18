/* Copyright (c) 2026 bhgv
 * EN: Own native relay lifetime and local channels without borrowing a VM or graphics owner.
 * RU: Владение native-потоками и локальными каналами без заимствования VM или graphics owner.
 */
#include "internal.h"


/* EN/RU: See internal.h; never close a channel or route under the relay state lock.
 * См. internal.h; закрытие каналов и route выполняется без lock состояния моста.
 */
void uibridgeend(StyxBidirUIBridge *bridge, int reason)
{
    pthread_mutex_lock(&bridge->lock);
    if (!bridge->reason)
    {
        bridge->reason = reason;
    }
    pthread_mutex_unlock(&bridge->lock);
    dvm_ui_close(bridge->local);
    uibridgetransportstop(bridge);
}


/* EN/RU: See bridge.h; wake native waiters while retaining all diagnostic storage.
 * См. bridge.h; разбудить native-ожидания с сохранением диагностического состояния.
 */
void styxbidiruibridgestop(StyxBidirUIBridge *bridge)
{
    if (bridge)
    {
        uibridgeend(bridge, STYX_UI_BRIDGE_STOPPED);
    }
}


/* EN/RU: See bridge.h; this lifecycle-only counter is never accessed by relay workers.
 * См. bridge.h; lifecycle-счётчик изменяет только host, native-потоки его не читают.
 */
void styxbidiruibridgejoin(StyxBidirUIBridge *bridge)
{
    if (bridge)
    {
        styxbidiruibridgestop(bridge);
        for (unsigned i = 0; i < bridge->started; i++)
        {
            pthread_join(bridge->workers[i], NULL);
        }
        bridge->started = 0;
    }
}


/* EN/RU: See bridge.h; external endpoint users must retire before owned queue destruction.
 * См. bridge.h; внешние пользователи endpoint завершаются до удаления своих очередей.
 */
void styxbidiruibridgefree(StyxBidirUIBridge *bridge)
{
    if (bridge)
    {
        styxbidiruibridgejoin(bridge);
        uibridgeretirementdetach(bridge);
        uibridgetransportfree(bridge);
        dvm_ui_destroy(bridge->local);
        pthread_mutex_destroy(&bridge->lock);
        free(bridge);
    }
}


/* EN/RU: See bridge.h; complete queue setup before either relay is published.
 * См. bridge.h; настройка очередей завершается до запуска обоих native-потоков.
 */
StyxBidirUIBridge *uibridgecreate(StyxBidirRoute *route, StyxOrdered *ordered, unsigned local_side, unsigned long tx_tag)
{
    StyxOrderedClient *client = ordered ? styxorderedclient(ordered, tx_tag) : NULL; /* Predeclared UI capability. */
    if ((!!route == !!ordered) || local_side > DVM_UI_DIS || tx_tag > 0xffffffffUL ||
        (route && (!tx_tag || styxbidirroutelimit(route) < DVM_UI_HEADER_SIZE + DVM_UI_TEXT_MAX + 32)) ||
        (ordered && (!client || styxorderedclientceiling(client) < DVM_UI_HEADER_SIZE + DVM_UI_TEXT_MAX)))
    {
        styxbidirroutefree(route);
        styxorderedfree(ordered);
        return NULL;
    }
    StyxBidirUIBridge *bridge = calloc(1, sizeof(*bridge)); /* Unpublished native owner. */
    if (!bridge)
    {
        styxbidirroutefree(route);
        styxorderedfree(ordered);
        return NULL;
    }
    if (pthread_mutex_init(&bridge->lock, NULL))
    {
        styxbidirroutefree(route);
        styxorderedfree(ordered);
        free(bridge);
        return NULL;
    }
    bridge->route = route;
    bridge->ordered = ordered;
    bridge->ordered_client = client;
    bridge->side = 1 - local_side;
    bridge->tx_tag = tx_tag;
    if (dvm_ui_create(0, &bridge->local) != FVM_OK)
    {
        styxbidiruibridgefree(bridge);
        return NULL;
    }
    if (!uibridgeretirementattach(bridge))
    {
        styxbidiruibridgefree(bridge);
        return NULL;
    }
    if (pthread_create(&bridge->workers[0], NULL, uibridgereceive, bridge))
    {
        styxbidiruibridgefree(bridge);
        return NULL;
    }
    bridge->started = 1;
    if (pthread_create(&bridge->workers[1], NULL, uibridgetransmit, bridge))
    {
        styxbidiruibridgefree(bridge);
        return NULL;
    }
    bridge->started = 2;
    return bridge;
}


/* EN/RU: See bridge.h; retain legacy flat-route construction and ownership unchanged.
 * См. bridge.h; сохранить создание и владение прежним flat-route без изменения контракта.
 */
StyxBidirUIBridge *styxbidiruibridge(StyxBidirRoute *route, unsigned local_side, unsigned long tx_tag)
{
    return uibridgecreate(route, NULL, local_side, tx_tag);
}
