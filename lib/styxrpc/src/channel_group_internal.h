/* Copyright (c) 2026 bhgv
 * EN: Internal dispatch and canonicalization for one owned result group.
 * RU: Внутренняя диспетчеризация и канонизация группы результата.
 */
#ifndef STYXRPC_CHANNEL_GROUP_INTERNAL_H
#define STYXRPC_CHANNEL_GROUP_INTERNAL_H
#include "styxrpc/channel_group.h"

typedef struct ChannelGroup
{
    StyxRpcChannelScheduler scheduler;          /* Dispatcher-owned scheduler. */
    StyxRpcChannelQueue queues[STYXRPC_CHANNEL_SLOTS]; /* Owned or exclusively borrowed arenas. */
    StyxRpcChannelGrant grants[STYXRPC_CHANNEL_SLOTS]; /* Exact operation keys and shapes. */
    StyxRpcChannelPolicy policy;                /* Current-group validation for each fid. */
    int reverse;                               /* Use post-request read offsets for reverse PID routes. */
    size_t count;                               /* Number of stable queue entries. */
    StyxRpcChannelQueue *borrowed;               /* Original descriptor for returning FIFO cursors. */
} ChannelGroup;


/* EN: Build virtual bus callbacks that borrow group.
 * RU: Создать колбэки виртуальной шины, заимствующие group. */
StyxRpcResource styxrpc_group_resource(ChannelGroup *group);


/* EN: Confirm a nested route belongs to this group's exact keys, shape and capacity.
 * RU: Сверить маршрут с точными ключами, типом и ёмкостью этой группы. */
int styxrpc_group_allows(const void *context, const StyxRpcChannelRoute *route);


/* EN: Preserve the capacity bit on echo of a known rendezvous key in a typed message.
 * The full element must have passed group.policy. Bytes and output are disjoint.
 * RU: Сохранить признак rendezvous при возврате известного ключа в типизированном
 * сообщении. Весь элемент проверен group.policy. Буферы раздельны. */
int styxrpc_group_normalize(ChannelGroup *group, const void *bytes, size_t size,
    unsigned char *output, size_t capacity, size_t *used);

#endif
