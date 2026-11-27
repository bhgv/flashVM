/* Copyright (c) 2026 bhgv
 * EN: Project exact module generations into the reverse session and retire them with it.
 * RU: Проецировать точные поколения модулей во встречную сессию и завершать вместе с ней.
 */
#include "live_modules_private.h"
#include "mount.h"
#include "../vfs/export.h"
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static LiveModules *installed; /* Installed before dispatch and cleared after join. */


/* EN/RU: See live_modules_private.h. / См. live_modules_private.h. */
LiveModules *dvm_live_modules_get(void)
{
    return __atomic_load_n(&installed, __ATOMIC_ACQUIRE);
}


/* EN/RU: See live_modules.h. / См. live_modules.h. */
int dvm_live_modules_start(DVM_Namespace *group, StyxClient *client, DVM_Provider *reverse)
{
    LiveModules *session = calloc(1, sizeof(*session));
    if (!session || pthread_mutex_init(&session->lock, NULL))
    {
        free(session);
        return 0;
    }
    pthread_condattr_t attr;
    int made = !pthread_condattr_init(&attr);
    int initialized = made && !pthread_condattr_setclock(&attr, CLOCK_MONOTONIC) &&
        !pthread_cond_init(&session->changed, &attr);
    if (made)
    {
        pthread_condattr_destroy(&attr);
    }
    if (!initialized)
    {
        pthread_mutex_destroy(&session->lock);
        free(session);
        return 0;
    }
    session->group = group;
    session->control = dvm_live_module_control(session);
    DVM_View *view = dvm_view_create(NULL, NULL, NULL);
    if (session->control && dvm_view_map(view, "/" STYXRPC_LIVE_MODULE_REQUEST,
        session->control, ".", FVM_ACCESS_READ | FVM_ACCESS_WRITE, 0))
    {
        session->empty = dvm_view_freeze(view);
    }
    else
    {
        dvm_view_discard(view);
    }
    if (styxclientretain(client))
    {
        session->client = client;
    }
    if (dvm_provider_retain(reverse))
    {
        session->reverse = reverse;
    }
    __atomic_store_n(&installed, session, __ATOMIC_RELEASE);
    if (session->control && session->empty && session->client && session->reverse)
    {
        return 1;
    }
    return 0;
}


/* EN/RU: See live_modules.h. / См. live_modules.h. */
DVM_Provider *dvm_live_modules_route(void *group, const char **path, int *reserved)
{
    const char *relative = *path;
    const size_t prefix = sizeof(STYXRPC_LIVE_MODULE_ROOT) - 1;
    *reserved = !strncmp(relative, STYXRPC_LIVE_MODULE_ROOT, prefix) &&
        (!relative[prefix] || relative[prefix] == '/');
    if (!*reserved)
    {
        return NULL;
    }
    LiveModules *session = dvm_live_modules_get();
    if (!session || group != session->group)
    {
        return NULL;
    }
    relative += prefix;
    DVM_Provider *root = NULL;
    pthread_mutex_lock(&session->lock);
    if (!session->stopped)
    {
        if (!*relative)
        {
            root = session->empty;
            *path = ".";
        }
        else if (!strcmp(relative + 1, STYXRPC_LIVE_MODULE_REQUEST))
        {
            root = session->control;
            *path = ".";
        }
        else
        {
            uint32_t id = dvm_live_module_parse(relative + 1);
            for (size_t i = 0; id && i < DVM_LIVE_MODULE_LIMIT; ++i)
            {
                LiveModule *entry = &session->entries[i];
                if (entry->generation == id && entry->phase == LIVE_READY)
                {
                    root = entry->root;
                    relative += 1 + STYXRPC_LIVE_MODULE_ID_BYTES;
                    *path = *relative ? relative + 1 : ".";
                    break;
                }
            }
        }
    }
    if (root && !dvm_provider_retain(root))
    {
        root = NULL;
    }
    pthread_mutex_unlock(&session->lock);
    return root;
}


/* EN/RU: See live_modules.h. / См. live_modules.h. */
void dvm_live_modules_primary(int pid)
{
    LiveModules *session = dvm_live_modules_get();
    if (session)
    {
        session->primary = pid;
    }
}


/* EN/RU: See live_modules.h. / См. live_modules.h. */
void dvm_live_modules_primary_exit(int pid)
{
    LiveModules *session = dvm_live_modules_get();
    if (session && pid == session->primary)
    {
        dvm_live_modules_stop();
    }
}


/* EN/RU: See live_modules.h. / См. live_modules.h. */
void dvm_live_modules_stop(void)
{
    LiveModules *session = dvm_live_modules_get();
    if (!session)
    {
        return;
    }
    pthread_mutex_lock(&session->lock);
    if (!session->stopped)
    {
        session->stopped = 1;
        for (unsigned i = 0; i < DVM_LIVE_MODULE_LIMIT; ++i)
        {
            LiveModule *entry = &session->entries[i];
            if (entry->generation && entry->breaker >= 0)
            {
                shutdown(entry->breaker, SHUT_RDWR);
            }
        }
        pthread_cond_broadcast(&session->changed);
    }
    pthread_mutex_unlock(&session->lock);
}


/* EN/RU: See live_modules_private.h. / См. live_modules_private.h. */
void dvm_live_module_release(LiveModule *entry)
{
    if (entry->generation && entry->breaker >= 0)
    {
        close(entry->breaker);
    }
    dvm_provider_release(entry->root);
    styxunibusserverjoin(entry->server);
}


/* EN/RU: See live_modules.h. / См. live_modules.h. */
void dvm_live_modules_finish(void)
{
    LiveModules *session = dvm_live_modules_get();
    if (!session)
    {
        return;
    }
    dvm_live_modules_stop();
    __atomic_store_n(&installed, NULL, __ATOMIC_RELEASE);
    for (unsigned i = 0; i < DVM_LIVE_MODULE_LIMIT; ++i)
    {
        dvm_live_module_release(&session->entries[i]);
    }
    dvm_provider_release(session->control);
    dvm_provider_release(session->empty);
    dvm_provider_release(session->reverse);
    styxclientfree(session->client);
    pthread_cond_destroy(&session->changed);
    pthread_mutex_destroy(&session->lock);
    free(session);
}
