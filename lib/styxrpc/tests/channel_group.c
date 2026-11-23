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
static int exchange(const StyxRpcChannelOutput *group, const StyxRpcChannelGrant *grants, size_t count,
    const StyxRpcSelection *operation, unsigned char *reply, StyxRpcChannelResult *result, int reverse)
{
    const StyxRpcResource *bus = &group->resource;
    unsigned char bytes[TEST_FRAME];
    size_t size, used;
    void *file = NULL;
    int ok = styxrpc_channel_request(grants, count, operation, NULL, 0, bytes, sizeof(bytes), &size) == STYXRPC_OK &&
        bus->open(bus->context, STYXRPC_READ | STYXRPC_WRITE, &file) &&
        bus->write(file, 0, bytes, size, &used) && used == size &&
        (!reverse || !bus->read(file, 0, reply, TEST_FRAME, &used)) &&
        bus->read(file, reverse ? size : 0, reply, TEST_FRAME, &used) && used &&
        styxrpc_channel_result(grants, count, operation, reply, used, result) == STYXRPC_OK;
    if (file)
    {
        bus->close(file);
    }
    return ok && result->selected == 1;
}


/* EN: Verify a detached seed and an echoed regular key both surface as the same rendezvous.
 * RU: Проверить, что копия начального сообщения и возвращённый обычный ключ дают один rendezvous. */
static int exercise(int reverse)
{
    const StyxRpcChannelGrant grants[] = {{1, "(c[i])"}, {2, "(i)"}};
    StyxRpcChannelRoute route = {.kind = STYXRPC_ROUTE_CURRENT, .channel = grants[1]};
    unsigned char bytes[TEST_FRAME], reply[TEST_FRAME];
    size_t size = 0;
    StyxRpcChannelQueue queues[] = {{.grant = grants[0], .bytes = bytes, .sizes = &size,
        .capacity = 1, .stride = sizeof(bytes), .count = 1}, {.grant = grants[1]}};
    StyxRpcChannelOutput output = {.key = 99};
    StyxRpcStatus (*build)(const StyxRpcChannelQueue *, size_t, size_t, StyxRpcChannelOutput *) =
        reverse ? styxrpc_channel_group_reverse_output : styxrpc_channel_group_output;
    route.channel.key = 3;
    if (!message(&route, bytes, &size) ||
        build(queues, 2, 0, &output) != STYXRPC_INVALID || output.key != 99)
    {
        return 0;
    }
    route.channel.key = 2;
    if (!message(&route, bytes, &size) ||
        build(queues, 2, 0, &output) != STYXRPC_OK)
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
        exchange(&output, grants, 2, &receive, reply, &result, reverse) &&
        styxrpc_channel_element_routes(grants[0].shape, result.value, result.size, &nested) == STYXRPC_OK &&
        nested.count == 1 && nested.entries[0].channel.key == 2 && nested.entries[0].unbuffered &&
        message(&route, bytes, &size) && size == sent_size &&
        exchange(&output, grants, 2, &send, reply, &result, reverse) &&
        exchange(&output, grants, 2, &receive, reply, &result, reverse) &&
        styxrpc_channel_element_routes(grants[0].shape, result.value, result.size, &nested) == STYXRPC_OK &&
        nested.count == 1 && nested.entries[0].channel.key == 2 && nested.entries[0].unbuffered;
    output.release(output.resource.context);
    return valid;
}


/* EN: Borrow a wrapped FIFO, consume its seed, and return the actual written bytes.
 * RU: Заимствовать кольцевой FIFO, прочитать начальное значение и вернуть записанные байты. */
static int exercise_borrow(void)
{
    unsigned char arena[2 * TEST_FRAME], reply[TEST_FRAME], value[TEST_FRAME];
    size_t sizes[2] = {0, 0}, length;
    const StyxRpcChannelGrant grant = {1, "(i)"};
    StyxRpcChannelQueue queue = {.grant = grant, .bytes = arena, .sizes = sizes,
        .capacity = 2, .stride = TEST_FRAME, .head = 1, .count = 1};
    StyxRpcChannelOutput output = {.key = 99};
    if (styxrpc_channel_group_reverse_borrow(&queue, &output) != STYXRPC_INVALID ||
        output.key != 99 || queue.head != 1 || queue.count != 1 ||
        styxrpc_value_integer(STYXRPC_VALUE_INT, 81, arena + TEST_FRAME, TEST_FRAME, sizes + 1) != STYXRPC_OK ||
        styxrpc_value_integer(STYXRPC_VALUE_INT, 43, value, sizeof(value), &length) != STYXRPC_OK ||
        styxrpc_channel_group_reverse_borrow(&queue, &output) != STYXRPC_OK)
    {
        return 0;
    }
    const StyxRpcSelection send = {.count = 1, .guards = {{1, 0, value, length}}};
    const StyxRpcSelection receive = {.count = 1, .guards = {{1, 1, NULL, 0}}};
    StyxRpcChannelResult result;
    int valid = output.key == 1 && !output.unbuffered &&
        exchange(&output, &grant, 1, &send, reply, &result, 1) &&
        exchange(&output, &grant, 1, &receive, reply, &result, 1) && result.integer == 81;
    output.release(output.resource.context);
    int64_t received;
    return valid && queue.head == 0 && queue.count == 1 &&
        styxrpc_value_get_integer(STYXRPC_VALUE_INT, arena, sizes[0], &received) == STYXRPC_OK && received == 43;
}


/* EN: Match two fids on a borrowed zero-slot queue without allocating FIFO storage.
 * RU: Сопоставить два fid в заимствованной очереди без слотов и буфера FIFO. */
static int exercise_rendezvous(void)
{
    const StyxRpcChannelGrant grant = {1, "(i)"};
    StyxRpcChannelQueue queue = {.grant = grant};
    StyxRpcChannelOutput output;
    unsigned char request[TEST_FRAME], reply[TEST_FRAME], value[TEST_FRAME];
    size_t integer, length, used;
    if (styxrpc_value_integer(STYXRPC_VALUE_INT, 12, value, sizeof(value), &integer) != STYXRPC_OK ||
        styxrpc_channel_group_reverse_borrow(&queue, &output) != STYXRPC_OK)
    {
        return 0;
    }
    const StyxRpcSelection receive = {.count = 1, .guards = {{1, 1, NULL, 0}}};
    const StyxRpcSelection send = {.count = 1, .guards = {{1, 0, value, integer}}};
    const StyxRpcResource *bus = &output.resource;
    void *receiver = NULL;
    StyxRpcChannelResult result;
    int valid = output.unbuffered &&
        styxrpc_channel_request(&grant, 1, &receive, NULL, 0, request, sizeof(request), &length) == STYXRPC_OK &&
        bus->open(bus->context, STYXRPC_READ | STYXRPC_WRITE, &receiver) &&
        bus->write(receiver, 0, request, length, &used) && used == length &&
        exchange(&output, &grant, 1, &send, reply, &result, 1) &&
        bus->read(receiver, length, reply, sizeof(reply), &used) &&
        styxrpc_channel_result(&grant, 1, &receive, reply, used, &result) == STYXRPC_OK &&
        result.selected == 1 && result.integer == 12;
    if (receiver)
    {
        bus->close(receiver);
    }
    output.release(bus->context);
    return valid && !queue.count && !queue.head;
}


/* EN: Run the group resource checks without FlashVM or DisVM.
 * RU: Проверить ресурс группы без FlashVM и DisVM. */
int main(void)
{
    if (!exercise(0) || !exercise(1) || !exercise_borrow() || !exercise_rendezvous())
    {
        fputs("channel group ownership or canonical echo failed\n", stderr);
        return 1;
    }
    return 0;
}
