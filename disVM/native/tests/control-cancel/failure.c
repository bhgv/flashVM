/* Copyright (c) 2026 bhgv
 * EN: Production abandoned-fid retirement under allocation, thread and reference refusal.
 * RU: Production abandoned-fid retirement при отказе allocation, thread и reference.
 */
#include "../../client/internal.h"
#include <errno.h>
static int refusal; /* 1 allocation,2 thread startup,3 reference overflow. */
static unsigned threads; /* Attempted native worker startups, never successful in this probe. */


/* EN/RU: Preserve allocation semantics except the explicitly selected setup refusal.
 * Сохранить allocation semantics кроме явно выбранного setup refusal. */
static void *allocate(size_t count, size_t size);


/* EN/RU: Refuse thread/output/attributes/entry/argument before publishing any worker.
 * Отклонить thread/output/attributes/entry/argument до публикации worker. */
static int createthread(pthread_t *thread, const pthread_attr_t *attributes, void *(*entry)(void *), void *argument);
#define calloc allocate
#define pthread_create createthread
#include "../../client/forget.c"
#undef pthread_create
#undef calloc


/* EN/RU: Keep contracts active and terminate only this private diagnostic process.
 * Сохранить contracts активными; завершить только private diagnostic process. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprint(2, "control cleanup refusal: %s\n", why);
        exit(2);
    }
}


/* EN/RU: See prototype; successful calls use the unchanged native allocator.
 * См. prototype; успешные calls используют неизменённый native allocator. */
static void *allocate(size_t count, size_t size)
{
    return refusal == 1 ? NULL : calloc(count, size);
}


/* EN/RU: See prototype; no thread exists and the caller retains all job ownership.
 * См. prototype; thread не создан, caller сохраняет всё job ownership. */
static int createthread(pthread_t *thread, const pthread_attr_t *attributes, void *(*entry)(void *), void *argument)
{
    USED(thread);
    USED(attributes);
    USED(entry);
    USED(argument);
    threads++;
    return EAGAIN;
}


/* EN/RU: Real terminal source delivers failure without provider, guest or transport I/O.
 * Real terminal source доставляет failure без provider, guest и transport I/O. */
static void terminal(void *opaque, int reason)
{
    unsigned *observed = opaque;
    check(reason == STYX_LINK_FAILED, "wrong terminal reason");
    (*observed)++;
}


/* EN/RU: Legacy clients and invalid outputs refuse new APIs before any transport access.
 * Legacy clients и invalid outputs отклоняют новые APIs до transport access. */
static void localrefusals(void)
{
    StyxClient legacy = {0};
    StyxFid fid = {0};
    fid.client = &legacy;
    StyxTransfer result;
    StyxMeta meta;
    memset(&meta, 0x5a, sizeof(meta));
    StyxMeta before = meta;
    check(!styxclientwalkcancel(&legacy, ".", NULL, &result) && result.confirmed && !result.uncertain,
          "legacy walk admitted");
    check(!styxclientwalkfromcancel(&fid, ".", NULL, &result) && result.confirmed && !result.uncertain,
          "legacy walkfrom admitted");
    check(!styxclientopencancel(&fid, 0, NULL, &result) && result.confirmed && !result.uncertain,
          "legacy open admitted");
    check(!styxclientstatcancel(&fid, &meta, NULL, &result) && result.confirmed && !result.uncertain &&
          !memcmp(&meta, &before, sizeof(meta)), "legacy stat admitted/modified output");
}


/* EN/RU: Every setup refusal must preserve reference count and publish terminal failure.
 * Каждый setup refusal сохраняет reference count и публикует terminal failure. */
int main(void)
{
    localrefusals();
    for (refusal = 1; refusal <= 3; refusal++)
    {
        StyxClient client = {0};
        unsigned observed = 0;
        threads = 0;
        client.references = refusal == 3 ? (size_t)-1 : 1;
        size_t initial = client.references;
        client.life = styxlifecreate();
        check(client.life && !pthread_mutex_init(&client.lock, NULL), "native fixture");
        StyxWatch *watch = styxclientwatch(&client, terminal, &observed, NULL);
        check(watch != NULL, "failure observation");
        pthread_mutex_lock(&client.lock);
        styxforgetlocked(&client, 2);
        check(client.failed && client.references == initial && observed == 1 && threads == (refusal == 2),
              "cleanup refusal waited/leaked ownership/failed silently");
        pthread_mutex_unlock(&client.lock);
        styxclientunwatch(watch);
        styxlifedrop(client.life);
        pthread_mutex_destroy(&client.lock);
    }
    print("PASS abandoned-fid setup refusal preserves ownership and retires connection\n");
    return 0;
}
