/* Copyright (c) 2026 bhgv
 * EN: Validate and mount independent owner-side library carriers.
 * RU: Проверка и подключение независимых транспортов библиотек владельца.
 */


/* EN: Stop the reverse worker before dropping the controller's transport pin.
 * RU: Остановить встречный сервер до снятия ссылки на транспорт. */
static void close_library(LibrarySession *session)
{
    if (session->breaker >= 0)
    {
        shutdown(session->breaker, SHUT_RDWR);
        close(session->breaker);
    }
    styxcallerjoin(session->reverse);
    phybidirfree(session->mux);
}


/* EN: Consume one delegated library stream and mount its root in the owner.
 * RU: Принять выданный поток библиотеки и смонтировать корень у владельца. */
static int mount_library(DVM_Namespace *group, const LibraryMount *library, LibrarySession *session)
{
    int fd = dvm_terminal_connected(library->descriptor);
    if (fd < 3)
    {
        return 0;
    }
    if (library->bidirectional)
    {
        session->breaker = dup(fd);
        if (session->breaker < 0)
        {
            close(fd);
            return 0;
        }
    }
    RemotePhy *carrier = phyposixfd(fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE);
    if (library->bidirectional)
    {
        session->mux = phybidircreate(carrier, 65536);
        session->reverse = session->mux ? styxcallerserver(phybidirchannel(session->mux, 1)) : NULL;
        carrier = session->reverse ? phybidirchannel(session->mux, 0) : NULL;
    }
    StyxClient *client = carrier ? styxclientmountphy(carrier, 4096, "owner", "", 5000, 8) : NULL;
    DVM_Provider *root = !client ? NULL : session->reverse ?
        dvm_terminal_mount_peer(session->reverse, client, NULL, NULL, 0, 0) : styxmountprovider(client, FVM_ACCESS_MASK);
    DVM_NamespacePlan target;
    int installed = root && dvm_namespace_plan(group, library->path, &target) &&
        dvm_namespace_mount_provider(group, &target, root, "/styx-libraries") == 1;
    dvm_provider_release(root);
    return installed;
}


/* EN: Reject the same path and nested mounts before touching any carrier.
 * RU: Отклонить одинаковые и вложенные точки маунта до обращения к соединениям. */
static int overlaps(const char *first, const char *second)
{
    size_t a = strlen(first), b = strlen(second);
    size_t prefix = a < b ? a : b;
    return !memcmp(first, second, prefix) && (a == b || (a < b ? second[a] : first[b]) == '/');
}


/* EN: Parse and check one mount against the previously accepted prefix.
 * RU: Разобрать и сверить маунт с ранее проверенными параметрами. */
static int admit_library(LibraryMount *mounts, size_t count, const char *launch, const char *fd, const char *path, int bidirectional)
{
    char *end;
    long number = strtol(fd, &end, 10);
    if (*end || number < 3 || number > INT_MAX || number == strtol(launch, NULL, 10) ||
        !dvm_terminal_mount_path_valid(path))
    {
        return 0;
    }
    for (size_t i = 0; i < count; i++)
    {
        if (number == mounts[i].number || overlaps(path, mounts[i].path))
        {
            return 0;
        }
    }
    mounts[count] = (LibraryMount){fd, path, number, bidirectional};
    return 1;
}


/* EN: Reject aliases of the same socket before any Styx handshake starts.
 * RU: Отклонить дубликаты одного сокета до начала обмена Styx. */
int dvm_terminal_distinct_carriers(int launch, const LibraryMount *mounts, size_t count)
{
    struct stat identities[LIBRARY_MOUNT_LIMIT + 1];
    if (fstat(launch, &identities[0]))
    {
        return 0;
    }
    for (size_t i = 0; i < count; i++)
    {
        struct stat *current = &identities[i + 1];
        int descriptor = dvm_terminal_connected(mounts[i].descriptor);
        if (descriptor < 0)
        {
            return 0;
        }
        int failed = fstat(descriptor, current);
        close(descriptor);
        if (failed)
        {
            return 0;
        }
        for (size_t j = 0; j <= i; j++)
        {
            if (current->st_dev == identities[j].st_dev && current->st_ino == identities[j].st_ino)
            {
                return 0;
            }
        }
    }
    return 1;
}


/* EN/RU: See serve_libraries.h. / См. serve_libraries.h. */
int dvm_terminal_parse_libraries(LibraryMount *mounts, size_t *count, int argc, char **argv, int *index)
{
    int position = *index;
    while (argc > position + 1 &&
        (!strcmp(argv[position], "--library-fd") || !strcmp(argv[position], "--library-bidir-fd")))
    {
        int bidirectional = !strcmp(argv[position], "--library-bidir-fd");
        const char *descriptor = argv[position + 1];
        const char *path = "/libraries";
        position += 2;
        if (argc > position + 1 && !strcmp(argv[position], "--library-at"))
        {
            path = argv[position + 1];
            position += 2;
        }
        if (*count >= LIBRARY_MOUNT_LIMIT ||
            !admit_library(mounts, *count, argv[2], descriptor, path, bidirectional))
        {
            return 0;
        }
        ++*count;
    }
    *index = position;
    return 1;
}


/* EN/RU: See serve_libraries.h. / См. serve_libraries.h. */
int dvm_terminal_mount_libraries(DVM_Namespace *group, const LibraryMount *mounts,
    size_t count, LibrarySession *sessions, size_t *opened)
{
    *opened = 0;
    while (*opened < count)
    {
        size_t index = (*opened)++;
        sessions[index].breaker = -1;
        if (!mount_library(group, &mounts[index], &sessions[index]))
        {
            return 0;
        }
    }
    return 1;
}


/* EN/RU: See serve_libraries.h. / См. serve_libraries.h. */
void dvm_terminal_close_libraries(LibrarySession *sessions, size_t opened)
{
    while (opened)
    {
        close_library(&sessions[--opened]);
    }
}
