/* Copyright (c) 2026 bhgv
 * EN: Borrowed endpoint access and mutex-protected diagnostic snapshots.
 * RU: Заимствование endpoint и диагностические копии под mutex.
 */
#include "internal.h"


/* EN/RU: See bridge.h; local queue storage survives until free after all endpoint users retire.
 * См. bridge.h; локальные очереди живут до free после завершения пользователей endpoint.
 */
DVM_UIBridge *styxbidiruibridgelocal(StyxBidirUIBridge *bridge)
{
    return bridge ? bridge->local : NULL;
}


/* EN/RU: See bridge.h; ownership remains with the adapter, including after explicit join.
 * См. bridge.h; route остаётся у моста, в том числе после явного join.
 */
StyxBidirRoute *styxbidiruibridgeroute(StyxBidirUIBridge *bridge)
{
    return bridge ? bridge->route : NULL;
}


/* EN/RU: See bridge.h; no observation grants ownership or causes queue/VM operations.
 * См. bridge.h; наблюдение не передаёт владения и не выполняет операции очереди/VM.
 */
int styxbidiruibridgestate(StyxBidirUIBridge *bridge)
{
    if (!bridge)
    {
        return STYX_UI_BRIDGE_LOCAL;
    }
    pthread_mutex_lock(&bridge->lock);
    int reason = bridge->reason; /* First observed relay stop/failure. */
    pthread_mutex_unlock(&bridge->lock);
    return reason;
}


/* EN/RU: See bridge.h; diagnostic copy does not consume or requeue a message.
 * См. bridge.h; диагностическая копия не удаляет и не возвращает сообщение в очередь.
 */
int styxbidiruibridgepending(StyxBidirUIBridge *bridge, int outgoing, DVM_UIMessage *output)
{
    if (!bridge || !output || (outgoing != 0 && outgoing != 1))
    {
        return 0;
    }
    pthread_mutex_lock(&bridge->lock);
    int pending = bridge->pending[outgoing]; /* Protected ownership snapshot. */
    if (pending)
    {
        *output = bridge->messages[outgoing];
    }
    pthread_mutex_unlock(&bridge->lock);
    return pending;
}
