/* Copyright (c) 2026 bhgv
 * EN: Route scalar messages to registered native consumers.
 * RU: Маршрутизация скалярных сообщений зарегистрированным потокам.
 */
#include "owner_messages.h"
#include <pthread.h>
#include <stdlib.h>

#define DVM_MESSAGE_BUCKETS 257 /* Fixed bucket count for native consumer IDs. */


/* EN: One FIFO and its notification, protected by the router mutex.
 * RU: FIFO и уведомление, защищённые мьютексом маршрутизатора. */
typedef struct DVM_MessageQueue
{
    DVM_OwnerMessage *head; /* First pending node. */
    DVM_OwnerMessage *tail; /* Last pending node. */
    pthread_cond_t ready;  /* Notified when a node becomes available or on stop. */
} DVM_MessageQueue;


/* EN: Registration and the two queues consumed by one native thread.
 * RU: Регистрация и две очереди, читаемые одним нативным потоком. */
struct DVM_MessageEndpoint
{
    DVM_MessageRouter *router;  /* Borrowed until unregistration. */
    DVM_MessageEndpoint *next; /* Hash chain link. */
    uintptr_t id;              /* Nonzero destination ID. */
    int stopped;               /* Reject sends and end empty waits. */
    DVM_MessageQueue requests; /* FIFO for work. */
    DVM_MessageQueue replies;  /* FIFO for completions. */
};


/* EN: Owns registrations and serializes queue updates and notification.
 * RU: Владеет регистрациями и защищает очереди и уведомления. */
struct DVM_MessageRouter
{
    pthread_mutex_t lock;                               /* Protects endpoints and queues. */
    DVM_MessageEndpoint *buckets[DVM_MESSAGE_BUCKETS];  /* Hash chains by destination. */
};


/* EN: Return the slot containing id or the empty insertion slot. Lock held.
 * RU: Вернуть адрес записи id или пустой ячейки. Мьютекс удерживается. */
static DVM_MessageEndpoint **find_slot(DVM_MessageRouter *router, uintptr_t id)
{
    DVM_MessageEndpoint **slot = &router->buckets[id % DVM_MESSAGE_BUCKETS];
    while (*slot && (*slot)->id != id)
    {
        slot = &(*slot)->next;
    }
    return slot;
}


/* EN: Allocate the routing table; null on allocation or mutex failure.
 * RU: Создать таблицу. При ошибке памяти или мьютекса вернуть null. */
DVM_MessageRouter *dvm_messages_create(void)
{
    DVM_MessageRouter *router = calloc(1, sizeof(*router));
    if (router && pthread_mutex_init(&router->lock, NULL))
    {
        free(router);
        return NULL;
    }
    return router;
}


/* EN: Free router after callers have removed all registrations.
 * RU: Освободить router после снятия всех регистраций. */
void dvm_messages_destroy(DVM_MessageRouter *router)
{
    pthread_mutex_destroy(&router->lock);
    free(router);
}


/* EN: Register unique id with two empty queues; null on failure.
 * RU: Зарегистрировать уникальный id с двумя очередями. При отказе вернуть null. */
DVM_MessageEndpoint *dvm_messages_register(DVM_MessageRouter *router, uintptr_t id)
{
    if (!id)
    {
        return NULL;
    }
    DVM_MessageEndpoint *endpoint = calloc(1, sizeof(*endpoint));
    if (!endpoint)
    {
        return NULL;
    }
    if (pthread_cond_init(&endpoint->requests.ready, NULL))
    {
        free(endpoint);
        return NULL;
    }
    if (pthread_cond_init(&endpoint->replies.ready, NULL))
    {
        pthread_cond_destroy(&endpoint->requests.ready);
        free(endpoint);
        return NULL;
    }
    endpoint->id = id;
    endpoint->router = router;
    pthread_mutex_lock(&router->lock);
    DVM_MessageEndpoint **slot = find_slot(router, id);
    if (*slot)
    {
        pthread_mutex_unlock(&router->lock);
        pthread_cond_destroy(&endpoint->requests.ready);
        pthread_cond_destroy(&endpoint->replies.ready);
        free(endpoint);
        return NULL;
    }
    *slot = endpoint;
    pthread_mutex_unlock(&router->lock);
    return endpoint;
}


/* EN: Remove a stopped and drained endpoint after its reader has joined.
 * RU: Удалить закрытый пустой endpoint после завершения его читателя. */
int dvm_messages_unregister(DVM_MessageRouter *router, DVM_MessageEndpoint *endpoint)
{
    pthread_mutex_lock(&router->lock);
    DVM_MessageEndpoint **slot = find_slot(router, endpoint->id);
    if (*slot != endpoint || !endpoint->stopped || endpoint->requests.head || endpoint->replies.head)
    {
        pthread_mutex_unlock(&router->lock);
        return 0;
    }
    *slot = endpoint->next;
    pthread_mutex_unlock(&router->lock);
    pthread_cond_destroy(&endpoint->requests.ready);
    pthread_cond_destroy(&endpoint->replies.ready);
    free(endpoint);
    return 1;
}


/* EN: Publish node in destination's selected FIFO and notify on empty transition.
 * RU: Опубликовать узел в выбранном FIFO destination и уведомить при переходе из пустого. */
static int send_node(DVM_MessageRouter *router, uintptr_t destination,
    int reply, DVM_OwnerMessage *node, int once)
{
    pthread_mutex_lock(&router->lock);
    DVM_MessageEndpoint *endpoint = *find_slot(router, destination);
    if (!endpoint || endpoint->stopped)
    {
        pthread_mutex_unlock(&router->lock);
        return 0;
    }
    DVM_MessageQueue *queue = reply ? &endpoint->replies : &endpoint->requests;
    if (once)
    {
        for (DVM_OwnerMessage *pending = queue->head; pending; pending = pending->next)
        {
            if (pending == node)
            {
                pthread_mutex_unlock(&router->lock);
                return 2;
            }
        }
    }
    node->next = NULL;
    if (queue->tail)
    {
        queue->tail->next = node;
    }
    else
    {
        queue->head = node;
        pthread_cond_signal(&queue->ready);
    }
    queue->tail = node;
    pthread_mutex_unlock(&router->lock);
    return 1;
}


/* EN: Send a uniquely owned node. RU: Отправить узел отправителя. */
int dvm_messages_send(DVM_MessageRouter *router, uintptr_t destination,
    int reply, DVM_OwnerMessage *node)
{
    return send_node(router, destination, reply, node, 0);
}


/* EN: Coalesce an embedded request until the consumer takes it.
 * RU: Объединить повторы встроенного запроса до его извлечения. */
int dvm_messages_send_once(DVM_MessageRouter *router, uintptr_t destination,
    DVM_OwnerMessage *node)
{
    return send_node(router, destination, 0, node, 1);
}


/* EN: Test the selected FIFO for kind, or any request if kind is zero.
 * RU: Проверить наличие kind в выбранном FIFO, либо любого запроса при нуле. */
int dvm_messages_pending(DVM_MessageEndpoint *endpoint, int reply, unsigned kind)
{
    int found = 0;
    pthread_mutex_lock(&endpoint->router->lock);
    DVM_MessageQueue *queue = reply ? &endpoint->replies : &endpoint->requests;
    for (DVM_OwnerMessage *node = queue->head; node; node = node->next)
    {
        if (!kind || node->kind == kind)
        {
            found = 1;
            break;
        }
    }
    pthread_mutex_unlock(&endpoint->router->lock);
    return found;
}


/* EN: Take the FIFO head. If wait is set, wait for publication or stop.
 * RU: Забрать голову FIFO. С wait ждать публикации или остановки. */
DVM_OwnerMessage *dvm_messages_take(DVM_MessageEndpoint *endpoint, int reply, int wait)
{
    DVM_MessageQueue *queue = reply ? &endpoint->replies : &endpoint->requests;
    pthread_mutex_t *lock = &endpoint->router->lock;
    pthread_mutex_lock(lock);
    while (wait && !queue->head && !endpoint->stopped)
    {
        pthread_cond_wait(&queue->ready, lock);
    }
    DVM_OwnerMessage *node = queue->head;
    if (node)
    {
        queue->head = node->next;
        if (!queue->head)
        {
            queue->tail = NULL;
        }
        node->next = NULL;
    }
    pthread_mutex_unlock(lock);
    return node;
}


/* EN: Close admission and wake a waiter on either FIFO.
 * RU: Закрыть приём и разбудить читателя любого FIFO. */
void dvm_messages_stop(DVM_MessageEndpoint *endpoint)
{
    pthread_mutex_t *lock = &endpoint->router->lock;
    pthread_mutex_lock(lock);
    endpoint->stopped = 1;
    pthread_cond_signal(&endpoint->requests.ready);
    pthread_cond_signal(&endpoint->replies.ready);
    pthread_mutex_unlock(lock);
}
