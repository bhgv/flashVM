/* Copyright (c) 2026 bhgv
 * EN: Allocation rollback, exact offer/renewal correlation and duplicate reset handling without transport timing.
 * RU: Откат выделений, точная корреляция предложений/сбросов и повторы сбросов без transport-таймингов.
 */
#include "../internal.h"
#include <stdio.h>
static int fail; /* Single-threaded fixture allocator failure selector. */


/* EN/RU: Original linker allocator, owned result/NULL; no wrapper recursion.
 * Исходный allocator линкера, собственный результат/NULL; без рекурсии wrapper.
 */
extern void *__real_malloc(size_t size);


/* EN/RU: Refuse selected copied-message allocations before state mutation; fixture has no workers.
 * Отказать отмеченным выделениям сообщений до изменения состояния; workers в fixture отсутствуют.
 */
void *__wrap_malloc(size_t size)
{
    return fail ? NULL : __real_malloc(size);
}


/* EN/RU: Fatal explicit contract, no optimized-away condition.
 * Явная обязательная проверка, условие не удаляется оптимизатором.
 */
static void check(int condition, int line)
{
    if (!condition)
    {
        fprintf(stderr, "ordered coordinator line %d\n", line);
        exit(2);
    }
}
#define CHECK(value) check((value) != 0, __LINE__) /* Always evaluate the fixture contract. */


/* EN/RU: Own workerless coordinator fixture; all normal queues and engine remain production implementations.
 * Своя fixture без workers; очереди и engine используются из production-реализации.
 */
static StyxOrdered *create(void)
{
    StyxOrdered *peer = calloc(1, sizeof(*peer)); /* Private native test owner. */
    CHECK(peer && !pthread_mutex_init(&peer->lock, NULL) && !pthread_cond_init(&peer->changed, NULL));
    peer->limit = 128;
    peer->window = 8;
    peer->engine = styxordercreate(128, 8);
    peer->clients = calloc(1, sizeof(*peer->clients));
    CHECK(peer->engine && peer->clients);
    peer->clients->peer = peer;
    peer->clients->tag = 7;
    return peer;
}


/* EN/RU: Feed one original encoded packet at ordinal0, copying payload before temporary storage expires.
 * Передать исходный пакет с номером0, скопировав payload до завершения временного буфера.
 */
static void feed(StyxOrdered *peer, const char *text)
{
    unsigned char bytes[128]; /* Private complete packet. */
    FVM_SOrdinal number = {0, 0}; /* Exact selected ordinal. */
    size_t size; /* Encoded extent. */
    CHECK(fvm_sordered_write(&number, 7, text, strlen(text), bytes, sizeof(bytes), &size) == FVM_S_OK);
    CHECK(styxorderfeed(peer->engine, bytes, size) == STYX_ORDER_ACCEPTED);
}


/* EN/RU: Native-only deterministic transitions, allocation refusal before pending/active mutations.
 * Детерминированные native-переходы, отказ выделения до изменения pending/active.
 */
int main(void)
{
    StyxOrdered *peer = create(); /* Owned workerless native coordinator. */
    StyxOrderedClient *client = peer->clients; /* Borrowed immutable capability. */
    fail = 1;
    CHECK(!styxorderedoffer(peer, 1, 71) && !peer->tx_pending);
    fail = 0;
    CHECK(styxorderedoffer(peer, 1, 71));
    FVM_SOrderControl answer = peer->offer; /* Exact proposed scalar values. */
    answer.command = FVM_SORDER_ACCEPT;
    answer.lane++;
    CHECK(!orderedprofile(peer, &answer) && peer->tx_pending && !peer->active[STYX_ORDER_TX]);
    answer.lane--;
    CHECK(orderedprofile(peer, &answer) && !peer->tx_pending);
    CHECK(orderedprofile(peer, &answer));
    answer.command = FVM_SORDER_REJECT;
    CHECK(!orderedprofile(peer, &answer));
    FVM_SOrderControl offer = peer->offer; /* Independent RX profile. */
    offer.lane = 91;
    offer.window = orderednumber(4);
    fail = 1;
    CHECK(orderedprofile(peer, &offer) == -1 && !peer->active[STYX_ORDER_RX]);
    fail = 0;
    CHECK(orderedprofile(peer, &offer));
    CHECK(styxorderedclientsend(client, "old", 3));
    fail = 1;
    CHECK(!styxorderedrenew(peer, 7) && !client->tx_pending);
    fail = 0;
    CHECK(styxorderedrenew(peer, 7));
    answer = client->tx_request;
    answer.command = FVM_SORDER_ACK;
    answer.operation = orderednumber(9);
    CHECK(orderedrenewal(peer, &answer) && client->tx_pending);
    answer.operation = orderednumber(1);
    answer.command = FVM_SORDER_NACK;
    CHECK(orderedrenewal(peer, &answer) && !client->tx_pending);
    CHECK(styxorderedclientsend(client, "next", 4) && client->last == 1);
    CHECK(styxorderedrenew(peer, 7));
    answer = client->tx_request;
    answer.command = FVM_SORDER_ACK;
    CHECK(orderedrenewal(peer, &answer));
    CHECK(styxorderedclientsend(client, "new", 3) && !client->last);
    CHECK(orderedrenewal(peer, &answer)); /* Late identical ACK must not reset a second time. */
    CHECK(styxorderedclientsend(client, "again", 5) && client->last == 1);
    feed(peer, "old");
    FVM_SOrderControl reset = {0}; /* Peer RX request at old ordinal0. */
    reset.command = FVM_SORDER_RENEW;
    reset.operation = orderednumber(1);
    reset.lane = 91;
    reset.tag = 7;
    fail = 1;
    CHECK(orderedrenewal(peer, &reset) == -1 && !client->rx_pending);
    fail = 0;
    CHECK(orderedrenewal(peer, &reset) && client->rx_pending);
    size_t size; /* Exact returned body size. */
    unsigned long long ordinal; /* Exact returned ordinal. */
    char bytes[16]; /* Copied payload. */
    CHECK(styxorderedclientnext(client, &ordinal, bytes, 1, &size) == STYX_ORDER_SMALL && client->rx_pending);
    CHECK(styxorderedclientnext(client, &ordinal, bytes, sizeof(bytes), &size) == STYX_ORDER_RECORD && !client->rx_pending);
    feed(peer, "new");
    CHECK(orderedrenewal(peer, &reset)); /* Replay ACK, never reset new queued data. */
    CHECK(styxorderedclientnext(client, &ordinal, bytes, sizeof(bytes), &size) == STYX_ORDER_RECORD && !ordinal && !memcmp(bytes, "new", 3));
    CHECK(!styxorderreceivewindow(peer->engine, 2));
    CHECK(!styxorderedofferwait(peer, 99, 0));
    styxorderedfree(peer);
    return 0;
}
