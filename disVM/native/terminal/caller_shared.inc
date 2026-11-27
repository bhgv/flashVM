/* Copyright (c) 2026 bhgv
 * EN: Intern ordinary reverse Qids identically for peer walks and local captures.
 * RU: Выдача одинаковых встречных Qid при обходе со стороны пары и местном захвате.
 */


/* EN: Reserve one Qid before the dispatcher enters a provider create that can block.
 * RU: Зарезервировать один Qid до потенциально блокирующей операции создания. */
static int reserve_create(StyxCallerServer *server)
{
    pthread_mutex_lock(&server->scopes.lock);
    int available = server->permanent.count < STYX_CALLER_NODES;
    server->reserving_create = available;
    pthread_mutex_unlock(&server->scopes.lock);
    return available;
}


/* EN: Consume node and return its permanent Qid, or zero if stopped or full.
 * creating marks the dispatcher's reserved create and releases that reservation even on error.
 * RU: Принять node и вернуть постоянный Qid либо ноль при остановке или переполнении.
 * creating обозначает зарезервированное создание и снимает резерв даже при отказе. */
static uint64_t remember_shared(StyxCallerServer *server, StyxBacking *node, int creating)
{
    uint64_t qid = 0;
    pthread_mutex_lock(&server->scopes.lock);
    if (creating)
    {
        server->reserving_create = 0;
    }
    CallerNodes *cache = &server->permanent;
    if (!server->scopes.stopped && node)
    {
        size_t i = PROG_QID + 1;
        for (; i < cache->count; ++i)
        {
            if (styxbackingsamenode(cache->nodes[i], node))
            {
                qid = i;
                break;
            }
        }
        if (!qid && cache->count < STYX_CALLER_NODES - (size_t)server->reserving_create)
        {
            qid = cache->count;
            cache->nodes[cache->count++] = node;
            node = NULL;
        }
    }
    pthread_mutex_unlock(&server->scopes.lock);
    styxbackingfree(node);
    return qid;
}


/* EN/RU: See caller_styx.h. / См. caller_styx.h. */
StyxOpen *styxcalleropen_shared(StyxCallerServer *server, const char *path, uint64_t qid, int mode)
{
    if (!server || qid <= PROG_QID || qid >= STYX_CALLER_NODES || (mode != 0 && mode != 2))
    {
        return NULL;
    }
    pthread_mutex_lock(&server->scopes.lock);
    StyxBacking *node = !server->scopes.stopped && qid < server->permanent.count ?
        styxbackingcopyat(server->permanent.nodes[qid], path) : NULL;
    pthread_mutex_unlock(&server->scopes.lock);
    StyxOpen *opened = node ? styxfileopen(node, mode) : NULL;
    styxbackingfree(node);
    return opened;
}


/* EN: Consume the current backing and walk one canonical path component at a time.
 * RU: Принять исходный backing и пройти канонический путь по одному компоненту. */
static StyxBacking *walk_path(StyxBacking *node, const char *path)
{
    while (node)
    {
        char component[STYX_NAME_MAX];
        size_t size = strcspn(path, "/");
        StyxBacking *child = NULL;
        if (size && size < sizeof(component))
        {
            memcpy(component, path, size);
            component[size] = 0;
            if (strcmp(component, ".") && strcmp(component, ".."))
            {
                child = styxwalknode(node, component);
            }
        }
        styxbackingfree(node);
        node = child;
        if (!path[size])
        {
            break;
        }
        path += size + 1;
    }
    return node;
}


/* EN/RU: See caller_styx.h. / См. caller_styx.h. */
StyxOpen *styxcallercapture_shared(StyxCallerServer *server, const char *path, uint64_t length,
    int mode, uint64_t *qid)
{
    if (!server || !path || !qid || (mode != 0 && mode != 2))
    {
        return NULL;
    }
    pthread_mutex_lock(&server->scopes.lock);
    DVM_Provider *provider = !server->scopes.stopped && server->shared &&
        dvm_provider_retain(server->shared) ? server->shared : NULL;
    pthread_mutex_unlock(&server->scopes.lock);
    StyxBacking *node = walk_path(styxtree(provider), path);
    dvm_provider_release(provider);
    StyxOpen *file = node ? styxfileopen(node, mode) : NULL;
    StyxMeta info;
    if (!file || !styxfileinfo(file, &info) || info.directory || info.length != length)
    {
        styxbackingfree(node);
        styxfileclose(file);
        return NULL;
    }
    uint64_t issued = remember_shared(server, node, 0);
    if (!issued)
    {
        styxfileclose(file);
        return NULL;
    }
    *qid = issued;
    return file;
}
