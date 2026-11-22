/* Copyright (c) 2026 bhgv
 * EN: Attach immutable launch offers from the peer's bounded program list.
 * RU: Привязка предложений запуска из ограниченного списка удалённых программ.
 */

#define PROGRAM_LIST_CAPACITY (STYX_LAUNCH_EXTRA_LIMIT * (STYX_LAUNCH_NAME_LIMIT + 1) + 1)


/* EN: Read the advertised catalog. Return zero for an older peer, -1 for an
 * unreadable advertisement or the byte count after verified EOF.
 * RU: Прочитать список. Вернуть ноль для старого провайдера, -1 при ошибке
 * чтения объявления либо число байт после подтверждённого EOF. */
static int read_program_catalog(StyxClient *client, char bytes[PROGRAM_LIST_CAPACITY])
{
    if (!styxclientretain(client))
    {
        return -1;
    }
    DVM_Provider *catalog = styxmountproviderat(client, STYX_LAUNCH_CATALOG, FVM_ACCESS_READ);
    if (!catalog)
    {
        return styxclientalive(client) ? 0 : -1;
    }
    DVM_VfsHandle reader = {0};
    int result = -1;
    if (dvm_provider_open(catalog, ".", "r", &reader) == FVM_OK)
    {
        size_t used = 0;
        while (used < PROGRAM_LIST_CAPACITY)
        {
            size_t capacity = PROGRAM_LIST_CAPACITY - used;
            DVM_IoResult got = dvm_vfs_read_chunk(&reader, bytes + used, capacity);
            if (got.status != FVM_OK || !(got.flags & DVM_IO_PREFIX) ||
                (got.flags & DVM_IO_UNCERTAIN) || got.count > capacity)
            {
                break;
            }
            used += got.count;
            if (!got.count || got.stop == DVM_IO_EOF)
            {
                if (used && used < PROGRAM_LIST_CAPACITY && bytes[used - 1] == '\n' &&
                    !memchr(bytes, '\0', used))
                {
                    result = (int)used;
                }
                break;
            }
        }
        dvm_vfs_close(&reader);
    }
    dvm_provider_release(catalog);
    return result;
}


/* EN: Install exact code/request pairs on one mount, consuming the client on
 * every outcome. Named pairs always complete on their read/write request fid.
 * RU: Установить точные пары образа и запроса на одном маунте. Принять client
 * при любом исходе. Именованные запуски завершаются через свой fid чтения-записи. */
static DVM_Provider *mount_launch_programs(StyxClient *client, const char *entry, const char *request,
    unsigned long tag, int scoped, int discover)
{
    char catalog[PROGRAM_LIST_CAPACITY];
    int size = discover ? read_program_catalog(client, catalog) : 0;
    MountedLaunch *primary = size < 0 ? NULL : open_launch_route(client, request, tag, scoped);
    if (!primary)
    {
        styxclientfree(client);
        return NULL;
    }
    char paths[STYX_LAUNCH_EXTRA_LIMIT][sizeof(STYX_LAUNCH_PROGRAMS) + 1 + STYX_LAUNCH_NAME_LIMIT];
    StyxLaunchOffer offers[STYX_LAUNCH_EXTRA_LIMIT + 1] = {{entry, resolve_launch, primary, release_launch}};
    size_t count = 1;
    char *name = catalog;
    char *end = catalog + size;
    while (name < end && count <= STYX_LAUNCH_EXTRA_LIMIT)
    {
        char *newline = memchr(name, '\n', (size_t)(end - name));
        if (!newline)
        {
            break;
        }
        *newline = 0;
        if (!styxunibusprogramname(name))
        {
            break;
        }
        char endpoint[sizeof(paths[0]) + 1];
        snprintf(endpoint, sizeof(endpoint), "%s/.%s", STYX_LAUNCH_PROGRAMS, name);
        MountedLaunch *launch = open_launch_route(client, endpoint, tag, 1);
        if (!launch || !launch->wait)
        {
            if (launch)
            {
                release_launch(launch);
            }
            break;
        }
        char *path = paths[count - 1];
        snprintf(path, sizeof(paths[0]), "%s/%s", STYX_LAUNCH_PROGRAMS, name);
        offers[count++] = (StyxLaunchOffer){path, resolve_launch, launch, release_launch};
        name = newline + 1;
    }
    if (name == end)
    {
        return styxmountproviderlaunches(client, ".", FVM_ACCESS_MASK, offers, count);
    }
    for (size_t i = 0; i < count; i++)
    {
        release_launch(offers[i].state);
    }
    styxclientfree(client);
    return NULL;
}
