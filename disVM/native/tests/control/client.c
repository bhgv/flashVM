/* Copyright (c) 2026 bhgv
 * EN: Actual live mount subscription CAS, standby handoff and retained ordinary fid tests.
 * RU: Тесты CAS подписки live mount, standby handoff и сохранённого ordinary fid.
 */
#define _POSIX_C_SOURCE 200809L
#include "mount.h"
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/* EN/RU: Native fixture state outlives all decoder capabilities and worker cleanup.
 * Native fixture state живёт дольше всех decoder capabilities и worker cleanup. */
typedef struct Test
{
    pthread_mutex_t lock; /* Protect fixture decoder/release gates. */
    pthread_cond_t gate; /* Independent native waiters, no guest polling. */
    int released; /* Exactly-once adopted context retirement. */
    int hold; /* Fixture-only decoder admission barrier. */
    int entered; /* Decoder reached barrier. */
    int proceed; /* Release admitted decoder. */
} Test;


/* EN/RU: Always-active native fixture invariant; failure terminates subprocess.
 * Постоянный native fixture invariant; failure завершает subprocess. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "mount control: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Decode fixture text only; test gate deliberately simulates in-flight native policy.
 * Decode только fixture text; gate моделирует in-flight native policy. */
static int decode(void *opaque, const void *bytes, size_t size, StyxEvent *event, size_t *consumed)
{
    Test *test = opaque;
    const char *end = memchr(bytes, '\n', size);
    if (!end)
    {
        return 0;
    }
    size_t length = end-(const char *)bytes+1;
    char text[128];
    if (length >= sizeof(text))
    {
        return -1;
    }
    memcpy(text, bytes, length);
    text[length] = 0;
    event->session = 1;
    if (sscanf(text, "event %llu %u %u %llu", &event->generation, &event->kinds,
               &event->flags, &event->resource) != 4)
    {
        return -1;
    }
    pthread_mutex_lock(&test->lock);
    test->entered = 1;
    pthread_cond_broadcast(&test->gate);
    while (test->hold && !test->proceed)
    {
        pthread_cond_wait(&test->gate, &test->lock);
    }
    pthread_mutex_unlock(&test->lock);
    *consumed = length;
    return 1;
}


/* EN/RU: Signal final adopted context retirement; duplicate cleanup fails fixture.
 * Сигнал final adopted context retirement; duplicate cleanup проваливает fixture. */
static void release(void *opaque)
{
    Test *test = opaque;
    pthread_mutex_lock(&test->lock);
    check(!test->released++, "duplicate context release");
    pthread_cond_broadcast(&test->gate);
    pthread_mutex_unlock(&test->lock);
}


/* EN/RU: Initialize external fixture storage before handing any native context to library.
 * Инициализировать external storage до передачи native context библиотеке. */
static void init(Test *test)
{
    memset(test, 0, sizeof(*test));
    check(!pthread_mutex_init(&test->lock, NULL) && !pthread_cond_init(&test->gate, NULL), "locks");
}


/* EN/RU: Bound test-only worker/release observation; never supplies production progress.
 * Ограниченное test-only наблюдение worker/release; не обеспечивает production progress. */
static void await(Test *test, StyxFeed *feed, int releaseonly)
{
    for (unsigned i = 0; i < 5000; i++)
    {
        StyxFeedInfo info = {0};
        styxfeedinfo(feed, &info);
        pthread_mutex_lock(&test->lock);
        int ready = releaseonly == 2 ? info.records != 0 :
            releaseonly ? test->released : feed ? info.finished : test->entered;
        pthread_mutex_unlock(&test->lock);
        if (ready)
        {
            return;
        }
        struct timespec tick = {0, 1000000};
        nanosleep(&tick, NULL);
    }
    check(0, "native fixture deadline");
}


/* EN/RU: External relay checkpoint; gate only coordinates adversarial native fixture timing.
 * External relay checkpoint; gate только координирует adversarial native fixture timing. */
static void checkpoint(const char *name)
{
    puts(name);
    fflush(stdout);
    check(getchar() == 'g', "external gate");
}


/* EN/RU: Background preparation keeps its own caller capability until joined by fixture.
 * Background preparation удерживает caller capability до fixture join. */
typedef struct Change
{
    StyxMountCtl *control; /* Borrowed under fixture's owner until join. */
    StyxFeedConfig config; /* State belongs to fixture unless CAS succeeds. */
    unsigned long long result; /* Zero for invalidated preparation. */
} Change;


/* EN/RU: Native preparation thread; no guest Prog, provider pointer or control lock during RPC.
 * Native preparation thread; без guest Prog/provider pointer/control lock при RPC. */
static void *prepare(void *opaque)
{
    Change *change = opaque;
    change->result = styxmountctlset(change->control, 1, &change->config, NULL);
    return NULL;
}


/* EN/RU: Exercise live replacement through actual original C server/codec and native fd actor.
 * Проверить live replacement через original C server/codec и native fd actor. */
int main(int argc, char **argv)
{
    check(argc == 3, "port/scenario");
    const char *mode = argv[2];
    Test tests[4];
    for (unsigned i = 0; i < 4; i++)
    {
        init(&tests[i]);
    }
    tests[0].hold = !strcmp(mode, "inflight");
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(atoi(argv[1]));
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    check(fd >= 0 && !connect(fd, (struct sockaddr *)&address, sizeof(address)), "connect");
    StyxClient *client = styxclientmountfd(fd, 1024, "alice", "", 3000, 2);
    check(client && styxclientretain(client), "native client");
    StyxFeedConfig config = {"events", 128, STYX_EVENT_CONTENT|STYX_EVENT_RIGHTS, 0, decode, &tests[0], release};
    StyxMountCtl *control = NULL;
    int empty = !strcmp(mode, "empty") || !strcmp(mode, "ceiling");
    DVM_Provider *provider = styxmountprovidercontrol(client, !strcmp(mode, "ceiling") ? FVM_ACCESS_WRITE : FVM_ACCESS_MASK,
                                                  empty ? NULL : &config, &control);
    check(provider && control, "controlled mount");
    if (empty)
    {
        StyxMountInfo info;
        unsigned long long version = 0;
        check(styxmountctlinfo(control, &info) && info.version == 1 && !info.configured &&
              !styxmountctlfeed(control, &version) && version == 1, "empty control status");
        StyxFeed *feed = NULL;
        unsigned long long result = styxmountctlset(control, 1, &config, &feed);
        if (!strcmp(mode, "ceiling"))
        {
            check(!result && !feed && !tests[0].released, "write-only ceiling admitted feed");
        }
        else
        {
            check(result == 2 && feed, "empty mount activation");
            checkpoint("ready empty");
            check(styxmountctlclear(control, 2) == 3 && !styxmountctlfeed(control, &version) &&
                  version == 3, "empty mount detach");
        }
        dvm_provider_release(provider);
        styxmountctlfree(control);
        styxfeedfree(feed);
        if (result)
        {
            await(&tests[0], NULL, 1);
        }
        styxclientshutdown(client);
        styxclientfree(client);
        for (unsigned i = 0; i < 4; i++)
        {
            pthread_cond_destroy(&tests[i].gate);
            pthread_mutex_destroy(&tests[i].lock);
        }
        printf("PASS live mount control: %s\n", mode);
        return 0;
    }
    DVM_VfsHandle data = {0};
    check(dvm_provider_open(provider, "data", "r", &data) == FVM_OK, "ordinary open");
    StyxFeed *old = styxmountctlfeed(control, NULL);
    check(old != NULL, "initial source");
    if (!strcmp(mode, "inflight"))
    {
        await(&tests[0], NULL, 0);
    }
    checkpoint("ready control");
    config.state = &tests[1];
    config.path = "../hidden";
    check(!styxmountctlset(control, 1, &config, NULL) && !tests[1].released, "failed setup adopted context");
    config.path = "events";
    unsigned long long version = 1;
    StyxFeed *current = NULL;
    if (!strcmp(mode, "setrace") || !strcmp(mode, "closerace"))
    {
        Change change = {control, config, 99};
        pthread_t worker;
        check(!pthread_create(&worker, NULL, prepare, &change), "prepare thread");
        checkpoint("preparing");
        StyxMountInfo preparing;
        check(styxmountctlinfo(control, &preparing) && preparing.changing &&
              !styxmountctlset(control, 1, &config, NULL), "concurrent preparation admitted");
        if (!strcmp(mode, "closerace"))
        {
            dvm_vfs_close(&data);
            dvm_provider_release(provider);
            provider = NULL;
        }
        else
        {
            check(styxmountctlclear(control, 1) == 2, "clear during preparation");
        }
        checkpoint("invalidated");
        check(!pthread_join(worker, NULL) && !change.result && !tests[1].released, "invalidated setup adopted context");
    }
    else
    {
        check(styxmountctlset(control, 1, &config, &current) == ++version && current != old, "replacement CAS");
        check(!styxmountctlset(control, 1, &config, NULL) && !styxmountctlclear(control, 1), "stale CAS");
        checkpoint("replaced");
        if (!strcmp(mode, "chain") || !strcmp(mode, "clear"))
        {
            if (!strcmp(mode, "clear"))
            {
                check(styxmountctlclear(control, version) == version+1, "clear standby");
                version++;
                await(&tests[1], current, 0);
            }
            config.state = &tests[2];
            StyxFeed *next = NULL;
            check(styxmountctlset(control, version, &config, &next) == version+1, "second replacement");
            version++;
            styxfeedfree(current);
            current = next;
            checkpoint("replaced again");
        }
        if (tests[0].hold)
        {
            pthread_mutex_lock(&tests[0].lock);
            tests[0].proceed = 1;
            pthread_cond_broadcast(&tests[0].gate);
            pthread_mutex_unlock(&tests[0].lock);
        }
        checkpoint("released predecessor");
        await(&tests[0], old, 0);
        if (tests[0].hold)
        {
            StyxFeedInfo stopped;
            check(styxfeedinfo(old, &stopped) && !stopped.records, "old decoder published after cutover");
        }
        checkpoint("watch resumed");
        await(&tests[1], current, 2);
        check(styxmountctlclear(control, version) == version+1, "final clear");
        await(&tests[1], current, 0);
    }
    await(&tests[0], old, 0);
    if (provider)
    {
        char bytes[16];
        DVM_IoResult result = dvm_vfs_read_chunk(&data, bytes, sizeof(bytes));
        check(result.status == FVM_OK && result.count == 8 &&
              !memcmp(bytes, "ordinary", 8), "replacement changed existing ordinary fid");
        dvm_vfs_close(&data);
        dvm_provider_release(provider);
    }
    StyxMountInfo info;
    check(styxmountctlinfo(control, &info) && info.closed && !info.configured && !info.changing &&
          !styxmountctlset(control, info.version, &config, NULL) &&
          !styxmountctlclear(control, info.version), "closed control resurrected");
    styxfeedfree(old);
    styxfeedfree(current);
    styxmountctlfree(control);
    await(&tests[0], NULL, 1);
    if (strcmp(mode, "setrace") && strcmp(mode, "closerace"))
    {
        await(&tests[1], NULL, 1);
    }
    if (!strcmp(mode, "chain") || !strcmp(mode, "clear"))
    {
        await(&tests[2], NULL, 1);
    }
    styxclientshutdown(client);
    styxclientfree(client);
    for (unsigned i = 0; i < 4; i++)
    {
        pthread_cond_destroy(&tests[i].gate);
        pthread_mutex_destroy(&tests[i].lock);
    }
    printf("PASS live mount control: %s\n", mode);
    return 0;
}
