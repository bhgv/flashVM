/* Copyright (c) 2026 bhgv
 * EN: Construct the reverse dispatcher and reserve its optional hidden service.
 * RU: Создать встречный диспетчер и зарезервировать его скрытую службу.
 */


/* EN: Create one owner with an optional trusted write-create provider.
 * RU: Создать владельца с необязательным доверенным провайдером создания. */
static StyxCallerServer *start_view(RemotePhy *reverse, DVM_Provider *shared,
    StyxRemoveNode remove, void *state, const char *name, size_t qid, DVM_Provider *service)
{
    StyxCallerServer *server = calloc(1, sizeof(*server));
    if (!server || pthread_mutex_init(&server->scopes.lock, NULL))
    {
        free(server);
        phyfree(reverse);
        return NULL;
    }
    if (shared)
    {
        server->shared_root = styxtree(shared);
        if (!server->shared_root || !dvm_provider_retain(shared))
        {
            styxbackingfree(server->shared_root);
            pthread_mutex_destroy(&server->scopes.lock);
            free(server);
            phyfree(reverse);
            return NULL;
        }
        server->shared = shared;
        server->remove = remove;
        server->remove_state = state;
        server->permanent.count = PROG_QID + 1;
    }
    if (service)
    {
        StyxBacking *file = styxbacking(service);
        if (!file)
        {
            styxbackingfree(server->shared_root);
            dvm_provider_release(server->shared);
            pthread_mutex_destroy(&server->scopes.lock);
            free(server);
            phyfree(reverse);
            return NULL;
        }
        server->service_name = name;
        server->service_qid = qid;
        server->permanent.count = qid + 1;
        server->permanent.nodes[qid] = file;
    }
    server->references = 1;
    if (styxphyinit(&server->server, &operations, reverse, remove ? 0777 : 0555, 0, 8))
    {
        clear(&server->permanent);
        styxbackingfree(server->shared_root);
        dvm_provider_release(server->shared);
        pthread_mutex_destroy(&server->scopes.lock);
        free(server);
        return NULL;
    }
    server->wake = styxphywakepin(&server->server);
    if (!server->wake || pthread_create(&server->worker, NULL, serve, server))
    {
        styxend(&server->server);
        styxphywakefree(server->wake);
        clear(&server->permanent);
        styxbackingfree(server->shared_root);
        dvm_provider_release(server->shared);
        pthread_mutex_destroy(&server->scopes.lock);
        free(server);
        return NULL;
    }
    return server;
}



/* EN/RU: See caller_styx.h. / См. caller_styx.h. */
StyxCallerServer *styxcallerserver_view(RemotePhy *reverse, DVM_Provider *shared)
{
    return start_view(reverse, shared, NULL, NULL, NULL, 0, NULL);
}


/* EN/RU: See caller_styx.h. / См. caller_styx.h. */
StyxCallerServer *styxcallerserver_view_create(RemotePhy *reverse, DVM_Provider *shared,
    StyxRemoveNode remove, void *state)
{
    return start_view(reverse, shared, remove, state, NULL, 0, NULL);
}


/* EN/RU: See caller_styx.h. / См. caller_styx.h. */
StyxCallerServer *styxcallerserver_view_service(RemotePhy *reverse, DVM_Provider *shared,
    StyxRemoveNode remove, void *state, const char *name, size_t qid, DVM_Provider *service)
{
    if (!shared || !name || !*name || strchr(name, '/') || !strcmp(name, ".") ||
        !strcmp(name, "..") || !strcmp(name, ".prog") || !service || qid <= PROG_QID ||
        qid >= STYX_CALLER_NODES)
    {
        phyfree(reverse);
        return NULL;
    }
    return start_view(reverse, shared, remove, state, name, qid, service);
}


/* EN/RU: See caller_styx.h. / См. caller_styx.h. */
StyxCallerServer *styxcallerserver(RemotePhy *reverse)
{
    return styxcallerserver_view(reverse, NULL);
}
