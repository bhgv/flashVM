/* Copyright (c) 2026 bhgv
 * EN: Real idle transport events, shared providers and unmount during native delivery.
 * RU: Реальные idle transport events, shared providers и unmount во время native delivery.
 */
#define _POSIX_C_SOURCE 200809L
#include "mount.h"
#include "notice.h"
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


/* EN/RU: External fixture owner keeps storage through detached callback release.
 * External fixture owner хранит storage до detached callback release. */
typedef struct Event
{
    pthread_mutex_t lock; /* Counts and fixture gate. */
    pthread_cond_t gate; /* Independent native completion. */
    unsigned count; /* Exactly-once delivery count. */
    unsigned released; /* Detached callback retirement. */
    int blocked; /* Test-only callback lifetime gate. */
    DVM_ChangeEvent value; /* Copied immutable native payload. */
    StyxClient *client; /* Borrowed monitor ref, held until callback returns. */
} Event;


/* EN/RU: Active fixture check, no assert or build-mode elision.
 * Активная fixture проверка без assert/build-mode elision. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "native events: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Initialize external event owner and optional test-only delivery gate.
 * Создать external event owner и optional test-only delivery gate. */
static void init(Event *event, StyxClient *client, int blocked)
{
    *event = (Event){0};
    event->client = client;
    event->blocked = blocked;
    check(!pthread_mutex_init(&event->lock, NULL), "event lock");
    check(!pthread_cond_init(&event->gate, NULL), "event gate");
}


/* EN/RU: Copy payload; local liveness query proves callbacks run outside client/engine locks.
 * Копировать payload; local liveness query проверяет отсутствие client/engine locks.
 * Only this fixture deliberately blocks to expose unmount during admitted delivery.
 * Только fixture блокирует callback для проверки unmount во время delivery. */
static void receive(void *opaque, const DVM_ChangeEvent *value)
{
    Event *event = opaque;
    check(!styxclientalive(event->client), "callback saw live failed source");
    pthread_mutex_lock(&event->lock);
    event->value = *value;
    event->count++;
    pthread_cond_broadcast(&event->gate);
    while (event->blocked)
    {
        pthread_cond_wait(&event->gate, &event->lock);
    }
    pthread_mutex_unlock(&event->lock);
}


/* EN/RU: Signal final detached tap release; external fixture retains its own storage.
 * Сигнализировать final detached tap release; storage удерживает fixture. */
static void release(void *opaque)
{
    Event *event = opaque;
    pthread_mutex_lock(&event->lock);
    event->released++;
    pthread_cond_broadcast(&event->gate);
    pthread_mutex_unlock(&event->lock);
}


/* EN/RU: Bounded native fixture wait for delivery or retirement; no guest/RPC progress.
 * Bounded native fixture wait delivery/retirement; без guest/RPC progress. */
static void waitfor(Event *event, int retired)
{
    struct timespec deadline;
    check(!clock_gettime(CLOCK_REALTIME, &deadline), "fixture clock");
    deadline.tv_sec += 4;
    pthread_mutex_lock(&event->lock);
    while (!(retired ? event->released : event->count))
    {
        check(!pthread_cond_timedwait(&event->gate, &event->lock, &deadline), "event deadline");
    }
    pthread_mutex_unlock(&event->lock);
}


/* EN/RU: Consume one tap and wait only in fixture for its independently retained callback.
 * Поглотить tap и ждать только в fixture независимо удержанный callback. */
static void finish(Event *event, DVM_NoticeTap *tap)
{
    dvm_notice_unsubscribe(tap);
    pthread_mutex_lock(&event->lock);
    event->blocked = 0;
    pthread_cond_broadcast(&event->gate);
    pthread_mutex_unlock(&event->lock);
    waitfor(event, 1);
    check(event->released == 1, "tap double release");
    pthread_cond_destroy(&event->gate);
    pthread_mutex_destroy(&event->lock);
}


/* EN/RU: Connect one native stream; mountfd consumes fd even on failure.
 * Подключить native stream; mountfd всегда поглощает fd. */
static StyxClient *connectclient(int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    check(fd >= 0 && !connect(fd, (struct sockaddr *)&address, sizeof(address)), "connect");
    StyxClient *client = styxclientmountfd(fd, 1024, "alice", "", 500, 8);
    check(client != NULL, "mountfd");
    return client;
}


/* EN/RU: Retain monitor-owned client once for a factory; each factory owns its journal.
 * Удержать monitor-owned client для factory; journal у каждой factory свой. */
static DVM_Provider *provider(StyxClient *client)
{
    check(styxclientretain(client), "client retain");
    DVM_Provider *result = styxmountprovider(client, FVM_ACCESS_MASK);
    check(result != NULL, "provider");
    return result;
}


/* EN/RU: Publish readiness before any remote close; stdin is only an external fixture gate.
 * Опубликовать readiness до remote close; stdin только external fixture gate. */
static void checkpoint(void)
{
    puts("ready idle");
    fflush(stdout);
    check(getchar() == 'g', "driver gate");
}


/* EN/RU: Validate idle EOF, local shutdown, quiet drop, late replay and independent journals.
 * Проверить idle EOF, local shutdown, quiet drop, late replay и independent journals. */
int main(int argc, char **argv)
{
    check(argc == 3, "port/mode");
    const char *mode = argv[2];
    int shared = !strcmp(mode, "shared") || !strcmp(mode, "shared-detach");
    int dropped = !strcmp(mode, "drop");
    int detached = !strcmp(mode, "detached") || !strcmp(mode, "shared-detach");
    int inflight = !strcmp(mode, "inflight");
    int late = !strcmp(mode, "late");
    int local = !strcmp(mode, "shutdown");
    StyxClient *client = connectclient(atoi(argv[1]));
    DVM_Provider *first = provider(client);
    DVM_Provider *second = shared ? provider(client) : NULL;
    Event a, b;
    init(&a, client, inflight);
    init(&b, client, 0);
    DVM_NoticeTap *tap = dvm_notice_subscribe(dvm_changes_notices(dvm_provider_changes(first)), receive, &a, release);
    DVM_NoticeTap *other = second ? dvm_notice_subscribe(dvm_changes_notices(dvm_provider_changes(second)), receive, &b, release) : NULL;
    check(tap && (!second || other), "journal taps");
    DVM_Changes *retained = NULL;
    if (detached || dropped)
    {
        retained = dvm_provider_changes(first);
        check(changesretain(retained), "independent journal retain");
        dvm_provider_release(first);
        first = NULL;
    }
    if (dropped)
    {
        styxclientfree(client);
        client = NULL;
    }
    checkpoint();
    if (local || late)
    {
        styxclientshutdown(client);
    }
    if (!dropped && !detached)
    {
        waitfor(&a, 0);
        if (other)
        {
            waitfor(&b, 0);
        }
        unsigned kinds = DVM_CHANGE_CONNECTION | ((local || late) ? 0 : DVM_CHANGE_NETWORK);
        check(a.count == 1 && a.value.session == 1 && a.value.generation == 2 && a.value.kinds == kinds, "terminal payload");
        if (shared)
        {
            check(b.count == 1 && b.value.session == a.value.session && b.value.generation == a.value.generation && b.value.kinds == kinds, "shared journal delivery");
        }
        if (inflight)
        {
            dvm_provider_release(first);
            first = NULL;
            check(!a.released, "callback state retired while in flight");
        }
        if (late)
        {
            DVM_Provider *after = provider(client);
            DVM_ChangeCursor cursor = {0};
            DVM_ChangeBatch batch;
            check(dvm_changes_pending(dvm_provider_changes(after), &cursor, DVM_CHANGE_ALL, &batch) == 1 && batch.event.generation == 2, "late journal replay");
            dvm_provider_release(after);
        }
    }
    else
    {
        check(!a.count, "retired provider notified");
    }
    if (other && detached)
    {
        waitfor(&b, 0);
        check(b.count == 1 && b.value.generation == 2 && b.value.kinds == (DVM_CHANGE_CONNECTION|DVM_CHANGE_NETWORK), "one detached factory disabled another");
    }
    finish(&a, tap);
    if (other)
    {
        finish(&b, other);
    }
    else
    {
        pthread_cond_destroy(&b.gate);
        pthread_mutex_destroy(&b.lock);
    }
    dvm_provider_release(first);
    dvm_provider_release(second);
    dvm_changes_destroy(retained);
    if (client)
    {
        styxclientshutdown(client);
        styxclientfree(client);
    }
    printf("PASS native idle event: %s\n", mode);
    return 0;
}
