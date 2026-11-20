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

struct StyxLaunchGrant
{
    size_t refs; /* Atomic refs shared by captured directory/file factories. */
    StyxRootLife *held; /* Detached retirement for the advertised remote fid. */
    unsigned long long identity; /* Exact remote node identifier. */
    unsigned long long device;   /* Exact remote device identifier. */
    DVM_LaunchAddress *(*resolve)(void *); /* Trusted native address factory. */
    void *state; /* Native configuration owned by the grant. */
    void (*release)(void *); /* Optional nonblocking state destructor. */
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
        styxmountrootretire(grant->held);
        if (grant->release)
        {
            grant->release(grant->state);
        }
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
    int matches = selected && styxmountstat(mount->client, selected, &meta) &&
        !meta.directory && meta.identity == grant->identity && meta.device == grant->device;
    styxmountclunk(mount->client, selected);
    if (!matches || dvm_cancel_requested(dvm_cancel_current()))
    {
        return NULL;
    }
    DVM_LaunchAddress *address = grant->resolve(grant->state);
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


/* EN/RU: See mount.h; stage both captures before publishing a launchable mount.
 * См. mount.h; подготовить оба захвата до публикации mount. */
DVM_Provider *styxmountproviderlaunch(StyxClient *client, const char *relative_root,
    unsigned access, const char *advertised_file, DVM_LaunchAddress *(*resolve)(void *),
    void *state, void (*release)(void *))
{
    StyxLaunchGrant *grant = NULL;
    StyxFid *root = NULL;
    StyxFid *file = NULL;
    StyxMeta root_meta;
    StyxMeta file_meta;
    unsigned required = FVM_ACCESS_READ | FVM_ACCESS_EXEC;
    if (client && styxclientcancellable(client) && resolve &&
        (access & required) == required && !(access & ~FVM_ACCESS_MASK) &&
        relative_root && *relative_root && advertised_file && *advertised_file &&
        !dvm_cancel_requested(dvm_cancel_current()))
    {
        root = styxmountwalkcancel(client, NULL, relative_root);
        if (root && styxmountstat(client, root, &root_meta) && root_meta.directory)
        {
            file = styxmountwalkcancel(client, root, advertised_file);
        }
        if (file && styxmountstat(client, file, &file_meta) && !file_meta.directory &&
            !dvm_cancel_requested(dvm_cancel_current()))
        {
            grant = calloc(1, sizeof(*grant));
        }
    }
    if (grant)
    {
        grant->held = styxmountrootlife(file);
        if (grant->held)
        {
            grant->refs = 1;
            grant->identity = file_meta.identity;
            grant->device = file_meta.device;
            grant->resolve = resolve;
            grant->state = state;
            grant->release = release;
            return styxmountroot(client, root, access, 1, 1, grant);
        }
        free(grant);
    }
    styxmountclunk(client, file);
    styxmountclunk(client, root);
    styxclientfree(client);
    if (release)
    {
        release(state);
    }
    return NULL;
}
