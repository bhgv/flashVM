/* Copyright (c) 2026 bhgv
 * EN: Exercise independent custom message fids and named RPC on one mounted socket.
 * RU: Проверка отдельных fid сообщений и именованных RPC через смонтированный сокет.
 */
#include "styxrpc/messages.h"
#include "styxrpc/scalar_module.h"
#include "styxrpc/scalar_client.h"
#include "styxrpc/vfs_client.h"
#include "client.h"
#include "../src/vfs_client_internal.h"
#include "drivers/posix/fd.h"
#include "flashvm/sbindings.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct MessageTest
{
    StyxRpcServer *server; /* Moved to the dispatcher thread before the mount. */
    const StyxRpcMessageSpec *replacement; /* Copied on a live hosted replacement. */
    size_t opened;         /* Dispatcher opens, read by main after join. */
    size_t closed;         /* Dispatcher closes, read by main after join. */
    size_t received;       /* Accepted sample records, read by main after join. */
    int error;             /* Hook failure, read by main after join. */
} MessageTest;

typedef struct Receiver
{
    size_t greetings;      /* Client's accepted zero-byte hello records. */
    size_t echoes;         /* Client's accepted maximum-size samples. */
} Receiver;


/* EN: Check that payload is the full deterministic binary sample, including NUL.
 * RU: Проверить, что payload содержит весь эталонный двоичный образец, включая NUL. */
static int is_sample(const void *payload, size_t size)
{
    if (size != STYXRPC_MESSAGE_PAYLOAD_MAX)
    {
        return 0;
    }
    const unsigned char *bytes = payload;
    for (size_t i = 0; i < size; ++i)
    {
        if (bytes[i] != (unsigned char)i)
        {
            return 0;
        }
    }
    return 1;
}


/* EN: Count a valid sample in context and echo the first one on port. Return 0 on error.
 * RU: Учесть образец в context и отправить первый обратно через port. Вернуть 0 при ошибке. */
static int server_sample(StyxRpcMessagePort *port, void *context, const void *payload, size_t size)
{
    MessageTest *test = context;
    ++test->received;
    return is_sample(payload, size) && (test->received != 1 ||
        styxrpc_message_send(port, "app.sample", 1, payload, size) == STYXRPC_OK);
}


/* EN: Count a valid binary echo in context. The borrowed port is unused.
 * RU: Учесть правильное двоичное эхо в context. Заимствованный port не используется. */
static int client_sample(StyxRpcMessagePort *port, void *context, const void *payload, size_t size)
{
    (void)port;
    Receiver *receiver = context;
    receiver->echoes++;
    return is_sample(payload, size);
}


/* EN: Count a zero-byte greeting in context. The borrowed port and payload are unused.
 * RU: Учесть пустое приветствие в context. Заимствованные port и payload не используются. */
static int client_hello(StyxRpcMessagePort *port, void *context, const void *payload, size_t size)
{
    (void)port;
    (void)payload;
    Receiver *receiver = context;
    receiver->greetings++;
    return !size;
}


/* EN: Queue the first server message on each opened port. Save hook counts in context.
 * RU: Поставить первое сообщение на каждый открытый port. Учесть вызов в context. */
static void opened_port(StyxRpcMessagePort *port, void *context)
{
    MessageTest *test = context;
    ++test->opened;
    if (styxrpc_message_send(port, "app.hello", 1, NULL, 0) != STYXRPC_OK)
    {
        test->error = 1;
    }
}


/* EN: Confirm a closing port rejects sends, and record closure in context.
 * RU: Проверить отказ отправки закрываемого port и учесть закрытие в context. */
static void closed_port(StyxRpcMessagePort *port, void *context)
{
    MessageTest *test = context;
    ++test->closed;
    if (styxrpc_message_send(port, "app.hello", 1, NULL, 0) != STYXRPC_INVALID)
    {
        test->error = 1;
    }
}


/* EN: Return argument+1 in result for an ordinary scalar call. Context is unused.
 * RU: Выдать argument+1 в result для обычного скалярного вызова. context не используется. */
static int increment(void *context, int64_t argument, int64_t *result)
{
    (void)context;
    *result = argument + 1;
    return 1;
}


/* EN: Replace events on the dispatcher, independently of exchange callbacks.
 * RU: Заменить events у диспетчера вне колбэков сообщений. */
static int replace_events(void *context, int64_t argument, int64_t *result)
{
    MessageTest *test = context;
    *result = argument;
    return styxrpc_messages_withdraw(test->server, "events") == STYXRPC_OK &&
        styxrpc_messages_publish(test->server, "events", test->replacement) == STYXRPC_OK;
}


/* EN: Process requests on context's owner thread and consume the server on link loss.
 * RU: Обслуживать запросы в потоке владельца context и освободить сервер при обрыве. */
static void *dispatch(void *context)
{
    MessageTest *test = context;
    while (styxrpc_server_step(test->server))
    {
    }
    styxrpc_server_destroy(test->server);
    return NULL;
}


/* EN: Confirm an ordinary named call on the same attached client still works.
 * RU: Проверить обычный именованный вызов на том же подключённом клиенте. */
static int call_increment(StyxClient *client)
{
    int64_t result;
    return styxrpc_scalar_invoke(client, "mounted/messages.dis", "increment", STYXRPC_INT_SIGNATURE,
        41, 1000, &result) == STYXRPC_CALL_OK && result == 42;
}


/* EN: Count a root message entry in context and check its metadata.
 * RU: Учесть запись сообщений в корне в context и проверить её метаданные. */
static int visit(void *context, const char *name, const StyxRpcFileInfo *info)
{
    if ((!strcmp(name, STYXRPC_MESSAGES_FILE) || !strcmp(name, "events") ||
        !strcmp(name, STYXRPC_UNIBUS_FILE)) && !info->directory && !info->length)
    {
        ++*(unsigned *)context;
    }
    return 1;
}


/* EN: Read batches from port while an echoed sample is pending in receiver.
 * RU: Читать порции из port, пока receiver ожидает эхо образца. */
static int read_echo(StyxRpcMessagePort *port, const Receiver *receiver)
{
    for (size_t round = 0; round < 8 && !receiver->echoes; ++round)
    {
        if (styxrpc_messages_pump(port) == STYXRPC_INVALID)
        {
            return 0;
        }
    }
    return receiver->echoes == 1 && styxrpc_messages_pump(port) == STYXRPC_MORE;
}


/* EN: Exchange registered data on an active fid and reject an unsupported schema on its sibling.
 * RU: Обменяться зарегистрированными данными, отклонить неподдерживаемую схему на соседнем fid. */
static int exchange(StyxClient *client)
{
    Receiver receiver = {0}, sibling = {0};
    char sample_name[] = "app.sample"; /* Test that the name is copied before opening returns. */
    StyxRpcMessageType types[] = {{sample_name, 1, UINT32_MAX, client_sample, &receiver},
        {"app.hello", 1, 0, client_hello, &receiver}};
    StyxRpcMessageType unknown[] = {{"app.sample", 2, 17, NULL, NULL},
        {"app.hello", 1, 0, client_hello, &sibling}};
    StyxRpcMessageSpec spec = {.types = types, .count = 2};
    StyxRpcMessageSpec other_spec = {.types = unknown, .count = 2};
    StyxRpcMessagePort *port = NULL, *other = NULL;
    StyxRpcVfsFile *read_only = NULL;
    unsigned entries = 0;
    int good = call_increment(client) &&
        styxrpc_vfs_list(client, "", 0, 2, visit, &entries) == STYXRPC_IO_OK && entries == 0 &&
        styxrpc_vfs_open(client, STYXRPC_MESSAGES_FILE, STYXRPC_FILE_RDWR, &read_only) == STYXRPC_IO_NOT_OPEN &&
        styxrpc_messages_open(client, &spec, &port) == STYXRPC_OK &&
        styxrpc_messages_open_at(client, NULL, &other_spec, &other) == STYXRPC_OK;
    StyxRpcFileInfo hidden;
    good = good && styxrpc_vfs_stat(client, "events", &hidden) == STYXRPC_IO_NOT_OPEN &&
        styxrpc_vfs_open(client, "events", STYXRPC_FILE_RDWR, &read_only) == STYXRPC_IO_NOT_OPEN;
    sample_name[0] = '!';
    unsigned char bytes[STYXRPC_MESSAGE_PAYLOAD_MAX];
    for (size_t i = 0; i < sizeof(bytes); ++i)
    {
        bytes[i] = (unsigned char)i;
    }
    good = good && styxrpc_messages_pump(port) == STYXRPC_OK && receiver.greetings == 1 &&
        styxrpc_messages_pump(other) == STYXRPC_OK && sibling.greetings == 1 &&
        styxrpc_message_send(port, "app.hello", 1, NULL, 0) == STYXRPC_INVALID &&
        styxrpc_message_send(port, "app.sample", 1, bytes, sizeof(bytes)) == STYXRPC_OK &&
        styxrpc_message_send(port, "app.sample", 1, bytes, sizeof(bytes)) == STYXRPC_OK &&
        styxrpc_message_send(port, "app.sample", 1, bytes, sizeof(bytes)) == STYXRPC_MORE &&
        styxrpc_message_send(other, "app.sample", 2, bytes, 1) == STYXRPC_OK &&
        styxrpc_messages_pump(other) == STYXRPC_INVALID &&
        styxrpc_message_send(other, "app.sample", 2, NULL, 0) == STYXRPC_INVALID &&
        call_increment(client) && read_echo(port, &receiver);
    styxrpc_vfs_close(read_only, 0);
    styxrpc_messages_abandon(other);
    styxrpc_messages_close(port);
    return good;
}


/* EN: Send a checked declaration and then a corrupt data record on raw file; no handler may run.
 * RU: Послать через file проверенное объявление и затем испорченную запись. Обработчик не вызывается. */
static int reject_corrupt(StyxRpcVfsFile *file)
{
    FVM_SBinding type = {5, 1, sizeof("app.sample") - 1, "app.sample"};
    unsigned char body[64], packet[100];
    size_t body_size, declared, record, used;
    if (fvm_sbindings_payload_write(&type, 1, body, sizeof(body), &body_size) != FVM_S_OK ||
        fvm_srecord_write(0, body, body_size, packet, sizeof(packet), &declared) != FVM_S_OK ||
        fvm_srecord_write(5, "bad", 3, packet + declared, sizeof(packet) - declared, &record) != FVM_S_OK ||
        styxrpc_vfs_file_write(file, 0, packet, declared, &used) != STYXRPC_IO_OK || used != declared)
    {
        return 0;
    }
    packet[declared + record - 1] ^= 1;
    return styxrpc_vfs_file_write(file, declared, packet + declared, record, &used) != STYXRPC_IO_OK &&
        styxrpc_vfs_file_read(file, 0, packet, sizeof(packet), &used) != STYXRPC_IO_OK;
}


/* EN: Replace one hosted root, invalidating both unopened and opened old fids.
 * RU: Заменить корень, отозвав старые fid и до открытия, и после. */
static int replace_and_check(StyxClient *client)
{
    StyxFid *walked = styxclientwalk(client, "events");
    StyxRpcVfsFile *opened = NULL, *replacement = NULL;
    int64_t result;
    unsigned char bytes[256];
    size_t read;
    int good = walked &&
        styxrpc_vfs_exchange_open(client, "events", STYXRPC_FILE_RDWR, &opened) == STYXRPC_IO_OK &&
        styxrpc_vfs_file_read(opened, 0, bytes, sizeof(bytes), &read) == STYXRPC_IO_OK && read > 0 &&
        styxrpc_scalar_invoke(client, "mounted/messages.dis", "replace", STYXRPC_INT_SIGNATURE,
            41, 1000, &result) == STYXRPC_CALL_OK && result == 41 &&
        !styxclientopen(walked, STYXRPC_FILE_RDWR) &&
        styxrpc_vfs_file_read(opened, read, bytes, sizeof(bytes), &read) != STYXRPC_IO_OK &&
        styxrpc_vfs_exchange_open(client, "events", STYXRPC_FILE_RDWR, &replacement) == STYXRPC_IO_OK &&
        styxrpc_vfs_file_read(replacement, 0, bytes, sizeof(bytes), &read) == STYXRPC_IO_OK && read > 0;
    if (walked)
    {
        styxclientclunk(walked);
    }
    styxrpc_vfs_close(opened, 0);
    styxrpc_vfs_close(replacement, 0);
    return good;
}


/* EN: Mount the real socket, verify framing, failure isolation, and dispose all owners.
 * RU: Смонтировать настоящий сокет, проверить framing и изоляцию ошибок, освободить ресурсы. */
int main(void)
{
    StyxRpcScalarModule module;
    MessageTest test = {0};
    const StyxRpcExport exports[] = {{"increment", STYXRPC_INT_SIGNATURE, increment, NULL},
        {"replace", STYXRPC_INT_SIGNATURE, replace_events, &test}};
    StyxRpcMessageType types[] = {{"app.sample", 1, 51, server_sample, &test},
        {"app.hello", 1, 51, NULL, NULL}};
    StyxRpcMessageSpec spec = {types, 2, opened_port, closed_port, &test};
    int sockets[2];
    if (styxrpc_scalar_module_init(&module, "messages.dis", exports, sizeof(exports) / sizeof(*exports)) != STYXRPC_OK ||
        socketpair(AF_UNIX, SOCK_STREAM, 0, sockets))
    {
        return 1;
    }
    test.server = styxrpc_server_create(phyposixfd(sockets[1], PHY_STREAM | PHY_ORDERED | PHY_RELIABLE),
        &styxrpc_scalar_module_tree, &module);
    int rejected = test.server && styxrpc_messages_enable(test.server, &spec) == STYXRPC_INVALID;
    types[1].send_tag = 55;
    StyxRpcMessageSpec isolated = {types + 1, 1, opened_port, closed_port, &test};
    test.replacement = &isolated;
    pthread_t owner;
    if (!rejected || styxrpc_messages_enable(test.server, &spec) != STYXRPC_OK ||
        styxrpc_messages_enable(test.server, &spec) != STYXRPC_INVALID ||
        styxrpc_messages_publish(test.server, "mounted", &isolated) != STYXRPC_INVALID ||
        styxrpc_messages_publish(test.server, NULL, &isolated) != STYXRPC_OK ||
        styxrpc_messages_publish(test.server, STYXRPC_UNIBUS_FILE, &isolated) != STYXRPC_INVALID ||
        styxrpc_messages_publish(test.server, "events", &isolated) != STYXRPC_OK ||
        pthread_create(&owner, NULL, dispatch, &test))
    {
        styxrpc_server_destroy(test.server);
        close(sockets[0]);
        fputs("custom message registration failed\n", stderr);
        return 1;
    }
    StyxClient *client = styxclientmountfd(sockets[0], 256, "test", "", 1000, 8);
    StyxRpcVfsFile *raw = NULL;
    int good = client && exchange(client) &&
        styxrpc_vfs_exchange_open(client, "events", STYXRPC_FILE_RDWR, &raw) == STYXRPC_IO_OK &&
        reject_corrupt(raw) && replace_and_check(client) && call_increment(client);
    styxrpc_vfs_close(raw, 0);
    styxclientshutdown(client);
    styxclientfree(client);
    pthread_join(owner, NULL);
    if (!good || test.error || test.opened != 5 || test.closed != 5 || test.received != 2)
    {
        fprintf(stderr, "message socket failed (I/O=%d, hook error=%d, opens=%zu, closes=%zu, samples=%zu)\n",
            good, test.error, test.opened, test.closed, test.received);
        return 1;
    }
    return 0;
}
