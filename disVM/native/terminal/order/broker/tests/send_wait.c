/* Copyright (c) 2026 bhgv
 * EN: Native per-client TX wait predicates, immutable deadlines, allocation rollback and sticky retirement ownership.
 * RU: Native TX-ожидания своего клиента, неизменные deadlines, откат выделений и владение sticky-подписками.
 */
#include "../internal.h"
#include <stdio.h>
static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER; /* Exact native wait entry checkpoints. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Fixture-only entry notification. */
static pthread_cond_t *watched; /* Selected production peer condition, stable through joined sender. */
static unsigned entered; /* Number of actual waits entered by current sender, under gate. */
static struct timespec original; /* First production deadline, must remain exact across unrelated wakes. */
static int fail_calloc; /* Armed only while background sender is waiting, with no concurrent registry allocation. */
static int fail_malloc; /* Armed only with sender threads joined, no allocator data race. */


/* EN/RU: Fatal explicit contract, never optimized away.
 * Явная обязательная проверка, не удаляемая оптимизатором.
 */
static void require(int condition, int line)
{
    if (!condition)
    {
        fprintf(stderr, "ordered sendwait line %d\n", line);
        exit(2);
    }
}
#define CHECK(value) require((value) != 0, __LINE__) /* Evaluated native fixture contract. */


/* EN/RU: Original allocation/condition entry points, no wrapper recursion.
 * Исходные allocation/condition-функции без рекурсии wrapper.
 */
extern void *__real_malloc(size_t);


/* EN/RU: Original zeroed registry allocation, owned result/NULL.
 * Исходное обнулённое выделение registry, собственный результат/NULL.
 */
extern void *__real_calloc(size_t, size_t);


/* EN/RU: Refuse selected first-counter allocation before any ordinal is accepted.
 * Отказать отмеченному выделению первого счётчика до принятия номера.
 */
void *__wrap_calloc(size_t count, size_t size)
{
    return fail_calloc ? NULL : __real_calloc(count, size);
}


/* EN/RU: Original timed native condition wait, borrowed mutex/deadline.
 * Исходное native-ожидание с заимствованными mutex/deadline.
 */
extern int __real_pthread_cond_timedwait(pthread_cond_t *, pthread_mutex_t *, const struct timespec *);


/* EN/RU: Refuse one selected synchronous spool allocation before numbering.
 * Отказать отмеченному синхронному выделению очереди до расходования номера.
 */
void *__wrap_malloc(size_t size)
{
    return fail_malloc ? NULL : __real_malloc(size);
}


/* EN/RU: Observe real production wait and require identical absolute deadline on every unrelated wake.
 * Наблюдать настоящее ожидание и проверять неизменность абсолютного deadline при посторонних пробуждениях.
 */
int __wrap_pthread_cond_timedwait(pthread_cond_t *condition, pthread_mutex_t *lock, const struct timespec *deadline)
{
    if (condition == watched)
    {
        pthread_mutex_lock(&gate);
        if (!entered)
        {
            original = *deadline;
        }
        CHECK(original.tv_sec == deadline->tv_sec && original.tv_nsec == deadline->tv_nsec);
        entered++;
        pthread_cond_broadcast(&changed);
        pthread_mutex_unlock(&gate);
    }
    return __real_pthread_cond_timedwait(condition, lock, deadline);
}


/* EN/RU: Own workerless production coordinator with two independent fixed-tag clients.
 * Свой production-координатор без I/O-workers, с двумя независимыми клиентами фиксированных тегов.
 */
static StyxOrdered *create(void)
{
    StyxOrdered *peer = calloc(1, sizeof(*peer)); /* Native owner, only test sender threads will access it. */
    pthread_condattr_t attributes; /* Actual production-compatible monotonic condition. */
    CHECK(peer && !pthread_mutex_init(&peer->lock, NULL) && !pthread_condattr_init(&attributes));
    CHECK(!pthread_condattr_setclock(&attributes, CLOCK_MONOTONIC) && !pthread_cond_init(&peer->changed, &attributes));
    pthread_condattr_destroy(&attributes);
    peer->limit = 128;
    peer->window = 8;
    peer->engine = styxordercreate(128, 8);
    peer->retired = dvm_cancel_create();
    CHECK(peer->engine && peer->retired);
    for (unsigned long tag = 7; tag <= 9; tag += 2)
    {
        StyxOrderedClient *client = calloc(1, sizeof(*client)); /* Owned predeclared client. */
        CHECK(client);
        client->peer = peer;
        client->tag = tag;
        client->next = peer->clients;
        peer->clients = client;
    }
    watched = &peer->changed;
    return peer;
}


/* EN/RU: Native fixture sender; result is read only after join, payload remains borrowed until return.
 * Native-отправитель fixture; результат читается после join, payload заимствован до возврата.
 */
typedef struct Sender
{
    StyxOrderedClient *client; /* Borrowed fixed-tag sender capability. */
    int result; /* Written by sender, published by pthread_join. */
} Sender;


/* EN/RU: One blocking native send, never loops on broker or guest polling.
 * Одна блокирующая native-отправка, без опроса брокера или гостя.
 */
static void *send(void *argument)
{
    Sender *sender = argument; /* Borrowed until caller joins. */
    sender->result = styxorderedclientsendwait(sender->client, "data", 4, 3000);
    return NULL;
}


/* EN/RU: Wait for exact timedwait entries, then release main without retaining peer mutex.
 * Дождаться точного числа входов в timedwait, не удерживая mutex peer.
 */
static void checkpoint(unsigned count)
{
    struct timespec deadline; /* Five-second test gate budget, separate from production deadline. */
    CHECK(!clock_gettime(CLOCK_REALTIME, &deadline));
    deadline.tv_sec += 5;
    pthread_mutex_lock(&gate);
    while (entered < count)
    {
        CHECK(!pthread_cond_timedwait(&changed, &gate, &deadline));
    }
    pthread_mutex_unlock(&gate);
}


/* EN/RU: Allocate a native subscriber context, retain through callback/source lifetime with no peer pointer.
 * Native-контекст подписчика живёт до конца callback/source, без указателя на peer.
 */
typedef struct Notice
{
    unsigned *calls; /* Borrowed fixture counter until both subscriptions retire. */
    unsigned *released; /* Borrowed final-disposal counter. */
} Notice;


/* EN/RU: Nonwaiting idempotent native subscriber, counts allowed repeated notifications only.
 * Native-подписчик без ожиданий, учитывает лишь допустимые повторные уведомления.
 */
static void receive(void *argument)
{
    Notice *notice = argument; /* Source pins callback context. */
    (*notice->calls)++;
}


/* EN/RU: Consume subscriber after all admitted callbacks; source pin can outlive peer.
 * Поглотить подписчика после всех callbacks; pin source может пережить peer.
 */
static void release(void *argument)
{
    Notice *notice = argument; /* Sole final owner. */
    (*notice->released)++;
    free(notice);
}


/* EN/RU: Exercise actual native waits and selected renewal while another client continues, then sticky shutdown.
 * Проверить настоящие native-ожидания и сброс своего тега при работающем соседнем клиенте, затем sticky-stop.
 */
int main(void)
{
    StyxOrdered *peer = create(); /* Owned native coordinator. */
    StyxOrderedClient *a = styxorderedclient(peer, 7); /* Borrowed selected tag. */
    StyxOrderedClient *b = styxorderedclient(peer, 9); /* Borrowed unrelated tag. */
    unsigned char oversized[128] = {0}; /* Local limit includes tag byte. */
    CHECK(styxorderedclientceiling(a) == 127);
    CHECK(styxorderedclientsendwait(a, "data", 4, 0) == STYX_ORDERED_WAIT);
    CHECK(styxorderedclientsendwait(a, oversized, sizeof(oversized), 3000) == STYX_ORDERED_SIZE);
    CHECK(styxorderedoffer(peer, 1, 71));
    Sender sender = {a, 99}; /* Joined result sentinel. */
    pthread_t worker; /* Native selected client sender. */
    CHECK(!pthread_create(&worker, NULL, send, &sender));
    checkpoint(1);
    pthread_mutex_lock(&peer->lock);
    FVM_SOrderControl answer = peer->offer; /* Exact directional proposal. */
    answer.command = FVM_SORDER_ACCEPT;
    CHECK(orderedprofile(peer, &answer));
    pthread_mutex_unlock(&peer->lock);
    CHECK(!pthread_join(worker, NULL) && sender.result == STYX_ORDERED_SENT && !a->last);
    CHECK(styxorderedrenew(peer, 7));
    entered = 0;
    CHECK(!pthread_create(&worker, NULL, send, &sender));
    checkpoint(1);
    fail_calloc = 1;
    CHECK(styxorderedclientsendwait(b, "other", 5, 0) == STYX_ORDERED_NOMEM);
    fail_calloc = 0;
    CHECK(styxorderedclientsendwait(b, "other", 5, 0) == STYX_ORDERED_SENT && !b->last);
    checkpoint(2); /* Unrelated queue notification must neither release A nor extend its deadline. */
    pthread_mutex_lock(&peer->lock);
    answer = a->tx_request;
    answer.command = FVM_SORDER_ACK;
    CHECK(orderedrenewal(peer, &answer));
    pthread_mutex_unlock(&peer->lock);
    CHECK(!pthread_join(worker, NULL) && sender.result == STYX_ORDERED_SENT && !a->last);
    fail_malloc = 1;
    CHECK(styxorderedclientsendwait(a, "x", 1, 0) == STYX_ORDERED_NOMEM);
    fail_malloc = 0;
    CHECK(styxorderedclientsendwait(a, "x", 1, 0) == STYX_ORDERED_SENT && a->last == 1);
    CHECK(styxorderedrenew(peer, 7));
    entered = 0;
    CHECK(!pthread_create(&worker, NULL, send, &sender));
    checkpoint(1);
    unsigned calls[2] = {0, 0}; /* Callback counts, contexts never store peer. */
    unsigned released = 0; /* Final context-disposal count. */
    DVM_CancelTap *taps[2]; /* Source pins retained beyond peer destruction. */
    for (unsigned i = 0; i < 2; i++)
    {
        if (i)
        {
            styxorderedstop(peer); /* Second registration must see sticky retirement before return. */
        }
        Notice *notice = malloc(sizeof(*notice)); /* Owned until subscription adopts it. */
        CHECK(notice);
        notice->calls = calls + i;
        notice->released = &released;
        taps[i] = styxorderedwatch(peer, receive, notice, release);
        CHECK(taps[i]);
    }
    CHECK(!pthread_join(worker, NULL) && sender.result == STYX_ORDERED_END && calls[0] && calls[1]);
    styxorderedfree(peer);
    CHECK(!released);
    dvm_cancel_unsubscribe(taps[0]);
    dvm_cancel_unsubscribe(taps[1]);
    CHECK(released == 2);
    return 0;
}
