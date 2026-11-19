/* Copyright (c) 2026 bhgv
 * EN: Independent per-client queue admission and backend execution. Failed client
 * replies also belong to its worker, never the transport RX owner.
 * RU: Независимые очереди/исполнение клиентов. Ответы с ошибками клиента также
 * принадлежат его worker, а не владельцу transport RX.
 * standalone-reviewed: sticky backend failure does not poison another client's queue.
 */
#include "bridge.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>


/* EN/RU: See bridge.h; allocation happens before locking and failure transfers nothing.
 * См. bridge.h; allocation до lock, при отказе владение не передаётся.
 */
int bridge_enqueue(Worker *worker, const void *bytes, size_t size, int invalid)
{
    if (!worker || size > BRIDGE_LIMIT || (!invalid && (!bytes || size < 7)))
    {
        return 0;
    }
    if (invalid)
    {
        size = 0; /* A malformed client message needs only an ordered error marker, not a copied bad frame. */
    }
    Packet *packet = malloc(sizeof(*packet) + size);
    if (!packet)
    {
        return 0; /* No arbitrary queue quota or silent loss; RX reports a terminal admission failure. */
    }
    packet->next = NULL;
    packet->size = size;
    packet->invalid = invalid;
    if (size)
    {
        memcpy(packet->bytes, bytes, size); /* The stream decoder can reuse its storage after this copy. */
    }
    pthread_mutex_lock(&worker->lock);
    int accepted = !worker->ending; /* A failed backend still has a worker to deliver private errors. */
    if (accepted)
    {
        if (worker->tail)
        {
            worker->tail->next = packet;
        }
        else
        {
            worker->head = packet;
        }
        worker->tail = packet;
        pthread_cond_signal(&worker->ready);
    }
    pthread_mutex_unlock(&worker->lock);
    if (!accepted)
    {
        free(packet); /* Caller never acquires ownership of an unaccepted copy. */
    }
    return accepted;
}


/* EN: Mark this worker's output terminal and notify borrowed pipe writer.
 * Main retains the pipe through join; EAGAIN means a wake byte is already pending.
 * Return nothing; this helper never waits for a client or tries to replay stdout.
 * RU: Пометить вывод worker отказавшим и уведомить через pipe writer.
 * Main удерживает pipe до join; EAGAIN означает уже ожидающее уведомление.
 * Возврата нет; без ожидания клиента и повтора stdout.
 */
static void output_failed(Worker *worker)
{
    pthread_mutex_lock(&worker->lock);
    worker->output_failed = 1;
    worker->aborting = 1;
    worker->ending = 1;
    pthread_mutex_unlock(&worker->lock);
    unsigned char event = 1; /* Private process wakeup, not an Unibus client-visible protocol extension. */
    while (write(worker->notify, &event, 1) < 0 && errno == EINTR)
    {
        /* Only repeat the interrupted wake byte; do not allocate or wait for backend work. */
    }
}


/* EN/RU: See bridge.h; borrowed state/socket remain pinned until the owner joins.
 * См. bridge.h; заимствованные state/socket удерживаются до join владельцем.
 */
void *bridge_worker(void *state)
{
    Worker *worker = state;
    unsigned char reply[BRIDGE_LIMIT]; /* Only one outstanding RPC per client in this explicit teaching profile. */
    for (;;)
    {
        pthread_mutex_lock(&worker->lock);
        while (!worker->head && !worker->ending)
        {
            pthread_cond_wait(&worker->ready, &worker->lock); /* Wait only for this client's queue; release its mutex while asleep. */
        }
        if (worker->aborting || !worker->head)
        {
            pthread_mutex_unlock(&worker->lock);
            return NULL; /* Main reclaims any still-queued packets only after join. */
        }
        Packet *packet = worker->head;
        worker->head = packet->next;
        if (!worker->head)
        {
            worker->tail = NULL;
        }
        int failed = worker->failed;
        pthread_mutex_unlock(&worker->lock); /* No queue lock spans socket I/O or physical output. */
        int success = 0;
        unsigned length = 0;
        if (!failed && !packet->invalid)
        {
            success = bridge_exact(worker->socket, packet->bytes, packet->size, 1) &&
                bridge_exact(worker->socket, reply, 4, 0); /* Partial backend progress is never re-encoded/replayed. */
            length = success ? bridge_length(reply) : 0;
            success = success && length >= 7 && length <= sizeof(reply) &&
                bridge_exact(worker->socket, reply + 4, length - 4, 0);
            if (!success)
            {
                pthread_mutex_lock(&worker->lock);
                worker->failed = 1; /* No later packet on this connection can cause another backend effect. */
                pthread_mutex_unlock(&worker->lock);
            }
        }
        int written;
        if (packet->invalid)
        {
            written = bridge_reply(worker->tag, 2, "invalid client frame", sizeof("invalid client frame") - 1);
        }
        else if (success)
        {
            written = bridge_reply(worker->tag, 1, reply, length);
        }
        else
        {
            written = bridge_reply(worker->tag, 2, "backend connection failed", sizeof("backend connection failed") - 1);
        }
        free(packet); /* Its copied request is no longer referenced by either queue or backend transfer. */
        if (!written)
        {
            output_failed(worker); /* Wake a broker blocked on RX; other worker state stays owned until join. */
            return NULL;
        }
    }
}
