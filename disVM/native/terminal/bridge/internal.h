/* Copyright (c) 2026 bhgv
 * EN: Native relay state with copied pending frames, independent of either VM heap.
 * RU: Native-состояние моста с копиями pending-кадров, без указателей в кучи VM.
 */
#ifndef STYX_BIDIR_UI_BRIDGE_INTERNAL_H
#define STYX_BIDIR_UI_BRIDGE_INTERNAL_H
#include "bridge.h"
#include "ordered.h"
#include <pthread.h>
#include <stdlib.h>


/* EN/RU: State survives all borrowed users and both joined native relays.
 * Состояние живёт до завершения пользователей и обоих native-потоков.
 */
struct StyxBidirUIBridge
{
    StyxOrdered *ordered; /* Owned negotiated broker or NULL for a flat route. */
    StyxOrderedClient *ordered_client; /* Borrowed sole UI inbox capability within ordered owner. */
    StyxBidirRoute *route; /* Owned, including its sole UI mailbox consumer role. */
    DVM_UIBridge *local; /* Owned real FlashVM channel pair; closed before relay join. */
    struct UIRetirement *retirement; /* Subscription-owned callback lifetime fence. */
    struct DVM_CancelTap *retired; /* Owned sticky route retirement subscription. */
    unsigned side; /* Relay role, opposite the local endpoint's role. */
    unsigned long tx_tag; /* Immutable independently established outgoing UI binding. */
    pthread_mutex_t lock; /* Protects first reason and diagnostic pending copies. */
    pthread_t workers[2]; /* Native RX and TX relays; no guest or graphics callbacks. */
    unsigned started; /* Host-lifecycle count of successfully started, unjoined workers. */
    int reason; /* First stop/failure reason, under lock. */
    DVM_UIMessage messages[2]; /* Copied pending RX[0] and TX[1] values, under lock. */
    int pending[2]; /* Whether each copied value awaits destination acceptance. */
};


/* EN/RU: Latch reason under lock, then close/wake owned queues without holding that lock.
 * Зафиксировать reason под lock, затем закрыть/разбудить свои очереди без этого lock.
 */
void uibridgeend(StyxBidirUIBridge *bridge, int reason);


/* EN/RU: Native receive relay, borrowing bridge until host join; no VM access.
 * Native-передача RX, bridge заимствован до join, доступа к VM нет.
 */
void *uibridgereceive(void *bridge);


/* EN/RU: Native transmit relay, borrowing bridge until host join; no VM access.
 * Native-передача TX, bridge заимствован до join, доступа к VM нет.
 */
void *uibridgetransmit(void *bridge);

/* EN/RU: Attach after local channels/setup, 1/0; constructor only, callback may run before return.
 * Подписаться после local channels/setup, 1/0; только constructor, callback возможен до return.
 */
int uibridgeretirementattach(StyxBidirUIBridge *bridge);


/* EN/RU: Detach bridge access before unsubscribe/disposal; only rollback or joined host lifecycle.
 * Отделить bridge до unsubscribe/disposal; только rollback либо joined host lifecycle.
 */
void uibridgeretirementdetach(StyxBidirUIBridge *bridge);


/* EN/RU: Consume exactly one transport backend, create local channel pair and native relays; owned result/NULL.
 * Поглотить один transport backend, создать локальные каналы и native relays; свой результат/NULL.
 */
StyxBidirUIBridge *uibridgecreate(StyxBidirRoute *route, StyxOrdered *ordered, unsigned local_side, unsigned long tag);


/* EN/RU: Backend-specific native stop/free/read/wait/send, no guest/graphics owner access.
 * Native stop/free/read/wait/send выбранного backend, без доступа к гостю/graphics owner.
 */
void uibridgetransportstop(StyxBidirUIBridge *bridge);


/* EN/RU: Consume owned backend after native relays and external users retire.
 * Поглотить backend после завершения native relays и внешних users.
 */
void uibridgetransportfree(StyxBidirUIBridge *bridge);


/* EN/RU: Copy one frame or RECORD/EMPTY/END; malformed frames return END, never enter local channels.
 * Скопировать один frame: RECORD/EMPTY/END; ошибочный frame не попадает в локальные каналы.
 */
int uibridgetransportreceive(StyxBidirUIBridge *bridge, DVM_UIMessage *message);


/* EN/RU: Native wait only this UI inbox, borrowed bridge remains pinned until return.
 * Native-ожидание только UI-очереди; bridge удерживается до возврата.
 */
void uibridgetransportwait(StyxBidirUIBridge *bridge);


/* EN/RU: One copied frame admission;1 accepted/0 native timeout/backpressure/-1 terminal or permanent refusal.
 * Приём одной копии frame:1 принят/0 native timeout/backpressure/-1 завершение либо постоянный отказ.
 */
int uibridgetransportsend(StyxBidirUIBridge *bridge, const DVM_UIMessage *message);


/* EN/RU: Copy backend terminal state, no I/O; only legacy route uses separate capacity-flush wait.
 * Скопировать завершение backend, без I/O; лишь прежний route требует отдельного ожидания capacity.
 */
int uibridgetransportstate(StyxBidirUIBridge *bridge);
#endif
