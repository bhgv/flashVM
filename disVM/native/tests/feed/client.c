/* Copyright (c) 2026 bhgv
 * EN: Actual native mount feed with independent health/event generations and nonwaiting detach.
 * RU: Настоящий native mount feed с independent health/event generations и nonwaiting detach.
 */
#define _POSIX_C_SOURCE 200809L
#include "mount.h"
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/* EN/RU: External fixture storage lives until both policy and notification callbacks retire.
 * External fixture storage живёт до retirement policy и notification callbacks. */
typedef struct Test
{
    pthread_mutex_t lock; /* Fixture status counters and native notification gate. */
    pthread_cond_t changed; /* No guest poll supplies progress. */
    uint64_t local; /* Highest observed local journal generation. */
    unsigned notices; /* Copied callback count. */
    int entered; /* Test-only decoder admission gate. */
    int proceed; /* Test-only decoder return gate. */
    unsigned released; /* Feed decoder context retired. */
    unsigned tapreleased; /* Notification recipient retired. */
    const char *mode; /* Immutable fixture decoder behavior. */
} Test;


/* EN/RU: Keep fixture checks active in every native build mode.
 * Fixture checks активны во всех native build modes. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "native remote feed: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Decode fixture-only text incrementally; no production format is selected by library.
 * Decode fixture-only text incremental; library не выбирает production format. */
static int decode(void *opaque, const void *bytes, size_t size, StyxEvent *event, size_t *consumed)
{
    Test *test = opaque;
    if (!strcmp(test->mode, "partial"))
    {
        return 0;
    }
    const char *end = memchr(bytes, '\n', size);
    if (!end)
    {
        return 0;
    }
    size_t length = (size_t)(end-(const char *)bytes)+1;
    if (length >= 128)
    {
        return -1;
    }
    char text[128];
    memcpy(text, bytes, length);
    text[length] = 0;
    event->session = 1;
    int used = 0;
    if (sscanf(text, "event %llu %u %u %llu\n%n", &event->generation, &event->kinds,
               &event->flags, &event->resource, &used) != 4 || (size_t)used != length)
    {
        return -1;
    }
    if (!strcmp(test->mode, "inflight"))
    {
        pthread_mutex_lock(&test->lock);
        test->entered = 1;
        pthread_cond_broadcast(&test->changed);
        while (!test->proceed)
        {
            pthread_cond_wait(&test->changed, &test->lock);
        }
        pthread_mutex_unlock(&test->lock);
    }
    *consumed = length;
    if (!strcmp(test->mode, "bad"))
    {
        event->kinds = 128;
    }
    if (!strcmp(test->mode, "progress"))
    {
        *consumed = 0;
    }
    return 1;
}


/* EN/RU: Record one rebased mount invalidation without guest execution or source I/O.
 * Записать одну rebased mount invalidation без guest execution/source I/O. */
static void noticed(void *opaque, const DVM_ChangeEvent *event)
{
    Test *test = opaque;
    pthread_mutex_lock(&test->lock);
    if (event->generation > test->local)
    {
        test->local = event->generation;
    }
    test->notices++;
    pthread_cond_broadcast(&test->changed);
    pthread_mutex_unlock(&test->lock);
}


/* EN/RU: Signal feed decoder retirement only after worker and public owners release it.
 * Сигнализировать decoder retirement после worker и public owners. */
static void released(void *opaque)
{
    Test *test = opaque;
    pthread_mutex_lock(&test->lock);
    test->released++;
    pthread_cond_broadcast(&test->changed);
    pthread_mutex_unlock(&test->lock);
}


/* EN/RU: Detached notification tap can outlive unsubscribe; signal its final return.
 * Detached notification tap может пережить unsubscribe; сигнализировать final return. */
static void tapreleased(void *opaque)
{
    Test *test = opaque;
    pthread_mutex_lock(&test->lock);
    test->tapreleased++;
    pthread_cond_broadcast(&test->changed);
    pthread_mutex_unlock(&test->lock);
}


/* EN/RU: Bound fixture wait; callbacks/worker make progress independently of this native waiter.
 * Bounded fixture wait; callbacks/worker продвигаются независимо от native waiter. */
static void waitfor(Test *test, StyxFeed *feed, int state, uint64_t generation, int retire)
{
    struct timespec until;
    check(!clock_gettime(CLOCK_REALTIME, &until), "clock");
    until.tv_sec += 5;
    pthread_mutex_lock(&test->lock);
    for (;;)
    {
        StyxFeedInfo info = {0};
        if (feed)
        {
            check(styxfeedinfo(feed, &info), "feed snapshot");
        }
        if (retire ? test->released == 1 && test->tapreleased == 1 : info.state == state && test->local >= generation)
        {
            break;
        }
        check(!pthread_cond_timedwait(&test->changed, &test->lock, &until), "native progress deadline");
    }
    pthread_mutex_unlock(&test->lock);
}


/* EN/RU: Publish an external control checkpoint and optionally await the deterministic relay gate.
 * Опубликовать external checkpoint и опционально ждать relay gate. */
static void checkpoint(const char *text, int gate)
{
    puts(text);
    fflush(stdout);
    if (gate)
    {
        check(getchar() == 'g', "relay gate");
    }
}


/* EN/RU: Mount and verify automatic native event routing without guest callbacks.
 * Подмонтировать и проверить automatic native event routing без guest callbacks. */
int main(int argc, char **argv)
{
    check(argc == 3, "port/mode");
    Test test = {0};
    test.mode = argv[2];
    check(!pthread_mutex_init(&test.lock, NULL) && !pthread_cond_init(&test.changed, NULL), "fixture locks");
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(atoi(argv[1]));
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    check(fd >= 0 && !connect(fd, (struct sockaddr *)&address, sizeof(address)), "connect");
    StyxClient *client = styxclientmountfd(fd, 1024, "alice", "", 300, !strcmp(test.mode, "noslots") ? 1 : 8);
    check(client && styxclientretain(client) && styxclientkeepalive(client, 40, 500, 4), "native client/monitor");
    StyxFeedConfig config = {!strcmp(test.mode, "badpath") ? "../hidden" : "events", 128,
                             STYX_EVENT_CONTENT|STYX_EVENT_RIGHTS, 0, decode, &test, released};
    StyxFeed *feed = NULL;
    DVM_Provider *provider = styxmountproviderfeed(client, !strcmp(test.mode, "writeonly") ? FVM_ACCESS_WRITE : FVM_ACCESS_MASK, &config, &feed);
    if (!strcmp(test.mode, "badpath") || !strcmp(test.mode, "noslots") || !strcmp(test.mode, "writeonly"))
    {
        check(!provider && !feed && !test.released && styxclientalive(client), "failed construction adopted context/broke client");
        styxclientshutdown(client);
        styxclientfree(client);
        pthread_cond_destroy(&test.changed);
        pthread_mutex_destroy(&test.lock);
        puts("PASS native remote feed rejected setup");
        return 0;
    }
    check(provider && feed, "mount with automatic event feed");
    DVM_Changes *journal = dvm_provider_changes(provider);
    check(changesretain(journal), "independent fixture journal pin");
    DVM_NoticeTap *tap = dvm_notice_subscribe(dvm_changes_notices(journal), noticed, &test, tapreleased);
    check(tap != NULL, "journal receiver");
    DVM_ChangeCursor cursor = {0};
    DVM_ChangeBatch batch;
    check(dvm_changes_pending(journal, &cursor, DVM_CHANGE_ALL, &batch) == 1, "initial journal capture");
    pthread_mutex_lock(&test.lock);
    if (batch.event.generation > test.local)
    {
        test.local = batch.event.generation;
    }
    pthread_mutex_unlock(&test.lock);
    int bad = !strcmp(test.mode, "bad") || !strcmp(test.mode, "progress");
    checkpoint("ready feed", !bad);
    if (!strcmp(test.mode, "inflight"))
    {
        pthread_mutex_lock(&test.lock);
        check(test.entered, "decoder gate admission");
        pthread_mutex_unlock(&test.lock);
    }
    else if (bad)
    {
        waitfor(&test, feed, STYX_FEED_FAILED, 2, 0);
    }
    else
    {
        FVM_FileInfo meta;
        check(dvm_provider_info(provider, ".", &meta) == FVM_OK, "ordinary RPC blocked by long-lived read");
        check(dvm_changes_pending(journal, &cursor, DVM_CHANGE_ALL, &batch) == 1 &&
              dvm_changes_ack(journal, &cursor, &batch) == FVM_OK, "initial ack");
        checkpoint("watching", 0);
        if (!strcmp(test.mode, "stop"))
        {
            check(getchar() == 'g', "detach gate");
        }
        else if (!strcmp(test.mode, "partial"))
        {
            waitfor(&test, feed, STYX_FEED_FAILED, 2, 0);
        }
        else
        {
            waitfor(&test, feed, STYX_FEED_ACTIVE, 3, 0);
            check(dvm_changes_pending(journal, &cursor, DVM_CHANGE_ALL, &batch) == 1 &&
                  batch.event.kinds == STYX_EVENT_CONTENT, "remote content not routed precisely");
            checkpoint("received", 0);
            if (!strcmp(test.mode, "shutdown"))
            {
                styxclientshutdown(client);
                waitfor(&test, feed, STYX_FEED_FAILED, 5, 0);
            }
            else
            {
                waitfor(&test, feed, STYX_FEED_EOF, 4, 0);
            }
        }
    }
    dvm_notice_unsubscribe(tap);
    dvm_provider_release(provider);
    styxfeedfree(feed);
    if (!strcmp(test.mode, "stop") || !strcmp(test.mode, "inflight"))
    {
        pthread_mutex_lock(&test.lock);
        check(!test.released, "detach waited for held Rflush");
        pthread_mutex_unlock(&test.lock);
        checkpoint("detached", 0);
        if (!strcmp(test.mode, "inflight"))
        {
            pthread_mutex_lock(&test.lock);
            test.proceed = 1;
            pthread_cond_broadcast(&test.changed);
            pthread_mutex_unlock(&test.lock);
        }
    }
    waitfor(&test, NULL, 0, 0, 1);
    if (!strcmp(test.mode, "inflight"))
    {
        check(dvm_changes_pending(journal, &cursor, DVM_CHANGE_ALL, &batch) == 1 && batch.event.generation == 1, "stopped decoder published late data");
    }
    dvm_changes_destroy(journal);
    styxclientshutdown(client);
    styxclientfree(client);
    pthread_cond_destroy(&test.changed);
    pthread_mutex_destroy(&test.lock);
    printf("PASS native remote feed: %s\n", test.mode);
    return 0;
}
