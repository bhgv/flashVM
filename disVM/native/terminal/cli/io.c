/* Copyright (c) 2026 bhgv
 * EN: Application-only S-record stdio, cancellation-aware partial I/O and independent per-tag native consumers.
 * RU: Клиентские S-записи stdio, частичный I/O с отменой и независимые native-потребители по тегам.
 */
#include "session.h"
#include "flashvm/sstream.h"
#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include <unistd.h>


/* EN/RU: Copy lifecycle stop flag under native state lock, no I/O.
 * Скопировать stop-флаг под native state mutex, без I/O.
 */
static int stopped(UnibusCliSession *session)
{
    pthread_mutex_lock(&session->lock);
    int result = session->stopping; /* Coherent lifecycle snapshot. */
    pthread_mutex_unlock(&session->lock);
    return result;
}


/* EN/RU: Poll one stdio descriptor plus an independent native wake;1 descriptor ready/0 cancellation/-1 error.
 * Ожидать stdio и независимое native-пробуждение:1 готово/0 отмена/-1 ошибка.
 */
static int waitfd(int fd, short events, int wake)
{
    struct pollfd descriptors[2] = {{fd, events, 0}, {wake, POLLIN, 0}}; /* Data and nonconsuming level-triggered stop. */
    for (;;)
    {
        int result = poll(descriptors, 2, -1); /* Native wait only, never broker or guest servicing. */
        if (result < 0 && errno == EINTR)
        {
            continue;
        }
        if (result < 0 || (descriptors[0].revents & POLLNVAL))
        {
            return -1;
        }
        if (descriptors[1].revents)
        {
            return 0;
        }
        return 1;
    }
}


/* EN/RU: Native stdin producer decodes ordinary application records and never interprets transport root commands.
 * Native stdin-производитель читает клиентские записи и не разбирает transport root-команды.
 */
void *unibuscli_input(void *argument)
{
    UnibusCliSession *session = argument; /* Borrowed through join. */
    unsigned char *storage = malloc(session->config->limit + 32); /* Original application S-stream storage. */
    FVM_SStream decoder; /* Incremental ordinary application decoder, no transport-prefix access. */
    if (!storage || fvm_sstream_init(&decoder, storage, session->config->limit + 32, session->config->limit) != FVM_S_OK)
    {
        free(storage);
        unibuscli_stop(session, 2);
        return NULL;
    }
    while (!stopped(session))
    {
        unsigned char bytes[1024]; /* One native stdin fragment. */
        int ready = waitfd(STDIN_FILENO, POLLIN, session->input_wake[0]); /* Separate from output drain wake. */
        if (ready <= 0)
        {
            if (ready < 0)
            {
                unibuscli_stop(session, 2);
            }
            break;
        }
        ssize_t count = read(STDIN_FILENO, bytes, sizeof(bytes)); /* No guest work or implicit frame replay. */
        if (count < 0 && (errno == EINTR || errno == EAGAIN))
        {
            continue;
        }
        if (count <= 0)
        {
            if (count < 0 || decoder.filled)
            {
                unibuscli_stop(session, 2);
            }
            break; /* Clean stdin EOF retires this producer only; RX remains active. */
        }
        for (size_t at = 0; at < (size_t)count;)
        {
            FVM_SRecord record; /* Borrowed original application view until reset. */
            size_t used; /* Exactly consumed fragment bytes. */
            FVM_SResult result = fvm_sstream_push(&decoder, bytes + at, (size_t)count - at, &record, &used); /* One complete record maximum. */
            at += used;
            if ((result != FVM_S_MORE && result != FVM_S_OK) || !used)
            {
                unibuscli_stop(session, 2);
                break;
            }
            if (result == FVM_S_OK)
            {
                StyxOrderedClient *client = styxorderedclient(session->peer, record.tag); /* Explicit predeclared grant only. */
                if (!client || styxorderedclientsendwait(client, record.payload, record.size, 120000) != STYX_ORDERED_SENT)
                {
                    if (!stopped(session))
                    {
                        unibuscli_stop(session, 2);
                    }
                    break;
                }
                fvm_sstream_reset(&decoder);
            }
        }
    }
    free(storage);
    return NULL;
}


/* EN/RU: Serialize one complete application record to stdout, retaining its exact suffix through short writes.
 * Сериализовать одну полную клиентскую запись в stdout, сохраняя suffix при частичной записи.
 */
static int publish(UnibusCliReader *reader, unsigned char *encoded, size_t size)
{
    UnibusCliSession *session = reader->session; /* Borrowed owner through output worker. */
    pthread_mutex_lock(&session->output);
    size_t sent = 0; /* Exact stdout prefix, never restart a partly written record. */
    int valid = !stopped(session); /* Quota already reached by another output worker means no extra record. */
    while (valid && sent < size)
    {
        if (waitfd(STDOUT_FILENO, POLLOUT, session->wake[0]) <= 0)
        {
            valid = 0;
            break;
        }
        ssize_t count = write(STDOUT_FILENO, encoded + sent, size - sent); /* Whole-record serialization lock only. */
        if (count < 0 && (errno == EINTR || errno == EAGAIN))
        {
            continue;
        }
        if (count <= 0)
        {
            valid = 0;
            break;
        }
        sent += (size_t)count;
    }
    if (valid)
    {
        pthread_mutex_lock(&session->lock);
        int overflow = session->received == (size_t)-1; /* Never silently wrap a process lifetime/quota counter. */
        if (!overflow)
        {
            session->received++;
        }
        int complete = session->config->records && session->received >= session->config->records; /* Explicit owner quota only. */
        pthread_mutex_unlock(&session->lock);
        if (overflow || complete)
        {
            unibuscli_stop(session, overflow ? 2 : 0);
        }
    }
    pthread_mutex_unlock(&session->output);
    return valid;
}


/* EN/RU: See session.h; ordinal stays private, stdout contains only authorized inner tag/payload records.
 * См. session.h; номер остаётся закрытым, stdout содержит лишь разрешённые внутренние tag/payload.
 */
void *unibuscli_output(void *argument)
{
    UnibusCliReader *reader = argument; /* Borrowed fixed client until join. */
    UnibusCliSession *session = reader->session; /* Stable lifecycle owner. */
    unsigned char *bytes = malloc(session->config->limit); /* One copied client payload. */
    unsigned char *encoded = malloc(session->config->limit + 32); /* Complete application S-record, no transport prefix. */
    if (!bytes || !encoded)
    {
        unibuscli_stop(session, 2);
    }
    while (bytes && encoded && !stopped(session))
    {
        unsigned long long ordinal; /* Transport-only checkpoint, never serialized to application stdout. */
        size_t size; /* Exact client payload extent. */
        int result = styxorderedclientnext(reader->client, &ordinal, bytes, session->config->limit, &size); /* One copied record. */
        if (result == STYX_ORDER_EMPTY)
        {
            styxorderedclientwait(reader->client, 120000);
            continue;
        }
        if (result == STYX_ORDER_END)
        {
            break;
        }
        size_t used; /* Exact encoded application record. */
        if (result != STYX_ORDER_RECORD || fvm_srecord_write(reader->tag, bytes, size, encoded, session->config->limit + 32, &used) != FVM_S_OK ||
            !publish(reader, encoded, used))
        {
            if (!stopped(session))
            {
                unibuscli_stop(session, 2);
            }
            break;
        }
    }
    free(encoded);
    free(bytes);
    pthread_mutex_lock(&session->lock);
    session->finished++;
    pthread_cond_broadcast(&session->changed);
    pthread_mutex_unlock(&session->lock);
    return NULL;
}
