/* Copyright (c) 2026 bhgv
 * EN: Concurrent request/reply FIFO, hash collision and shutdown regressions.
 * RU: Регрессии параллельных FIFO запросов/ответов, коллизий и остановки.
 */
#include "owner_messages.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define CLIENTS 8 /* Concurrent independently addressed requesters. */
#define REQUESTS 256 /* Backlog per requester before reading replies. */
#define SERVER 99999 /* Service destination, distinct from requester IDs. */


/* EN: Borrowed router and uniquely owned client message storage until join.
 * RU: Заимствованный router и личные сообщения клиента до join. */
typedef struct Client
{
    DVM_MessageRouter *router; /* Shared routing mutex protects transport. */
    DVM_MessageEndpoint *inbox; /* Single-consumer reply endpoint. */
    uintptr_t id; /* IDs intentionally collide in the hash table. */
    DVM_OwnerMessage nodes[REQUESTS]; /* Owned until send and after reply. */
} Client;


/* EN: Terminate on a failed condition; text names the violated contract.
 * RU: Завершить при ложном condition; text описывает нарушенный контракт. */
static void require(int condition, const char *text)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL owner messages: %s\n", text);
        exit(1);
    }
}


/* EN: Send the whole client's backlog, then verify ordered correlated replies.
 * argument borrows Client until join. Returns null; failures terminate the test.
 * RU: Отправить пачку клиента, проверить порядок и соответствие ответов.
 * argument заимствует Client до join. Вернуть null; ошибка завершает тест. */
static void *client_run(void *argument)
{
    Client *client = argument;
    for (unsigned i = 0; i < REQUESTS; i++)
    {
        DVM_OwnerMessage *node = &client->nodes[i];
        node->sender = client->id;
        node->request = i;
        node->kind = 1;
        node->value = (intptr_t)i;
        require(dvm_messages_send(client->router, SERVER, 0, node), "request accepted");
    }
    for (unsigned i = 0; i < REQUESTS; i++)
    {
        DVM_OwnerMessage *node = dvm_messages_take(client->inbox, 1, 1);
        require(node == &client->nodes[i], "reply identity and FIFO");
        require(node->sender == client->id && node->request == i &&
            node->kind == 2 && node->value == (intptr_t)(i + 1000), "reply payload");
        require(!node->next, "taken reply detached");
    }
    return NULL;
}


/* EN: Serve a fixed total on a borrowed endpoint, returning each owned node.
 * argument is Client carrying service inbox/router. Returns null, fatal on error.
 * RU: Обработать заданное число запросов, вернуть каждый полученный узел.
 * argument — Client с inbox/router службы. Вернуть null, ошибка фатальна. */
static void *server_run(void *argument)
{
    Client *server = argument;
    for (unsigned i = 0; i < CLIENTS * REQUESTS; i++)
    {
        DVM_OwnerMessage *node = dvm_messages_take(server->inbox, 0, 1);
        require(node && node->kind == 1, "server receives request");
        node->kind = 2;
        node->value += 1000;
        require(dvm_messages_send(server->router, node->sender, 1, node), "reply accepted");
    }
    return NULL;
}


/* EN: Close and remove a borrowed endpoint after its reader has joined.
 * RU: Закрыть и удалить заимствованный endpoint после join читателя. */
static void close_endpoint(DVM_MessageRouter *router, DVM_MessageEndpoint *endpoint)
{
    dvm_messages_stop(endpoint);
    require(dvm_messages_unregister(router, endpoint), "drained endpoint unregisters");
}


/* EN: Verify embedded-node coalescing, retained payload and stop/drain ownership.
 * router is borrowed; no concurrent readers access this temporary endpoint.
 * RU: Проверить объединение повторов узла, сохранность данных и stop/drain.
 * router заимствован; временный endpoint не имеет параллельных читателей. */
static void check_stop(DVM_MessageRouter *router)
{
    DVM_MessageEndpoint *endpoint = dvm_messages_register(router, SERVER);
    DVM_OwnerMessage first = {0}, second = {0}, refused = {0};
    require(endpoint != NULL, "service ID reusable after unregister");
    first.kind = 7;
    first.value = 123;
    require(dvm_messages_send_once(router, SERVER, &first) == 1, "first embedded send");
    require(dvm_messages_send_once(router, SERVER, &first) == 2, "embedded duplicate coalesces");
    require(first.kind == 7 && first.value == 123, "pending payload retained");
    require(dvm_messages_take(endpoint, 0, 0) == &first, "embedded node consumed once");
    require(!dvm_messages_take(endpoint, 0, 0), "coalescing leaves no duplicate");
    require(dvm_messages_send_once(router, SERVER, &first) == 1, "consumed embedded node reusable");
    require(dvm_messages_send(router, SERVER, 0, &second), "second request queued");
    require(dvm_messages_send(router, SERVER, 1, &refused), "reply queued independently");
    dvm_messages_stop(endpoint);
    require(!dvm_messages_unregister(router, endpoint), "pending endpoint cannot unregister");
    require(dvm_messages_take(endpoint, 1, 1) == &refused, "stopped replies drain");
    refused.next = &first;
    refused.value = 789;
    require(!dvm_messages_send(router, SERVER, 0, &refused), "stopped send refused");
    require(refused.next == &first && refused.value == 789, "refusal retains caller ownership");
    require(dvm_messages_take(endpoint, 0, 1) == &first, "stopped request FIFO head");
    require(dvm_messages_take(endpoint, 0, 1) == &second, "stopped request FIFO tail");
    require(!dvm_messages_take(endpoint, 0, 1), "stopped empty request returns");
    require(!dvm_messages_take(endpoint, 1, 1), "stopped empty reply returns");
    require(dvm_messages_unregister(router, endpoint), "stopped drained unregister");
    require(!dvm_messages_send(router, SERVER, 0, &refused), "absent destination refused");
}


/* EN: Exercise colliding parallel clients and shutdown, then free all transport.
 * No arguments; return zero on success, terminate on contract failure.
 * RU: Проверить параллельных клиентов с коллизиями и остановку, удалить транспорт.
 * Без аргументов; вернуть ноль при успехе, завершить при нарушении контракта. */
int main(void)
{
    DVM_MessageRouter *router = dvm_messages_create();
    Client clients[CLIENTS] = {0};
    Client server = {0};
    pthread_t service, threads[CLIENTS];
    require(router != NULL, "router created");
    server.router = router;
    server.inbox = dvm_messages_register(router, SERVER);
    require(server.inbox != NULL, "service registered");
    require(!dvm_messages_register(router, SERVER), "duplicate registration refused");
    for (unsigned i = 0; i < CLIENTS; i++)
    {
        clients[i].router = router;
        clients[i].id = 1 + 257 * i;
        clients[i].inbox = dvm_messages_register(router, clients[i].id);
        require(clients[i].inbox != NULL, "colliding requester registered");
    }
    require(!pthread_create(&service, NULL, server_run, &server), "service started");
    for (unsigned i = 0; i < CLIENTS; i++)
    {
        require(!pthread_create(&threads[i], NULL, client_run, &clients[i]), "client started");
    }
    for (unsigned i = 0; i < CLIENTS; i++)
    {
        require(!pthread_join(threads[i], NULL), "client joined");
        close_endpoint(router, clients[i].inbox);
    }
    require(!pthread_join(service, NULL), "service joined");
    close_endpoint(router, server.inbox);
    check_stop(router);
    dvm_messages_destroy(router);
    puts("PASS 2048 parallel requests/replies: hash collisions, FIFO, coalescing, stop/drain ownership");
    return 0;
}
