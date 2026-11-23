/* Copyright (c) 2026 bhgv
 * EN: Check standalone result group ownership and native capacity markers without a VM.
 * RU: Проверка владения группой и признаков ёмкости без участия VM.
 */
#include "styxrpc/channel_group.h"
#include "styxrpc/channel_route.h"
#include <stdio.h>
#include <string.h>

#define TEST_FRAME 256U /* Space for one small group operation and reply. */


/* EN: Frame one child route as an element of chan of chan of int.
 * RU: Записать маршрут дочернего канала как элемент chan of chan of int. */
static int message(const StyxRpcChannelRoute *route, unsigned char *bytes, size_t *size)
{
    unsigned char leaf[TEST_FRAME];
    size_t length;
    return styxrpc_channel_route_value(route, leaf, sizeof(leaf), &length) == STYXRPC_OK &&
        fvm_srecord_write(STYXRPC_CHANNEL_ELEMENT_TAG, leaf, length, bytes, TEST_FRAME, size) == FVM_S_OK;
}


/* EN: Drive one C resource fid with a checked request and decode its reply before release.
 * RU: Выполнить проверенный запрос на fid ресурса C и разобрать ответ до закрытия. */
static int exchange(const StyxRpcChannelOutput *group, const StyxRpcChannelGrant *grants,
    const StyxRpcSelection *operation, unsigned char *reply, StyxRpcChannelResult *result)
{
    const StyxRpcResource *bus = &group->resource;
    unsigned char bytes[TEST_FRAME];
    size_t size, used;
    void *file = NULL;
    int ok = styxrpc_channel_request(grants, 2, operation, NULL, 0, bytes, sizeof(bytes), &size) == STYXRPC_OK &&
        bus->open(bus->context, STYXRPC_READ | STYXRPC_WRITE, &file) &&
        bus->write(file, 0, bytes, size, &used) && used == size &&
        bus->read(file, 0, reply, TEST_FRAME, &used) && used &&
        styxrpc_channel_result(grants, 2, operation, reply, used, result) == STYXRPC_OK;
    if (file)
    {
        bus->close(file);
    }
    return ok && result->selected == 1;
}


/* EN: Verify a detached seed and an echoed regular key both surface as the same rendezvous.
 * RU: Проверить, что копия начального сообщения и возвращённый обычный ключ дают один rendezvous. */
static int exercise(void)
{
    const StyxRpcChannelGrant grants[] = {{1, "(c[i])"}, {2, "(i)"}};
    StyxRpcChannelRoute route = {.kind = STYXRPC_ROUTE_CURRENT, .channel = grants[1]};
    unsigned char bytes[TEST_FRAME], reply[TEST_FRAME];
    size_t size = 0;
    StyxRpcChannelQueue queues[] = {{.grant = grants[0], .bytes = bytes, .sizes = &size,
        .capacity = 1, .stride = sizeof(bytes), .count = 1}, {.grant = grants[1]}};
    StyxRpcChannelOutput output = {.key = 99};
    route.channel.key = 3;
    if (!message(&route, bytes, &size) ||
        styxrpc_channel_group_output(queues, 2, 0, &output) != STYXRPC_INVALID || output.key != 99)
    {
        return 0;
    }
    route.channel.key = 2;
    if (!message(&route, bytes, &size) ||
        styxrpc_channel_group_output(queues, 2, 0, &output) != STYXRPC_OK)
    {
        return 0;
    }
    size_t sent_size = size;
    memset(bytes, 0, sizeof(bytes));
    const StyxRpcSelection receive = {.count = 1, .guards = {{1, 1, NULL, 0}}};
    const StyxRpcSelection send = {.count = 1, .guards = {{1, 0, bytes, sent_size}}};
    StyxRpcChannelResult result;
    StyxRpcChannelRoutes nested;
    int valid = output.key == 1 && !output.unbuffered &&
        exchange(&output, grants, &receive, reply, &result) &&
        styxrpc_channel_element_routes(grants[0].shape, result.value, result.size, &nested) == STYXRPC_OK &&
        nested.count == 1 && nested.entries[0].channel.key == 2 && nested.entries[0].unbuffered &&
        message(&route, bytes, &size) && size == sent_size &&
        exchange(&output, grants, &send, reply, &result) && exchange(&output, grants, &receive, reply, &result) &&
        styxrpc_channel_element_routes(grants[0].shape, result.value, result.size, &nested) == STYXRPC_OK &&
        nested.count == 1 && nested.entries[0].channel.key == 2 && nested.entries[0].unbuffered;
    output.release(output.resource.context);
    return valid;
}


/* EN: Run the group resource checks without FlashVM or DisVM.
 * RU: Проверить ресурс группы без FlashVM и DisVM. */
int main(void)
{
    if (!exercise())
    {
        fputs("channel group ownership or canonical echo failed\n", stderr);
        return 1;
    }
    return 0;
}
