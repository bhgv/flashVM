/* Copyright (c) 2026 bhgv
 * EN: Retain and retire reverse Styx mount bindings outside dispatcher ownership.
 * RU: Удержание и освобождение привязок встречного Styx вне диспетчера.
 */
#include "caller_styx_state.h"
#include "mount.h"
#include <stdlib.h>

/* EN: Publish a caller tree for its PID until the matching release.
 * RU: Опубликовать дерево клиента для его PID до освобождения. */
static void *begin(void *state, DVM_Provider *target, DVM_Provider *tree, uint32_t pid)
{
    StyxCallerServer *server = state;
    (void)target;
    return styxcallerscopeadd(&server->scopes, tree, pid) ? tree : NULL;
}


/* EN: Revoke this invocation without affecting other PIDs.
 * RU: Отозвать этот вызов, сохранив остальные PID. */
static void end(void *state, void *lease)
{
    StyxCallerServer *server = state;
    styxcallerscopedrop(&server->scopes, lease);
    styxphywake(server->wake);
}


/* EN: Release one control or provider reference without joining the worker.
 * RU: Снять ссылку управления или провайдера без ожидания потока. */
static void drop(void *state)
{
    StyxCallerServer *server = state;
    pthread_mutex_lock(&server->scopes.lock);
    int last = --server->references == 0;
    pthread_mutex_unlock(&server->scopes.lock);
    if (last)
    {
        styxphywakefree(server->wake);
        pthread_mutex_destroy(&server->scopes.lock);
        free(server);
    }
}


/* EN/RU: See caller_styx.h. On successful binding the provider owns a pin.
 * См. caller_styx.h. При успехе провайдер владеет ссылкой. */
int styxcallerbind(StyxCallerServer *server, DVM_Provider *forward)
{
    if (!server)
    {
        return 0;
    }
    pthread_mutex_lock(&server->scopes.lock);
    int retained = !server->scopes.stopped && server->references < (size_t)-1;
    if (retained)
    {
        server->references++;
    }
    pthread_mutex_unlock(&server->scopes.lock);
    if (!retained)
    {
        return 0;
    }
    DVM_CallerBinding binding = {.begin = begin, .end = end, .state = server, .release = drop};
    if (!dvm_provider_bind_caller(forward, &binding))
    {
        drop(server);
        return 0;
    }
    return 1;
}


/* EN/RU: See caller_styx.h. Do not expose a partially configured mount.
 * См. caller_styx.h. Публиковать только полностью настроенный маунт. */
DVM_Provider *styxcallermount(StyxCallerServer *server, StyxClient *forward, unsigned access)
{
    DVM_Provider *mount = styxmountprovider(forward, access);
    if (!mount || !styxcallerbind(server, mount))
    {
        dvm_provider_release(mount);
        return NULL;
    }
    return mount;
}


/* EN/RU: See caller_styx.h. The references held by mounts retire independently.
 * См. caller_styx.h. Ссылки маунтов освобождаются независимо. */
void styxcallerjoin(StyxCallerServer *server)
{
    if (server)
    {
        pthread_join(server->worker, NULL);
        drop(server);
    }
}
