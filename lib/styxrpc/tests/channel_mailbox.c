/* Copyright (c) 2026 bhgv
 * EN: Verify directional isolation and bounded backpressure without either VM.
 * RU: Проверка разделения направлений и ограничения очередей без обеих VM.
 */
#include "styxrpc/channel_mailbox.h"
#include <stdio.h>
#include <string.h>

#define FRAME 128U /* Extent of all messages and replies in this test. */

static const StyxRpcChannelGrant grant = {1, "(c[i])"}; /* Native key and sole mailbox element type. */


/* EN: Allow the expected address kind in the synthetic peer's codec.
 * RU: Допустить ожидаемый тип адреса в кодеке тестового партнёра. */
static int allow(const void *context, const StyxRpcChannelRoute *route)
{
    const StyxRpcChannelRouteKind *kind = context;
    return route->kind == *kind;
}


/* EN: Stage one complete operation on a fresh virtual fid owned by the caller.
 * RU: Записать полную операцию в новый виртуальный fid вызывающего. */
static int start(const StyxRpcResource *bus, const StyxRpcSelection *operation, void **fid, size_t *origin)
{
    unsigned char request[FRAME];
    size_t sent;
    return styxrpc_channel_request(&grant, 1, operation, NULL, 0, request, sizeof(request), origin) == STYXRPC_OK &&
        bus->open(bus->context, STYXRPC_READ | STYXRPC_WRITE, fid) &&
        bus->write(*fid, 0, request, *origin, &sent) && sent == *origin;
}


/* EN: Stage ordinary peer writes and check that only the owner can drain them.
 * RU: Записать сообщения партнёра и проверить, что их извлекает только владелец. */
static int exercise(StyxRpcChannelMailbox *mailbox, const StyxRpcResource *bus, int result)
{
    StyxRpcChannelRoute route = {.kind = result ? STYXRPC_ROUTE_PID : STYXRPC_ROUTE_MODULE, .channel = {23, "(i)"}};
    const StyxRpcChannelPolicy policy = {allow, &route.kind};
    unsigned char child[FRAME], value[FRAME], copy[FRAME], reply[FRAME];
    size_t child_size, length, extents[3], size;
    void *fids[3] = {NULL, NULL, NULL};
    if (result)
    {
        route.pid = UINT32_C(0x80000001);
    }
    else
    {
        route.token[0] = 1;
    }
    if (styxrpc_channel_route_value(&route, child, sizeof(child), &child_size) != STYXRPC_OK ||
        fvm_srecord_write(STYXRPC_CHANNEL_ELEMENT_TAG, child, child_size, value, sizeof(value), &length) != FVM_S_OK)
    {
        return 0;
    }
    const StyxRpcSelection send = {.count = 1, .guards = {{1, 0, value, length}}, .policy = &policy};
    const StyxRpcSelection receive = {.count = 1, .guards = {{1, 1, NULL, 0}}, .policy = &policy};
    StyxRpcChannelResult decoded;
    int ok = styxrpc_channel_mailbox_receive(mailbox, copy, sizeof(copy), &size) == STYXRPC_MORE &&
        start(bus, &send, fids, extents) && start(bus, &send, fids + 1, extents + 1) &&
        bus->read(fids[1], result ? 0 : extents[1], reply, sizeof(reply), &size) && !size &&
        start(bus, &receive, fids + 2, extents + 2) &&
        bus->read(fids[2], result ? 0 : extents[2], reply, sizeof(reply), &size) && !size &&
        styxrpc_channel_mailbox_send(mailbox, value, length) == STYXRPC_OK &&
        bus->read(fids[2], result ? 0 : extents[2], reply, sizeof(reply), &size) &&
        styxrpc_channel_result(&grant, 1, &receive, reply, size, &decoded) == STYXRPC_OK &&
        decoded.size == length && !memcmp(decoded.value, value, length) &&
        styxrpc_channel_mailbox_send(mailbox, value, length) == STYXRPC_OK &&
        styxrpc_channel_mailbox_send(mailbox, value, length) == STYXRPC_MORE &&
        styxrpc_channel_mailbox_receive(mailbox, copy, 1, &size) == STYXRPC_INVALID &&
        styxrpc_channel_mailbox_receive(mailbox, copy, sizeof(copy), &size) == STYXRPC_OK &&
        size == length && !memcmp(copy, value, length) &&
        bus->read(fids[1], result ? 0 : extents[1], reply, sizeof(reply), &size) &&
        styxrpc_channel_result(&grant, 1, &send, reply, size, &decoded) == STYXRPC_OK &&
        decoded.selected == 1 &&
        styxrpc_channel_mailbox_receive(mailbox, copy, sizeof(copy), &size) == STYXRPC_OK &&
        styxrpc_channel_mailbox_receive(mailbox, copy, sizeof(copy), &size) == STYXRPC_MORE;
    for (size_t i = 0; i < sizeof(fids) / sizeof(fids[0]); ++i)
    {
        if (fids[i])
        {
            bus->close(fids[i]);
        }
    }
    return ok;
}


/* EN: Verify that an independent owner handle survives result retirement as closed.
 * RU: Проверить, что отдельная ссылка владельца переживает отзыв в закрытом состоянии. */
static int module_result(void)
{
    StyxRpcChannelOutput output;
    StyxRpcChannelMailbox *mailbox;
    unsigned char byte;
    size_t size;
    if (styxrpc_channel_mailbox_result_create(grant.shape, 1, &mailbox, &output) != STYXRPC_OK)
    {
        return 0;
    }
    int ok = styxrpc_channel_mailbox_active(mailbox) && exercise(mailbox, &output.resource, 1);
    output.release(output.resource.context);
    ok = ok && !styxrpc_channel_mailbox_active(mailbox) &&
        styxrpc_channel_mailbox_receive(mailbox, &byte, 1, &size) == STYXRPC_INVALID;
    styxrpc_channel_mailbox_owner_release(mailbox);
    return ok;
}


/* EN: Exercise the independent C mailbox and release its resource after clunk.
 * RU: Проверить отдельную очередь C и освободить ресурс после clunk. */
int main(void)
{
    StyxRpcChannelOutput output;
    StyxRpcChannelMailbox *mailbox;
    if (styxrpc_channel_mailbox_create(grant.shape, 1, &mailbox, &output) != STYXRPC_OK)
    {
        return 1;
    }
    int ok = exercise(mailbox, &output.resource, 0);
    output.release(output.resource.context);
    ok = ok && module_result();
    if (!ok)
    {
        fputs("mailbox direction or backpressure failed\n", stderr);
    }
    return ok ? 0 : 1;
}
