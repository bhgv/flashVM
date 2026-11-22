/* Copyright (c) 2026 bhgv
 * EN: Associate a held remote Styx file with a trusted native launch resolver.
 * RU: Связать удержанный удалённый файл Styx с доверенным resolver запуска.
 */
#include "mount_internal.h"
#include "mount_cancel.h"
#include "launch/route.h"
#include "cancel.h"
#include <stdlib.h>
#include <string.h>

typedef struct StyxLaunchNode
{
    StyxRootLife *held; /* Detached retirement for the advertised remote fid. */
    unsigned long long identity; /* Exact remote node identifier. */
    unsigned long long device; /* Exact remote device identifier. */
    DVM_LaunchAddress *(*resolve)(void *); /* Trusted native address factory. */
    void *state; /* Native configuration owned by the grant. */
    void (*release)(void *); /* Optional nonblocking state destructor. */
} StyxLaunchNode;

struct StyxLaunchGrant
{
    size_t refs; /* Atomic refs shared by captured directory/file factories. */
    size_t count; /* Number of advertised images owned by this grant. */
    StyxLaunchNode *nodes; /* Retained fids and native offer state. */
};


/* EN/RU: Pin an existing grant across capture, refusing reference overflow.
 * Удержать grant при capture, отказать при переполнении ссылок. */
int styxmountlaunchretain(StyxLaunchGrant *grant)
{
    if (!grant)
    {
        return 1;
    }
    size_t count = __atomic_load_n(&grant->refs, __ATOMIC_RELAXED);
    while (count != (size_t)-1)
    {
        if (__atomic_compare_exchange_n(&grant->refs, &count, count + 1, 0,
                __ATOMIC_RELAXED, __ATOMIC_RELAXED))
        {
            return 1;
        }
    }
    return 0;
}


/* EN/RU: Retire the fid and native resolver state after the last factory pin.
 * Завершить fid и state resolver после последней ссылки. */
void styxmountlaunchfree(StyxLaunchGrant *grant)
{
    if (grant && __atomic_sub_fetch(&grant->refs, 1, __ATOMIC_ACQ_REL) == 0)
    {
        for (size_t i = 0; i < grant->count; i++)
        {
            StyxLaunchNode *node = &grant->nodes[i];
            if (node->held)
            {
                styxmountrootretire(node->held);
            }
            if (node->release)
            {
                node->release(node->state);
            }
        }
        free(grant->nodes);
        free(grant);
    }
}


/* EN/RU: Check only the pinned connection's current native health.
 * Проверить состояние того же удержанного соединения. */
static int connected(void *state)
{
    return styxclientalive(state);
}


/* EN/RU: Drop the independent connection pin after address retirement.
 * Освободить отдельную ссылку на соединение при освобождении адреса. */
static void release_client(void *state)
{
    styxclientfree(state);
}


/* EN/RU: Resolve only the exact advertised object in the current mounted view.
 * Разрешить только объявленный объект текущего mount. */
DVM_LaunchAddress *styxmountlaunch(void *state, const char *relative)
{
    struct Mount *mount = state;
    StyxLaunchGrant *grant = mount->launch;
    if ((mount->file_root && strcmp(relative, ".")) || !styxclientalive(mount->client))
    {
        return NULL;
    }
    StyxFid *selected = styxmountwalk(mount, relative);
    StyxMeta meta;
    int file = selected && styxmountstat(mount->client, selected, &meta) && !meta.directory;
    styxmountclunk(mount->client, selected);
    if (!file || dvm_cancel_requested(dvm_cancel_current()))
    {
        return NULL;
    }
    DVM_LaunchAddress *address = NULL;
    for (size_t i = 0; i < grant->count; i++)
    {
        StyxLaunchNode *node = &grant->nodes[i];
        if (meta.identity == node->identity && meta.device == node->device)
        {
            address = node->resolve(node->state);
            break;
        }
    }
    if (!address || !styxclientretain(mount->client))
    {
        dvm_launch_address_free(address);
        return NULL;
    }
    DVM_LaunchAddress *guard = dvm_launch_address_guard(address, connected, mount->client, release_client);
    if (!guard)
    {
        styxclientfree(mount->client);
    }
    return guard;
}


/* EN: Retain a single remote file and reject identity duplicates.
 * RU: Закрепить один удалённый файл и отклонить дубликаты идентичности. */
static int pin_node(StyxClient *client, StyxFid *root, StyxLaunchGrant *grant, size_t index, const char *path)
{
    StyxFid *file = path && *path ? styxmountwalkcancel(client, root, path) : NULL;
    StyxMeta meta;
    int valid = file && styxmountstat(client, file, &meta) && !meta.directory;
    for (size_t i = 0; valid && i < index; i++)
    {
        valid = grant->nodes[i].identity != meta.identity || grant->nodes[i].device != meta.device;
    }
    StyxLaunchNode *node = &grant->nodes[index];
    if (valid && !dvm_cancel_requested(dvm_cancel_current()) && (node->held = styxmountrootlife(file)))
    {
        node->identity = meta.identity;
        node->device = meta.device;
        return 1;
    }
    styxmountclunk(client, file);
    return 0;
}


/* EN/RU: See mount.h. / См. mount.h. */
DVM_Provider *styxmountproviderlaunches(StyxClient *client, const char *relative_root,
    unsigned access, const StyxLaunchOffer *offers, size_t count)
{
    StyxLaunchGrant *grant = NULL;
    StyxFid *root = NULL;
    StyxMeta meta;
    unsigned required = FVM_ACCESS_READ | FVM_ACCESS_EXEC;
    if (client && styxclientcancellable(client) && offers && count &&
        (access & required) == required && !(access & ~FVM_ACCESS_MASK) &&
        relative_root && *relative_root && !dvm_cancel_requested(dvm_cancel_current()))
    {
        root = styxmountwalkcancel(client, NULL, relative_root);
        if (root && styxmountstat(client, root, &meta) && meta.directory)
        {
            grant = calloc(1, sizeof(*grant));
        }
    }
    if (grant)
    {
        grant->nodes = calloc(count, sizeof(*grant->nodes));
        if (grant->nodes)
        {
            grant->refs = 1;
            grant->count = count;
            for (size_t i = 0; i < count; i++)
            {
                grant->nodes[i].resolve = offers[i].resolve;
                grant->nodes[i].state = offers[i].state;
                grant->nodes[i].release = offers[i].release;
            }
            for (size_t i = 0; i < count; i++)
            {
                if (!offers[i].resolve || !pin_node(client, root, grant, i, offers[i].path))
                {
                    styxmountlaunchfree(grant);
                    styxmountclunk(client, root);
                    styxclientfree(client);
                    return NULL;
                }
            }
            return styxmountroot(client, root, access, 1, 1, grant);
        }
        free(grant);
    }
    styxmountclunk(client, root);
    styxclientfree(client);
    for (size_t i = 0; offers && i < count; i++)
    {
        if (offers[i].release)
        {
            offers[i].release(offers[i].state);
        }
    }
    return NULL;
}


/* EN/RU: See mount.h; preserve the exact single-file interface. / См. mount.h; сохранить API одного файла. */
DVM_Provider *styxmountproviderlaunch(StyxClient *client, const char *relative_root,
    unsigned access, const char *advertised_file, DVM_LaunchAddress *(*resolve)(void *),
    void *state, void (*release)(void *))
{
    StyxLaunchOffer offer = {advertised_file, resolve, state, release};
    return styxmountproviderlaunches(client, relative_root, access, &offer, 1);
}
