/* Copyright (c) 2026 bhgv
 * EN: Export immutable fixture movies as ordinary files over a granted Styx session.
 * RU: Публикация неизменяемых тестовых фильмов как обычных файлов по выданной сессии Styx.
 */
#include "movie-assets.h"
#include "drivers/posix/fd.h"
#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>

typedef struct Asset
{
    StyxRpcEntry entry; /* Public logical node and permissions. */
    uint64_t parent;    /* Public parent ID. */
    const char *path;  /* Test provider path for files, NULL for directories. */
} Asset;

static const Asset assets[] = {
    {{0, "/", 0555, 1}, 0, NULL},
    {{1, "movies", 0555, 1}, 0, NULL},
    {{2, "chapter", 0555, 1}, 1, NULL},
    {{3, "child.swf", 0444, 0}, 2, "share/child.swf"},
    {{4, "leaf.swf", 0444, 0}, 1, "share/leaf.swf"},
    {{5, "broken.swf", 0444, 0}, 0, "share/broken.swf"},
    {{6, ".prog", 0555, 1}, 0, NULL},
    {{7, "child.swf", 0444, 0}, 6, "share/child.swf"}
}; /* The client sees only this virtual namespace. */


/* EN: Enumerate immediate children from the fixed provider, ignoring the host cwd.
 * RU: Перечислить потомков фиксированного провайдера независимо от cwd. */
static const StyxRpcEntry *entries(void *state, uint64_t parent, size_t index)
{
    (void)state;
    for (size_t i = 1; i < sizeof(assets) / sizeof(*assets); ++i)
    {
        if (assets[i].parent == parent && !index--)
        {
            return &assets[i].entry;
        }
    }
    return NULL;
}


/* EN: Report stable node identity and the extent of its one fixture file.
 * RU: Вернуть идентификатор узла и длину одного файла проверки. */
static int describe(void *state, uint64_t id, StyxRpcNode *out)
{
    (void)state;
    if (id >= sizeof(assets) / sizeof(*assets))
    {
        return 0;
    }
    const Asset *asset = assets + id;
    struct stat metadata;
    if (asset->path && (stat(asset->path, &metadata) != 0 || metadata.st_size < 0))
    {
        return 0;
    }
    *out = (StyxRpcNode){.entry = asset->entry, .parent = asset->parent,
        .length = asset->path ? (uint64_t)metadata.st_size : 0};
    return 1;
}


/* EN: Hold one read-only open for the exact requested node.
 * RU: Удержать одно открытие на чтение точного узла. */
static int open_asset(void *state, uint64_t id, unsigned mode, void **cookie)
{
    (void)state;
    if (id >= sizeof(assets) / sizeof(*assets) || !assets[id].path || mode != STYXRPC_READ)
    {
        return 0;
    }
    *cookie = fopen(assets[id].path, "rb");
    return *cookie != NULL;
}


/* EN: Serve each acknowledged slice from the exact open, including EOF.
 * RU: Вернуть подтверждённый фрагмент точного дескриптора, включая EOF. */
static int read_asset(void *cookie, uint64_t offset, void *bytes, size_t count, size_t *used)
{
    FILE *file = cookie;
    if (offset > 32768 || fseek(file, (long)offset, SEEK_SET))
    {
        return 0;
    }
    *used = fread(bytes, 1, count, file);
    return !ferror(file);
}


/* EN: Reject all writes to the immutable provider.
 * RU: Отклонить запись в неизменяемый провайдер. */
static int deny_write(void *cookie, uint64_t offset, const void *bytes, size_t count, size_t *used)
{
    (void)cookie;
    (void)offset;
    (void)bytes;
    (void)count;
    (void)used;
    return 0;
}


/* EN: Close one file after clunk or loss of the carrier.
 * RU: Закрыть файл после clunk либо разрыва соединения. */
static void close_asset(void *cookie)
{
    fclose(cookie);
}

static const StyxRpcTree tree = {{entries, open_asset, read_asset, deny_write, close_asset}, describe};


/* EN: Own the server on its independent thread through disconnection.
 * RU: Владеть сервером в отдельном потоке до разрыва соединения. */
static void *serve(void *state)
{
    StyxRpcServer *server = state;
    while (styxrpc_server_step(server))
    {
    }
    styxrpc_server_destroy(server);
    return NULL;
}


/* EN/RU: See movie-assets.h. / См. movie-assets.h. */
int test_movie_assets_start(MovieAssets *provider)
{
    int socket_fd[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fd))
    {
        return 0;
    }
    RemotePhy *peer = phyposixfd(socket_fd[0], PHY_STREAM | PHY_ORDERED | PHY_RELIABLE);
    provider->server = styxrpc_server_create(phyposixfd(socket_fd[1], PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), &tree, NULL);
    if (!provider->server || pthread_create(&provider->thread, NULL, serve, provider->server))
    {
        phyfree(peer);
        styxrpc_server_destroy(provider->server);
        return 0;
    }
    provider->running = 1;
    provider->client = styxclientmountphy(peer, 4096, "movies", "", 3000, 8);
    return provider->client != NULL;
}


/* EN/RU: See movie-assets.h. / См. movie-assets.h. */
void test_movie_assets_stop(MovieAssets *provider)
{
    styxclientshutdown(provider->client);
    styxclientfree(provider->client);
    if (provider->running)
    {
        pthread_join(provider->thread, NULL);
    }
}
