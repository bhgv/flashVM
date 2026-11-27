/* Copyright (c) 2026 bhgv
 * EN: Attach private mailbox routes and a checked Styx directory to one resident.
 * RU: Привязать частные очереди и каталог Styx к одному владельцу.
 */
#include "live_modules_private.h"
#include "mount.h"
#include "publish_profile.h"
#include "drivers/posix/fd.h"
#include "../vfs/export.h"
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>


/* EN: Build the worker's exact request, reply and offer file mappings. Return owned root.
 * RU: Собрать точные файлы запросов, ответов и предложений. Вернуть собственный корень. */
static DVM_Provider *owner_routes(StyxUnibusServer *server, size_t count)
{
    DVM_View *view = dvm_view_create(NULL, NULL, NULL);
    if (!view)
    {
        return NULL;
    }
    for (size_t i = 0; i < count; i++)
    {
        char request[24], reply[24], offer[24];
        snprintf(request, sizeof(request), "/request%zu", i);
        snprintf(reply, sizeof(reply), "/reply%zu", i);
        snprintf(offer, sizeof(offer), "/offer%zu", i);
        if (!dvm_view_map(view, request, styxunibusservermailbox(server, i, 0), ".", FVM_ACCESS_READ, 0) ||
            !dvm_view_map(view, reply, styxunibusservermailbox(server, i, 1), ".", FVM_ACCESS_WRITE, 0) ||
            !dvm_view_map(view, offer, styxunibusserverloadprofileat(server, i), ".", FVM_ACCESS_WRITE, 0))
        {
            dvm_view_discard(view);
            return NULL;
        }
    }
    return dvm_view_freeze(view);
}


/* EN: Find the exact calling resident with the expected phase while holding lock.
 * RU: Найти точного вызывающего владельца в ожидаемой фазе под блокировкой. */
static LiveModule *resident(LiveModules *session, int pid, LiveModulePhase phase)
{
    for (size_t i = 0; !session->stopped && i < DVM_LIVE_MODULE_LIMIT; ++i)
    {
        LiveModule *entry = &session->entries[i];
        if (entry->phase == phase && entry->pid == pid)
        {
            return entry;
        }
    }
    return NULL;
}


/* EN: Publish one resident's validated route while its original anchor remains live.
 * RU: Опубликовать проверенный маршрут владельца при удерживаемом исходном anchor. */
int dvm_live_module_prepare(DVM_Namespace *group, int pid, const void *offers, size_t size)
{
    LiveModules *session = dvm_live_modules_get();
    const char *names[STYX_MOUNTED_EXPORT_LIMIT];
    unsigned char profile[STYX_MOUNTED_EXPORT_LIMIT * STYX_SHAPED_OFFER_MAX];
    StyxPublishedExport entries[STYX_MOUNTED_EXPORT_LIMIT];
    size_t used, count;
    if (!session || !styxpublishprofile(offers, size, NULL, 1,
        profile, sizeof(profile), &used, entries, STYX_MOUNTED_EXPORT_LIMIT, &count))
    {
        return 0;
    }
    pthread_mutex_lock(&session->lock);
    LiveModule *entry = resident(session, pid, LIVE_BOOTING);
    pthread_mutex_unlock(&session->lock);
    if (!entry)
    {
        return 0;
    }
    for (size_t i = 0; i < count; ++i)
    {
        names[i] = entries[i].name;
    }
    int pair[2];
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, pair))
    {
        return 0;
    }
    int breaker = dup(pair[0]);
    if (breaker < 0)
    {
        close(pair[0]);
        close(pair[1]);
        return 0;
    }
    StyxUnibusServer *server = styxunibusserverstartscopedset(
        phyposixfd(pair[1], PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), "live.dis",
        names, count, session->reverse);
    StyxClient *loop = styxclientmountphy(
        phyposixfd(pair[0], PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), 4096, "owner", "", 3000, 8);
    DVM_Provider *root = loop ? styxmountproviderat(loop, DVM_LIVE_MODULE_WIRE, FVM_ACCESS_MASK) : NULL;
    DVM_Provider *routes = server ? owner_routes(server, count) : NULL;
    DVM_NamespacePlan plan;
    int ready = root && routes && dvm_namespace_plan(group, DVM_LIVE_MODULE_OWNER, &plan) &&
        dvm_namespace_mount_provider(group, &plan, routes, "/styx-live-owner") == 1;
    dvm_provider_release(routes);
    pthread_mutex_lock(&session->lock);
    ready = ready && !session->stopped && entry->phase == LIVE_BOOTING;
    if (ready)
    {
        entry->breaker = breaker;
        entry->server = server;
        entry->root = root;
        entry->phase = LIVE_STAGING;
    }
    pthread_mutex_unlock(&session->lock);
    if (!ready)
    {
        shutdown(breaker, SHUT_RDWR);
        close(breaker);
        dvm_provider_release(root);
        styxunibusserverjoin(server);
    }
    return ready;
}


/* EN/RU: See live_modules_private.h. / См. live_modules_private.h. */
int dvm_live_module_commit(int pid)
{
    LiveModules *session = dvm_live_modules_get();
    if (!session)
    {
        return 0;
    }
    pthread_mutex_lock(&session->lock);
    LiveModule *entry = resident(session, pid, LIVE_STAGING);
    DVM_Provider *root = entry && dvm_provider_retain(entry->root) ? entry->root : NULL;
    pthread_mutex_unlock(&session->lock);
    if (!root)
    {
        return 0;
    }
    DVM_VfsHandle profile = {0};
    int ready = dvm_provider_open(root, ".interface", "r", &profile) == FVM_OK;
    dvm_vfs_close(&profile);
    dvm_provider_release(root);
    pthread_mutex_lock(&session->lock);
    ready = ready && !session->stopped && entry->phase == LIVE_STAGING;
    if (ready)
    {
        entry->phase = LIVE_READY;
        pthread_cond_broadcast(&session->changed);
    }
    pthread_mutex_unlock(&session->lock);
    return ready;
}
