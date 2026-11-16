/* Copyright (c) 2026 bhgv
 * EN: Real native heartbeat transitions, pending I/O and policy reconfiguration gates.
 * RU: Реальные native heartbeat transitions, pending I/O и policy reconfiguration gates.
 */
#define _POSIX_C_SOURCE 200809L
#include "mount.h"
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


/* EN/RU: Fixture owner retains state/client until watch retirement and worker join.
 * Fixture owner удерживает state/client до watch retirement и worker join. */
typedef struct Test
{
    pthread_mutex_t lock; /* Observation synchronization. */
    pthread_cond_t gate; /* Native callback readiness. */
    StyxClient *client; /* Borrowed monitor pin through callback retirement. */
    StyxLinkEvent last; /* Latest copied source generation. */
    unsigned released; /* Both detached subscriptions retired. */
    unsigned notices; /* Provider journal notifications. */
    unsigned suspect; /* First missed-window notification count. */
    unsigned recovered; /* Recovery notification count. */
    StyxFid *fid; /* Borrowed independently pinned read fid. */
    StyxTransfer transfer; /* Worker-owned until join. */
    char bytes[16]; /* Worker-owned read buffer. */
} Test;


/* EN/RU: Always-active fixture check; invalid exits without relying on assert.
 * Постоянная fixture check; invalid завершает без assert. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "heartbeat: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Copy one observation under fixture lock; generations reject stale replay.
 * Копировать observation под fixture lock; generations отклоняют stale replay. */
static void observed(void *opaque, const StyxLinkEvent *event)
{
    Test *test = opaque;
    StyxLinkEvent latest;
    check(styxclientlink(test->client, &latest) && latest.generation >= event->generation, "observation under source lock/stale snapshot");
    pthread_mutex_lock(&test->lock);
    if (event->generation > test->last.generation)
    {
        if (event->state == STYX_LINK_SUSPECT)
        {
            test->suspect++;
        }
        if (event->state == STYX_LINK_READY && test->last.state == STYX_LINK_SUSPECT)
        {
            test->recovered++;
        }
        test->last = *event;
    }
    pthread_cond_broadcast(&test->gate);
    pthread_mutex_unlock(&test->lock);
}


/* EN/RU: Count source-to-provider delivery without I/O or guest execution.
 * Считать source-to-provider delivery без I/O/guest execution. */
static void changed(void *opaque, const DVM_ChangeEvent *event)
{
    Test *test = opaque;
    check(event->generation >= 2 && event->kinds == (DVM_CHANGE_CONNECTION|DVM_CHANGE_NETWORK), "provider event classes");
    pthread_mutex_lock(&test->lock);
    test->notices++;
    pthread_cond_broadcast(&test->gate);
    pthread_mutex_unlock(&test->lock);
}


/* EN/RU: Signal detached callback retirement before fixture state can be destroyed.
 * Сигнализировать detached callback retirement до освобождения fixture state. */
static void released(void *opaque)
{
    Test *test = opaque;
    pthread_mutex_lock(&test->lock);
    test->released++;
    pthread_cond_broadcast(&test->gate);
    pthread_mutex_unlock(&test->lock);
}


/* EN/RU: Await native observation/journal only, never drive protocol progress from guest.
 * Ожидать native observation/journal; guest не обеспечивает protocol progress. */
static void waitstate(Test *test, int state, unsigned notices)
{
    struct timespec until;
    check(!clock_gettime(CLOCK_REALTIME, &until), "clock");
    until.tv_sec += 5;
    pthread_mutex_lock(&test->lock);
    while (test->last.state != state || test->notices < notices)
    {
        check(!pthread_cond_timedwait(&test->gate, &test->lock, &until), "observation deadline");
    }
    pthread_mutex_unlock(&test->lock);
}


/* EN/RU: Native read can remain pending while the same connection answers heartbeat.
 * Native read может ждать, пока то же соединение отвечает heartbeat. */
static void *reading(void *opaque)
{
    Test *test = opaque;
    styxclientread(test->fid, 0, test->bytes, sizeof(test->bytes), &test->transfer);
    return NULL;
}


/* EN/RU: Publish a deterministic external relay checkpoint, no clock-based fixture guesses.
 * Опубликовать external relay checkpoint, без clock-based fixture guesses. */
static void checkpoint(const char *text, int wait)
{
    puts(text);
    fflush(stdout);
    if (wait)
    {
        check(getchar() == 'g', "relay gate");
    }
}


/* EN/RU: Connect once; native fd owner consumes the stream, monitor keeps a client pin.
 * Подключиться один раз; native owner принимает stream, monitor удерживает client. */
static StyxClient *connectclient(int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    check(fd >= 0 && !connect(fd, (struct sockaddr *)&address, sizeof(address)), "connect");
    StyxClient *client = styxclientmountfd(fd, 1024, "alice", "", 3000, 8);
    check(client != NULL, "mountfd");
    return client;
}


/* EN/RU: Validate optional standard9P heartbeat and native provider event routing.
 * Проверить optional standard9P heartbeat и native provider event routing. */
int main(int argc, char **argv)
{
    check(argc == 3, "port/mode");
    const char *mode = argv[2];
    Test test = {0};
    check(!pthread_mutex_init(&test.lock, NULL) && !pthread_cond_init(&test.gate, NULL), "fixture locks");
    StyxClient *client = connectclient(atoi(argv[1]));
    test.client = client;
    check(styxclientretain(client), "monitor pin");
    DVM_Provider *provider = styxmountprovider(client, FVM_ACCESS_MASK);
    check(provider != NULL, "provider");
    StyxWatch *watch = styxclientobserve(client, observed, &test, released);
    DVM_NoticeTap *tap = dvm_notice_subscribe(dvm_changes_notices(dvm_provider_changes(provider)), changed, &test, released);
    check(watch && tap && test.last.state == STYX_LINK_READY, "native subscriptions");
    check(!styxclientkeepalive(NULL, 1, 1, 2) && !styxclientkeepalive(client, 0, 1, 2) &&
        !styxclientkeepalive(client, 1, 0, 2) && !styxclientkeepalive(client, 1, 1, 1) &&
        !styxclientkeepalive(client, 120001, 1, 2) && !styxclientkeepalive(client, 1, 1, 17), "invalid policy accepted");
    int pending = !strcmp(mode, "pending");
    pthread_t worker;
    if (pending)
    {
        test.fid = styxclientwalk(client, "first");
        check(test.fid && styxclientopen(test.fid, 0), "pending fid");
        check(!pthread_create(&worker, NULL, reading, &test), "pending native read");
    }
    check(styxclientkeepalive(client, 40, 180, 3), "enable policy");
    checkpoint("ready heartbeat", 0);
    int recover = !strcmp(mode, "recover") || !strcmp(mode, "traffic");
    int timeout = !strcmp(mode, "timeout") || !strcmp(mode, "disable") || !strcmp(mode, "reconfigure");
    if (!strcmp(mode, "disable") || !strcmp(mode, "reconfigure"))
    {
        check(getchar() == 'g', "probe admitted gate");
        check(!strcmp(mode, "disable") ? styxclientkeepalive(client, 0, 0, 0) : styxclientkeepalive(client, 10000, 10000, 16), "change admitted policy");
        checkpoint("policy changed", 0);
    }
    if (recover)
    {
        waitstate(&test, STYX_LINK_SUSPECT, 1);
        checkpoint("suspect", !strcmp(mode, "traffic"));
        if (!strcmp(mode, "traffic"))
        {
            FVM_FileInfo meta;
            check(dvm_provider_info(provider, ".", &meta) == FVM_OK, "ordinary traffic recovery");
        }
        waitstate(&test, STYX_LINK_READY, 2);
        check(test.suspect == 1 && test.recovered == 1, "duplicate transition");
        checkpoint("recovered", 1);
    }
    else if (timeout || !strcmp(mode, "badreply"))
    {
        waitstate(&test, timeout ? STYX_LINK_TIMEOUT : STYX_LINK_FAILED, timeout ? 2 : 1);
        check(!styxclientalive(client), "timeout retained live connection");
        check(!styxclientkeepalive(client, 1, 1, 2), "terminal reconfiguration");
    }
    else
    {
        check(getchar() == 'g', "healthy probes gate");
        check(styxclientalive(client), "healthy peer failed");
    }
    StyxLinkEvent snapshot;
    check(styxclientlink(client, &snapshot), "snapshot");
    if (!timeout && strcmp(mode, "badreply"))
    {
        check(styxclientkeepalive(client, 0, 0, 0), "disable future probes");
    }
    if (pending)
    {
        checkpoint("release read", 0);
        check(!pthread_join(worker, NULL), "read join");
        check(test.transfer.success && test.transfer.count == 7 && !memcmp(test.bytes, "initial", 7), "pending read changed by probe");
        styxclientclunk(test.fid);
    }
    dvm_notice_unsubscribe(tap);
    styxclientunwatch(watch);
    pthread_mutex_lock(&test.lock);
    while (test.released != 2)
    {
        pthread_cond_wait(&test.gate, &test.lock);
    }
    pthread_mutex_unlock(&test.lock);
    dvm_provider_release(provider);
    styxclientshutdown(client);
    styxclientfree(client);
    pthread_cond_destroy(&test.gate);
    pthread_mutex_destroy(&test.lock);
    printf("PASS native heartbeat: %s\n", mode);
    return 0;
}
