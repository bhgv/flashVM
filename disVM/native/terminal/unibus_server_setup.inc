/* Copyright (c) 2026 bhgv
 * EN: Configure the served tree and transfer its transport to the dispatcher.
 * RU: Настроить публикуемое дерево и передать транспорт диспетчеру.
 */


/* EN/RU: See unibus_server.h; release partially initialized native resources on failure.
 * См. unibus_server.h; при отказе освободить частично созданные ресурсы. */
StyxUnibusServer *styxunibusstart(RemotePhy *channel, unsigned long tag,
    DVM_Provider *original, DVM_Provider *profile, int reusable, int completion, int persistent)
{
    StyxUnibusServer *server = channel ? calloc(1, sizeof(*server)) : NULL;
    if (!server)
    {
        phyfree(channel);
        return NULL;
    }
    server->completion = completion;
    if (persistent)
    {
        server->launches = launch_queue(tag);
    }
    else
    {
        server->inbox = reusable ? styxunibusmailbox(tag) : styxunibusinbox(tag);
        server->file = styxbacking(server->inbox);
    }
    server->original = original ? styxbacking(original) : NULL;
    server->profile = profile ? styxbacking(profile) : NULL;
    if (!(server->file || server->launches) || (original && !server->original) || (profile && !server->profile))
    {
        phyfree(channel);
    }
    else if (!styxphyinit(&server->server, &operations, channel, 0555, 1, 2))
    {
        if (export_file(&server->server, Qroot, server->original, 2, "implementation.dis") &&
            export_file(&server->server, Qroot, server->profile, 3, ".interface") &&
            (!completion || styxaddfile(&server->server, Qroot, MOUNTED_ROOT, ".exit", 0444, "unibus")) &&
            styxaddfile(&server->server, Qroot, 1, STYX_UNIBUS_NODE, completion ? 0666 : 0222, "unibus") &&
            !pthread_create(&server->thread, NULL, serve, server))
        {
            return server;
        }
        styxend(&server->server);
    }
    free_launch_queue(server->launches);
    styxbackingfree(server->profile);
    styxbackingfree(server->original);
    styxbackingfree(server->file);
    dvm_provider_release(server->inbox);
    free(server);
    return NULL;
}


StyxUnibusServer *styxunibusserverstartmodule(RemotePhy *channel, unsigned long tag,
    DVM_Provider *original, DVM_Provider *profile)
{
    return styxunibusstart(channel, tag, original, profile, 0, 0, 0);
}


/* EN/RU: Publish a reusable endpoint for one serialized scalar session.
 * Опубликовать переиспользуемую точку входа для последовательной сессии. */
StyxUnibusServer *styxunibusserverstartsession(RemotePhy *channel, unsigned long tag,
    DVM_Provider *original, DVM_Provider *profile)
{
    return styxunibusstart(channel, tag, original, profile, 1, 0, 0);
}


/* EN/RU: Validate a single non-special Styx path component.
 * Проверить один обычный элемент пути Styx. */
static int node_name(const char *name)
{
    return name && *name && !strchr(name, '/') && strcmp(name, ".") && strcmp(name, "..");
}


/* EN/RU: Drop all allocated routes and the array after server stop or setup refusal.
 * Освободить все выделенные маршруты и массив после остановки или отказа запуска. */
static void free_modules(StyxUnibusServer *server)
{
    for (size_t i = 0; i < server->count; i++)
    {
        dvm_provider_release(server->modules[i].owner.request);
        dvm_provider_release(server->modules[i].owner.reply);
        dvm_provider_release(server->modules[i].load_offer);
    }
    free(server->modules);
}


/* EN/RU: Validate one borrowed entry and reserve its private dispatch routes.
 * Проверить заимствованную запись и зарезервировать её собственные маршруты. */
static int configure(MountedExport *module, const StyxMountedScalar *entry, int loaded)
{
    if (!node_name(entry->module) || !node_name(entry->name) || !strcmp(entry->name, ".interface") ||
        !strcmp(entry->name, STYX_MANAGED_DELTA_FILE))
    {
        return 0;
    }
    if (loaded)
    {
        return styxunibusprepareload(module, entry->name);
    }
    if (entry->native)
    {
        module->function = *entry->native;
        return styxscalarfunctionvalid(&module->function);
    }
    return styxscalarownerconfigure(&module->owner, &module->function, entry->dis_big, entry->dis_tuple, entry->dis_ref, entry->dis_call, entry->dis_ref_pair);
}

/* EN/RU: Export all distinct entries before making the dispatcher visible.
 * Опубликовать различные записи до запуска диспетчера. */
static int export_modules(StyxUnibusServer *server, const StyxMountedScalar *entries, int loaded)
{
    if (!styxadddir(&server->server, Qroot, MOUNTED_ROOT, "mounted", 0555, "unibus"))
    {
        return 0;
    }
    for (size_t i = 0; i < server->count; i++)
    {
        if (!configure(&server->modules[i], &entries[i], loaded))
        {
            return 0;
        }
        size_t leader = i, exports = 1;
        for (size_t j = 0; j < i; j++)
        {
            if (!strcmp(entries[i].module, entries[j].module))
            {
                if (++exports > STYX_MOUNTED_EXPORT_LIMIT || !strcmp(entries[i].name, entries[j].name))
                {
                    return 0;
                }
                if (leader == i)
                {
                    leader = j;
                }
            }
        }
        server->modules[i].directory_leader = leader;
        if (!export_function(server, i, leader, &entries[i]))
        {
            return 0;
        }
    }
    return 1;
}

#include "unibus_server_programs.inc"


/* EN/RU: Consume the channel and publish a bounded table on one Styx session.
 * Принять канал и опубликовать ограниченную таблицу в одной сессии Styx. */
StyxUnibusServer *styxunibusstarttable(RemotePhy *channel,
    const StyxMountedScalar *entries, size_t count, int loaded, DVM_Provider *reverse, const StyxUnibusLaunch *launch)
{
    int valid = count ? entries && count <= STYX_MOUNTED_SCALAR_LIMIT : launch && launch->persistent;
    StyxUnibusServer *server = channel && valid ?
        calloc(1, sizeof(*server)) : NULL;
    if (!server)
    {
        phyfree(channel);
        return NULL;
    }
    server->modules = count ? calloc(count, sizeof(*server->modules)) : NULL;
    if (count && !server->modules)
    {
        phyfree(channel);
        free(server);
        return NULL;
    }
    server->count = count;
    server->loaded = loaded;
    if (reverse && !dvm_provider_retain(reverse))
    {
        free_modules(server);
        phyfree(channel);
        free(server);
        return NULL;
    }
    server->caller_root = reverse;
    if (launch)
    {
        server->completion = 1;
        if (launch->persistent)
        {
            server->launches = launch_queue(launch->tag);
        }
        else
        {
            server->inbox = styxunibusinbox(launch->tag);
            server->file = server->inbox ? styxbacking(server->inbox) : NULL;
        }
        server->original = launch->original ? styxbacking(launch->original) : NULL;
        server->profile = launch->profile ? styxbacking(launch->profile) : NULL;
    }
    if (launch && (!(server->launches || server->file) || !server->original || !server->profile ||
        !retain_programs(server, launch)))
    {
        phyfree(channel);
    }
    else if (!styxphyinit(&server->server, &operations, channel, 0555, 1, 2))
    {
        int launch_nodes = !launch ||
            (export_file(&server->server, Qroot, server->original, 2, "implementation.dis") &&
             export_file(&server->server, Qroot, server->profile, 3, ".interface") &&
             styxaddfile(&server->server, Qroot, count ? LAUNCH_EXIT : MOUNTED_ROOT, ".exit", 0444, "unibus") &&
             styxaddfile(&server->server, Qroot, 1, STYX_UNIBUS_NODE, 0666, "unibus"));
        if (launch_nodes && (!count || export_modules(server, entries, loaded)) &&
            (!launch || export_programs(server, launch)) && !pthread_create(&server->thread, NULL, serve, server))
        {
            return server;
        }
        styxend(&server->server);
    }
    free_launch_queue(server->launches);
    styxbackingfree(server->profile);
    styxbackingfree(server->original);
    styxbackingfree(server->file);
    dvm_provider_release(server->inbox);
    free_modules(server);
    free_programs(server);
    dvm_provider_release(server->caller_root);
    free(server);
    return NULL;
}


/* EN/RU: See unibus_server.h. / См. unibus_server.h. */
StyxUnibusServer *styxunibusserverstartprograms(RemotePhy *channel, const StyxUnibusLaunch *launch)
{
    return styxunibusstarttable(channel, NULL, 0, 0, NULL, launch);
}
