/* Copyright (c) 2026 bhgv
 * EN: Pass and forward a live Int32 reference on one connected UniBus session.
 * RU: Передать и переслать живую ссылку Int32 в одной подключённой сессии UniBus.
 */
#include "styxrpc/exchange.h"
#include "styxrpc/variable_ref.h"
#include "client.h"
#include "drivers/posix/fd.h"
#include "mux/bidir.h"
#include <pthread.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct Exchange
{
    StyxRpcIntRef issued;   /* Server-owned identity, fixed before the dispatcher starts. */
    StyxRpcIntRef received; /* Client's decoded identity, on the client thread only. */
    int32_t value;          /* Cell value, accessed only on the dispatcher. */
    int forwarded;          /* Valid forwarding observed on the dispatcher. */
} Exchange;


/* EN/RU: Read the live cell. / Прочитать живую ячейку. */
static int load(void *state, int32_t *out)
{
    *out = ((Exchange *)state)->value;
    return 1;
}


/* EN/RU: Update the live cell. / Записать живую ячейку. */
static int store(void *state, int32_t value)
{
    ((Exchange *)state)->value = value;
    return 1;
}


/* EN: Offer the reference only after the peer opens the exchange.
 * RU: Передать ссылку после открытия корня обмена другой стороной. */
static void offer(StyxRpcMessagePort *port, void *state)
{
    const Exchange *test = state;
    unsigned char bytes[STYXRPC_INT_REF_SIZE];
    if (styxrpc_int_ref_encode(&test->issued, bytes, sizeof(bytes)))
    {
        styxrpc_message_send(port, "test.variable-ref", 1, bytes, sizeof(bytes));
    }
}


/* EN: Decode a received descriptor on the client.
 * RU: Декодировать полученный дескриптор на клиенте. */
static int accept_ref(StyxRpcMessagePort *port, void *state, const void *bytes, size_t size)
{
    (void)port;
    return styxrpc_int_ref_decode(bytes, size, state);
}


/* EN: Confirm that the forwarded message still names the identical cell.
 * RU: Подтвердить, что пересланное сообщение обозначает ту же ячейку. */
static int forwarded(StyxRpcMessagePort *port, void *state, const void *bytes, size_t size)
{
    (void)port;
    Exchange *test = state;
    StyxRpcIntRef copy;
    test->forwarded = styxrpc_int_ref_decode(bytes, size, &copy) && copy.pid == test->issued.pid &&
        copy.writable == test->issued.writable;
    return test->forwarded;
}


/* EN/RU: Dispatch until the peer closes. / Обслуживать до отключения. */
static void *serve(void *server)
{
    while (styxrpc_server_step(server))
    {
    }
    return NULL;
}


/* EN: Request cancellation before the first wire operation.
 * RU: Запросить отмену до первой операции по протоколу. */
static int stopped(void *state)
{
    (void)state;
    return 1;
}


/* EN: Open both ends of a forwarded reference and check live read-after-write.
 * RU: Открыть ссылку и её пересланную копию, проверить чтение после записи. */
static int exchange(StyxRpcServer *server, StyxClient *client, Exchange *test)
{
    StyxRpcMessageType type = {"test.variable-ref", 1, 29, accept_ref, &test->received};
    StyxRpcMessageSpec spec = {&type, 1, NULL, NULL, NULL};
    StyxRpcMessagePort *port = NULL;
    StyxRpcVfsFile *first = NULL, *second = NULL, *replacement = NULL, *denied = NULL;
    StyxIntVariable variable = {test, load, store};
    unsigned char bytes[STYXRPC_INT_REF_SIZE];
    StyxRpcIntRef fresh = {0};
    int32_t value = -1;
    int good = styxrpc_messages_open_at(client, "refs", &spec, &port) == STYXRPC_OK &&
        styxrpc_messages_pump(port) == STYXRPC_OK &&
        styxrpc_int_ref_open(client, &test->received, &first) == STYXRPC_IO_OK &&
        (denied = first) != NULL &&
        styxrpc_int_ref_open_until(client, &test->received, &denied, stopped, NULL) == STYXRPC_IO_CANCELLED &&
        denied == first;
    denied = NULL;
    good = good && styxrpc_int_ref_encode(&test->received, bytes, sizeof(bytes)) &&
        styxrpc_message_send(port, "test.variable-ref", 1, bytes, sizeof(bytes)) == STYXRPC_OK &&
        styxrpc_messages_pump(port) == STYXRPC_MORE &&
        styxrpc_int_ref_open_until(client, &test->received, &second, NULL, NULL) == STYXRPC_IO_OK &&
        styxrpc_vfs_int_write(first, -225) == STYXRPC_IO_OK &&
        styxrpc_vfs_int_read(second, &value) == STYXRPC_IO_OK && value == -225 &&
        !styxrpc_program_server_revoke(server, test->received.pid) &&
        styxrpc_int_ref_revoke(server, &test->received) &&
        !styxrpc_int_ref_revoke(server, &test->received) &&
        styxrpc_int_ref_grant(server, &variable, 0, &fresh) && fresh.pid != test->received.pid &&
        styxrpc_vfs_int_read(first, &value) != STYXRPC_IO_OK &&
        styxrpc_vfs_int_write(second, 12345) != STYXRPC_IO_OK &&
        styxrpc_int_ref_open(client, &test->received, &denied) == STYXRPC_IO_NOT_OPEN &&
        styxrpc_int_ref_open(client, &fresh, &replacement) == STYXRPC_IO_OK &&
        styxrpc_vfs_int_read(replacement, &value) == STYXRPC_IO_OK && value == -225;
    if (good)
    {
        StyxRpcIntRef spoof = fresh;
        spoof.writable = 1;
        good = styxrpc_int_ref_open(client, &spoof, &denied) == STYXRPC_IO_NOT_OPEN &&
            !styxrpc_int_ref_revoke(server, &spoof) && styxrpc_int_ref_revoke(server, &fresh);
    }
    styxrpc_vfs_close(first, 0);
    styxrpc_vfs_close(second, 0);
    styxrpc_vfs_close(denied, 0);
    styxrpc_vfs_close(replacement, 0);
    styxrpc_messages_close(port);
    return good;
}


/* EN: Check the bounded wire shape independent of transport.
 * RU: Проверить ограниченный сетевой формат отдельно от транспорта. */
static int wire_contract(void)
{
    const StyxRpcIntRef input = {UINT32_C(0x8090a0b0), 1};
    const unsigned char expected[] = {'V', 'R', 1, 1, 0x80, 0x90, 0xa0, 0xb0};
    unsigned char bytes[STYXRPC_INT_REF_SIZE];
    StyxRpcIntRef unchanged = {0};
    if (!styxrpc_int_ref_encode(&input, bytes, sizeof(bytes)) || memcmp(bytes, expected, sizeof(bytes)) ||
        styxrpc_int_ref_decode(bytes, sizeof(bytes) - 1, &unchanged) || unchanged.pid)
    {
        return 0;
    }
    bytes[3] = 2;
    return !styxrpc_int_ref_decode(bytes, sizeof(bytes), &unchanged) && !unchanged.pid;
}


/* EN: Offer one original reference on the portable mixed exchange schema.
 * RU: Выдать исходную ссылку по переносимой схеме смешанного обмена. */
static void offer_mixed(StyxRpcMessagePort *port, void *state)
{
    const Exchange *test = state;
    unsigned char bytes[STYXRPC_INT_REF_SIZE];
    if (styxrpc_int_ref_encode(&test->issued, bytes, sizeof(bytes)))
    {
        styxrpc_message_send(port, STYXRPC_CHANNEL_MESSAGE_REF, STYXRPC_CHANNEL_MESSAGE_VERSION,
            bytes, sizeof(bytes));
    }
}


/* EN: Echo the identical live lease only after validating its PID and mode.
 * RU: Вернуть ту же живую ссылку после проверки её PID и режима. */
static int echo_mixed(StyxRpcMessagePort *port, void *state, const void *bytes, size_t size)
{
    return forwarded(port, state, bytes, size) &&
        styxrpc_message_send(port, STYXRPC_CHANNEL_MESSAGE_REF, STYXRPC_CHANNEL_MESSAGE_VERSION,
            bytes, size) == STYXRPC_OK;
}


/* EN: Serve one leased cell to a real DisVM over one inherited paired carrier.
 * RU: Обслужить выданную ячейку для DisVM на унаследованном парном потоке. */
static int dis_provider(int fd, int mixed)
{
    StyxRpcRegistration nodes[6];
    StyxRpcPrograms programs;
    Exchange test = {.value = 41};
    const StyxIntVariable variable = {&test, load, store};
    unsigned char bytes[STYXRPC_INT_REF_SIZE];
    PhyBidir *mux = phybidircreate(phyposixfd(fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), 65536);
    if (!mux)
    {
        return 1;
    }
    StyxRpcServer *server = NULL;
    if (styxrpc_programs_init(&programs, nodes, 6))
    {
        server = styxrpc_program_server_create(phybidirchannel(mux, 0), &programs);
    }
    int ok = server && styxrpc_int_ref_grant(server, &variable, 1, &test.issued);
    if (ok && mixed)
    {
        StyxRpcMessageType type = {STYXRPC_CHANNEL_MESSAGE_REF, STYXRPC_CHANNEL_MESSAGE_VERSION,
            19, echo_mixed, &test};
        StyxRpcMessageSpec spec = {&type, 1, offer_mixed, NULL, &test};
        ok = styxrpc_messages_publish(server, STYXRPC_EXCHANGE_DEFAULT, &spec) == STYXRPC_OK;
    }
    else if (ok)
    {
        ok = styxrpc_int_ref_encode(&test.issued, bytes, sizeof(bytes)) &&
            fwrite(bytes, 1, sizeof(bytes), stdout) == sizeof(bytes) && !fflush(stdout);
    }
    if (ok)
    {
        serve(server);
    }
    styxrpc_server_destroy(server);
    phybidirfree(mux);
    return ok && test.value == -32 && (!mixed || test.forwarded) ? 0 : 1;
}


/* EN: Run on a real attached carrier with only one temporary cell worth of registry space.
 * RU: Проверить настоящее соединение с реестром на одну временную ячейку. */
int main(int argc, char **argv)
{
    if (argc == 3 && (!strcmp(argv[1], "--dis-fd") || !strcmp(argv[1], "--dis-mixed-fd")))
    {
        char *end;
        long fd = strtol(argv[2], &end, 10);
        return *argv[2] && !*end && fd >= 3 && fd <= INT_MAX ? dis_provider((int)fd, !strcmp(argv[1], "--dis-mixed-fd")) : 1;
    }
    if (argc != 1)
    {
        return 1;
    }
    StyxRpcRegistration nodes[6];
    StyxRpcPrograms programs;
    int sockets[2];
    if (!wire_contract() || !styxrpc_programs_init(&programs, nodes, 6) ||
        socketpair(AF_UNIX, SOCK_STREAM, 0, sockets))
    {
        return 1;
    }
    Exchange test = {.value = 41};
    StyxIntVariable variable = {&test, load, store};
    StyxRpcServer *server = styxrpc_program_server_create(
        phyposixfd(sockets[1], PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), &programs);
    StyxRpcMessageType type = {"test.variable-ref", 1, 37, forwarded, &test};
    StyxRpcMessageSpec spec = {&type, 1, offer, NULL, &test};
    StyxRpcIntRef overflow = {0};
    pthread_t worker;
    if (!server || styxrpc_messages_publish(server, "refs", &spec) != STYXRPC_OK ||
        !styxrpc_int_ref_grant(server, &variable, 1, &test.issued) ||
        styxrpc_int_ref_grant(server, &variable, 1, &overflow) || overflow.pid ||
        pthread_create(&worker, NULL, serve, server))
    {
        close(sockets[0]);
        styxrpc_server_destroy(server);
        return 1;
    }
    StyxClient *client = styxclientmountfd(sockets[0], 4096, "test", "", 1000, 8);
    int good = client && exchange(server, client, &test);
    styxclientshutdown(client);
    styxclientfree(client);
    pthread_join(worker, NULL);
    styxrpc_server_destroy(server);
    if (!good || !test.forwarded || test.value != -225)
    {
        fputs("variable reference exchange failed\n", stderr);
        return 1;
    }
    return 0;
}
