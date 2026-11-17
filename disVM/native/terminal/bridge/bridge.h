/* Copyright (c) 2026 bhgv
 * EN: Native bidir relay between a terminal route and the existing local IDE UI channels.
 * RU: Native bidir-мост между маршрутом терминала и существующими каналами IDE.
 */
#ifndef STYX_BIDIR_UI_BRIDGE_H
#define STYX_BIDIR_UI_BRIDGE_H
#include "route/route.h"
#include "disvm/ui_bridge.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct StyxBidirUIBridge StyxBidirUIBridge; /* Owned route, local queues and two native relays. */
#define STYX_UI_BRIDGE_LIVE 0 /* Both native relays are admitted. */
#define STYX_UI_BRIDGE_STOPPED 1 /* Explicit host cancellation. */
#define STYX_UI_BRIDGE_REMOTE 2 /* Terminal route stopped or refused a valid frame. */
#define STYX_UI_BRIDGE_LOCAL 3 /* Local channels closed or refused a valid frame. */


/* EN: Consume owned route on EVERY outcome; create owned local channels and two native relays.
 * local_side is DVM_UI_DIS or DVM_UI_UI, the role retained by the local endpoint. Relays own
 * the opposite role and become the route's sole UI consumer. tx_tag is independently established
 * by the host; RX declarations never supply it. Return owned bridge/NULL; no guest/VM callbacks.
 * Record limit must hold every valid UI frame (HEADER_SIZE + TEXT_MAX + 32); a smaller limit
 * is rejected instead of retrying an unencodable frame forever. General records remain
 * available through the borrowed route accessor and must be serviced.
 * RU: Принять route при ЛЮБОМ исходе, создать свои локальные каналы и два native-потока.
 * local_side — роль локального endpoint; мост использует противоположную роль. tx_tag согласуется
 * отдельно от RX. Лимит записи должен вмещать HEADER_SIZE + TEXT_MAX + 32 байта. Вернуть свой
 * bridge/NULL. Общую очередь route по-прежнему обслуживает host.
 */
StyxBidirUIBridge *styxbidiruibridge(StyxBidirRoute *route, unsigned local_side, unsigned long tx_tag);


/* EN/RU: Borrow stable local queue pair until free; caller uses the constructor's local_side.
 * Заимствовать локальную пару очередей до free; роль caller задана local_side конструктора.
 */
DVM_UIBridge *styxbidiruibridgelocal(StyxBidirUIBridge *bridge);


/* EN/RU: Borrow the owned route for GENERAL records/state only, never consume its UI mailbox.
 * Заимствовать route только для ОБЩЕЙ очереди и состояния; UI-очередь читает только мост.
 */
StyxBidirRoute *styxbidiruibridgeroute(StyxBidirUIBridge *bridge);


/* EN/RU: Copy first relay terminal reason or LIVE; invalid gives LOCAL, without I/O or callbacks.
 * Скопировать первую terminal reason/LIVE; неверный аргумент даёт LOCAL, без I/O и callbacks.
 */
int styxbidiruibridgestate(StyxBidirUIBridge *bridge);


/* EN: Inspect one retained message without consuming it; outgoing=1 selects TX, 0 selects RX.
 * Return 1 copied/0 absent or invalid; output unchanged on 0. A pending snapshot may already
 * be accepted by its destination while a relay finishes publishing completion. For diagnosis
 * or recovery only AFTER explicit join while the bridge remains alive; never replay a live snapshot.
 * RU: Скопировать удержанное сообщение без удаления; outgoing — к удалённой стороне (0/1).
 * 1 — копия, 0 — нет/ошибка, output сохранён. Живой snapshot не разрешает повторную отправку:
 * получатель мог уже принять сообщение до фиксации завершения. Восстановление — только после join.
 */
int styxbidiruibridgepending(StyxBidirUIBridge *bridge, int outgoing, DVM_UIMessage *output);


/* EN: Latch stop, close only owned local channels and owned terminal route, wake both relays.
 * NULL harmless, no join; shared Styx client and its other fids remain owned by the host.
 * RU: Зафиксировать stop, закрыть только свои локальные каналы и route, разбудить оба потока.
 * NULL допустим, без join; общий Styx client и его остальные fids остаются у host.
 */
void styxbidiruibridgestop(StyxBidirUIBridge *bridge);


/* EN: Stop/join native relays while retaining channels, route and pending snapshots for inspection.
 * Host-only lifecycle operation, no concurrent join/free. NULL harmless; repeated join is safe.
 * RU: Stop/join потоков с сохранением каналов, route и pending-сообщений для проверки.
 * Lifecycle-операция host: нельзя одновременно join/free. NULL и повторный join допустимы.
 */
void styxbidiruibridgejoin(StyxBidirUIBridge *bridge);


/* EN/RU: After all external users retire, join and free all owned queues/route; NULL harmless.
 * После завершения внешних пользователей join и освобождение своих очередей/route; NULL допустим.
 */
void styxbidiruibridgefree(StyxBidirUIBridge *bridge);
#ifdef __cplusplus
}
#endif
#endif
