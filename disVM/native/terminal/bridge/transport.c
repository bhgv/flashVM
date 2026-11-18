/* Copyright (c) 2026 bhgv
 * EN: Existing frame/channel bridge selects flat or negotiated transport without changing guest-facing messages.
 * RU: Существующий frame/channel-мост выбирает flat либо согласованный transport без изменения гостевых сообщений.
 */
#include "internal.h"
#include "disvm/ui_frame.h"


/* EN/RU: See internal.h; stop only the owned backend, outside bridge state lock.
 * См. internal.h; остановить только свой backend вне mutex состояния моста.
 */
void uibridgetransportstop(StyxBidirUIBridge *bridge)
{
    if (bridge->ordered)
    {
        styxorderedstop(bridge->ordered);
    }
    else
    {
        styxbidirroutestop(bridge->route);
    }
}


/* EN/RU: See internal.h; exactly one of these mutually exclusive owners is nonnull.
 * См. internal.h; лишь один из взаимоисключающих owners ненулевой.
 */
void uibridgetransportfree(StyxBidirUIBridge *bridge)
{
    styxorderedfree(bridge->ordered);
    styxbidirroutefree(bridge->route);
}


/* EN/RU: See internal.h; native relay owns the selected UI consumer, ordinal stays transport metadata.
 * См. internal.h; native relay владеет UI-потребителем, номер остаётся transport-метаданными.
 */
int uibridgetransportreceive(StyxBidirUIBridge *bridge, DVM_UIMessage *message)
{
    if (!bridge->ordered)
    {
        return styxbidirrouteui(bridge->route, message);
    }
    unsigned char bytes[DVM_UI_HEADER_SIZE + DVM_UI_TEXT_MAX]; /* One complete canonical frame. */
    size_t size; /* Actual copied payload. */
    unsigned long long ordinal; /* Private per-tag order checkpoint. */
    int result = styxorderedclientnext(bridge->ordered_client, &ordinal, bytes, sizeof(bytes), &size); /* One packet. */
    if (result == STYX_ORDER_RECORD)
    {
        return dvm_ui_frame_decode(bytes, size, message) == FVM_OK ? STYX_CONTROL_RECORD : STYX_CONTROL_END;
    }
    return result == STYX_ORDER_EMPTY ? STYX_CONTROL_EMPTY : STYX_CONTROL_END;
}


/* EN/RU: See internal.h; broker and all unrelated client queues remain active during this native wait.
 * См. internal.h; брокер и остальные клиентские очереди продолжают работу во время native-ожидания.
 */
void uibridgetransportwait(StyxBidirUIBridge *bridge)
{
    if (bridge->ordered)
    {
        styxorderedclientwait(bridge->ordered_client, 120000);
    }
    else
    {
        styxbidirroutewait(bridge->route, 1, 120000);
    }
}


/* EN/RU: See internal.h; encode only ordinary value frames, no managed pointer or graphics callback crosses transport.
 * См. internal.h; кодировать только value-frames, без managed-указателей и graphics callbacks в transport.
 */
int uibridgetransportsend(StyxBidirUIBridge *bridge, const DVM_UIMessage *message)
{
    if (!bridge->ordered)
    {
        return styxbidirrouteuisend(bridge->route, bridge->tx_tag, message);
    }
    char bytes[DVM_UI_HEADER_SIZE + DVM_UI_TEXT_MAX]; /* Private canonical frame, retained during native TX wait. */
    size_t size; /* Exact frame length. */
    if (dvm_ui_frame_encode(message, bytes, sizeof(bytes), &size) != FVM_OK)
    {
        return -1;
    }
    int result = styxorderedclientsendwait(bridge->ordered_client, bytes, size, 120000); /* Only this client's predicate. */
    return result == STYX_ORDERED_SENT ? 1 : result == STYX_ORDERED_WAIT ? 0 : -1;
}


/* EN/RU: See internal.h; no I/O or guest action, copy sticky native backend reason.
 * См. internal.h; без I/O и гостевых действий, скопировать sticky-причину backend.
 */
int uibridgetransportstate(StyxBidirUIBridge *bridge)
{
    return bridge->ordered ? styxorderedstate(bridge->ordered) : styxbidirroutestate(bridge->route);
}
