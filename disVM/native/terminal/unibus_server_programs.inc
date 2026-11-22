/* Copyright (c) 2026 bhgv
 * EN: Publish exact additional Dis images and one request endpoint per image.
 * RU: Публикация дополнительных образов Dis с точкой запуска на каждый.
 */


/* EN/RU: See unibus_server.h. / См. unibus_server.h. */
int styxunibusprogramname(const char *name)
{
    static const char initial[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    size_t size = name ? strlen(name) : 0;
    if (size <= 4 || size > STYX_LAUNCH_NAME_LIMIT || strcmp(name + size - 4, ".dis") ||
        !strchr(initial, name[0]))
    {
        return 0;
    }
    return strspn(name, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._-") == size;
}


/* EN: Release every acquired image and the catalog after dispatcher teardown.
 * RU: Освободить все взятые образы и список после остановки диспетчера. */
static void free_programs(StyxUnibusServer *server)
{
    for (size_t i = 0; i < server->program_count; i++)
    {
        styxbackingfree(server->programs[i]);
    }
    free(server->programs);
    styxbackingfree(server->catalog);
}


/* EN: Retain the host-authorized image list. The caller drops partial state on refusal.
 * RU: Удержать разрешённые хостом образы. При отказе вызывающая сторона очистит часть списка. */
static int retain_programs(StyxUnibusServer *server, const StyxUnibusLaunch *launch)
{
    size_t count = launch->program_count;
    if (!count)
    {
        return !launch->catalog;
    }
    if (!launch->persistent || !launch->programs || !launch->catalog || count > STYX_LAUNCH_EXTRA_LIMIT)
    {
        return 0;
    }
    server->catalog = styxbacking(launch->catalog);
    server->programs = calloc(count, sizeof(*server->programs));
    if (!server->catalog || !server->programs)
    {
        return 0;
    }
    server->program_count = count;
    for (size_t i = 0; i < count; i++)
    {
        const StyxUnibusProgram *entry = &launch->programs[i];
        if (!styxunibusprogramname(entry->name) || !entry->image ||
            !(server->programs[i] = styxbacking(entry->image)))
        {
            return 0;
        }
    }
    return 1;
}


/* EN: Install the read-only images and read/write endpoints before serving requests.
 * RU: Установить образы на чтение и точки чтения-записи до обслуживания запросов. */
static int export_programs(StyxUnibusServer *server, const StyxUnibusLaunch *launch)
{
    if (!server->program_count)
    {
        return 1;
    }
    if (!styxadddir(&server->server, Qroot, PROGRAM_DIRECTORY, STYX_LAUNCH_PROGRAMS, 0555, "unibus") ||
        !export_file(&server->server, Qroot, server->catalog, PROGRAM_CATALOG, STYX_LAUNCH_CATALOG))
    {
        return 0;
    }
    for (size_t i = 0; i < server->program_count; i++)
    {
        ulong path = FIRST_PROGRAM + i * 2;
        const char *name = launch->programs[i].name;
        char request[STYX_LAUNCH_NAME_LIMIT + 2] = ".";
        strcat(request, name);
        if (!export_file(&server->server, PROGRAM_DIRECTORY, server->programs[i], path, name) ||
            !styxaddfile(&server->server, PROGRAM_DIRECTORY, path + 1, request, 0666, "unibus"))
        {
            return 0;
        }
    }
    return 1;
}
