/* Copyright (c) 2026 bhgv
 * EN: Dispatch complete execute requests to the exact Limbo owner fid.
 * RU: Передавать полные запросы исполнения точному fid владельца Limbo.
 */
#include "exec_private.h"
#include "styxrpc/named_channel.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>


/* EN: Encode a local mailbox count in big-endian order.
 * RU: Записать счётчик локальной очереди старшим байтом вперёд. */
static void put(unsigned char *bytes, uint32_t value)
{
    for (size_t i = DVM_EXEC_WORD; i; --i)
    {
        bytes[i - 1] = (unsigned char)value;
        value >>= 8;
    }
}


/* EN: Read a full local mailbox word in big-endian order.
 * RU: Прочитать целое слово локальной очереди старшим байтом вперёд. */
static uint32_t get(const unsigned char *bytes)
{
    uint32_t value = 0;
    for (size_t i = 0; i < DVM_EXEC_WORD; ++i)
    {
        value = (value << 8) | bytes[i];
    }
    return value;
}


/* EN: Find one active exact spelling while holding the broker lock.
 * RU: Найти одно активное точное имя под блокировкой брокера. */
static ExecOwner *find(DvmExecBroker *broker, const char *absolute, uint64_t layer)
{
    for (ExecOwner *owner = broker->owners; owner; owner = owner->next)
    {
        if ((!layer || owner->layer == layer) && !strcmp(owner->path, absolute))
        {
            return owner;
        }
    }
    return NULL;
}


/* EN/RU: See exec_private.h. / См. exec_private.h. */
int dvm_exec_invoke(void *context, const char *path, void *bytes, size_t size, size_t *used)
{
    DvmExecBroker *broker = context;
    char absolute[STYXRPC_NODE_PATH_MAX + 2];
    absolute[0] = '/';
    strcpy(absolute + 1, path);
    if (!styxrpc_named_path(absolute))
    {
        return 0;
    }
    pthread_mutex_lock(&broker->lock);
    int published = !broker->stopped && find(broker, absolute, 0);
    pthread_mutex_unlock(&broker->lock);
    uint64_t layer = published ? dvm_exec_layer(broker, absolute) : 0;
    if (!layer)
    {
        return 0;
    }
    struct timespec deadline;
    if (clock_gettime(CLOCK_MONOTONIC, &deadline))
    {
        return 0;
    }
    deadline.tv_sec += DVM_EXEC_DEADLINE_SECONDS;
    pthread_mutex_lock(&broker->lock);
    ExecOwner *owner = find(broker, absolute, layer);
    if (!owner || owner->job || broker->stopped || broker->ticket == UINT32_MAX)
    {
        pthread_mutex_unlock(&broker->lock);
        return 0;
    }
    ExecJob job = {.owner = owner, .bytes = bytes, .size = size, .ticket = ++broker->ticket};
    owner->job = &job;
    while (!job.finished && !broker->stopped)
    {
        if (pthread_cond_timedwait(&broker->changed, &broker->lock, &deadline))
        {
            break;
        }
    }
    if (job.owner)
    {
        job.owner->job = NULL;
    }
    *used = job.used;
    pthread_mutex_unlock(&broker->lock);
    return job.accepted;
}


/* EN/RU: See exec_private.h. / См. exec_private.h. */
FVM_Result dvm_exec_receive(void *state, void *buffer, size_t size, size_t *used)
{
    ExecOwner *owner = state;
    DvmExecBroker *broker = owner->broker;
    unsigned char *bytes = buffer;
    FVM_Result status = FVM_OK;
    *used = 0;
    pthread_mutex_lock(&broker->lock);
    ExecJob *job = owner->job;
    if (!owner->layer || broker->stopped)
    {
        status = FVM_ERROR;
    }
    else if (job && !job->taken)
    {
        if (size < DVM_EXEC_HEADER + job->size)
        {
            status = FVM_INVALID_ARGUMENT;
        }
        else
        {
            put(bytes, job->ticket);
            put(bytes + DVM_EXEC_WORD, (uint32_t)job->size);
            memcpy(bytes + DVM_EXEC_HEADER, job->bytes, job->size);
            *used = DVM_EXEC_HEADER + job->size;
            job->taken = 1;
        }
    }
    pthread_mutex_unlock(&broker->lock);
    return status;
}


/* EN: Publish a registration from one complete counted local write.
 * RU: Опубликовать регистрацию из одной полной локальной записи с длиной. */
static FVM_Result register_owner(ExecOwner *owner, const void *bytes, size_t size)
{
    char path[STYXRPC_NODE_PATH_MAX + 2];
    if (!size || size >= sizeof(path) || memchr(bytes, 0, size))
    {
        return FVM_INVALID_ARGUMENT;
    }
    memcpy(path, bytes, size);
    path[size] = 0;
    DvmExecBroker *broker = owner->broker;
    if (!dvm_exec_claim(owner, path))
    {
        return FVM_ERROR;
    }
    pthread_mutex_lock(&broker->lock);
    if (broker->stopped)
    {
        pthread_mutex_unlock(&broker->lock);
        return FVM_ERROR;
    }
    owner->next = broker->owners;
    broker->owners = owner;
    pthread_mutex_unlock(&broker->lock);
    return FVM_OK;
}


/* EN/RU: See exec_private.h. / См. exec_private.h. */
FVM_Result dvm_exec_answer(void *state, const void *buffer, size_t size)
{
    ExecOwner *owner = state;
    if (!owner->layer)
    {
        return register_owner(owner, buffer, size);
    }
    if (size < DVM_EXEC_HEADER)
    {
        return FVM_INVALID_ARGUMENT;
    }
    const unsigned char *bytes = buffer;
    uint32_t ticket = get(bytes);
    uint32_t count = get(bytes + DVM_EXEC_WORD);
    size_t body = size - DVM_EXEC_HEADER;
    if (body != (count == DVM_EXEC_REFUSE ? 0 : count))
    {
        return FVM_INVALID_ARGUMENT;
    }
    DvmExecBroker *broker = owner->broker;
    pthread_mutex_lock(&broker->lock);
    ExecJob *job = owner->job;
    if (!job || !job->taken || job->finished || job->ticket != ticket || body > job->size || broker->stopped)
    {
        pthread_mutex_unlock(&broker->lock);
        return FVM_NOT_FOUND;
    }
    memcpy(job->bytes, bytes + DVM_EXEC_HEADER, body);
    job->used = body;
    job->accepted = count != DVM_EXEC_REFUSE;
    job->finished = 1;
    pthread_cond_broadcast(&broker->changed);
    pthread_mutex_unlock(&broker->lock);
    return FVM_OK;
}


/* EN/RU: See exec_private.h. / См. exec_private.h. */
void dvm_exec_close_owner(void *state)
{
    ExecOwner *owner = state;
    DvmExecBroker *broker = owner->broker;
    if (owner->layer)
    {
        pthread_mutex_lock(&broker->lock);
        ExecOwner **link = &broker->owners;
        while (*link && *link != owner)
        {
            link = &(*link)->next;
        }
        if (*link)
        {
            *link = owner->next;
        }
        if (owner->job)
        {
            owner->job->owner = NULL;
            owner->job->finished = 1;
            pthread_cond_broadcast(&broker->changed);
        }
        int unclaim = !broker->stopped && dvm_namespace_retain(broker->group);
        pthread_mutex_unlock(&broker->lock);
        if (unclaim)
        {
            dvm_exec_unclaim(owner);
            dvm_namespace_release(broker->group);
        }
    }
    free(owner);
}
