/* Copyright (c) 2026 bhgv
 * EN: Native relay state with copied pending frames, independent of either VM heap.
 * RU: Native-состояние моста с копиями pending-кадров, без указателей в кучи VM.
 */
#ifndef STYX_BIDIR_UI_BRIDGE_INTERNAL_H
#define STYX_BIDIR_UI_BRIDGE_INTERNAL_H
#include "bridge.h"
#include <pthread.h>
#include <stdlib.h>


/* EN/RU: State survives all borrowed users and both joined native relays.
 * Состояние живёт до завершения пользователей и обоих native-потоков.
 */
struct StyxBidirUIBridge
{
    StyxBidirRoute *route; /* Owned, including its sole UI mailbox consumer role. */
    DVM_UIBridge *local; /* Owned real FlashVM channel pair; closed before relay join. */
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
#endif
