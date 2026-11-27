/* Copyright (c) 2026 bhgv
 * EN: Verify live request cancellation and exact-fid transfer on a local Styx link.
 * RU: Проверить отмену запроса и передачу точных fid на локальном соединении Styx.
 */
#include "styxrpc/live_module.h"
#include "styxrpc/module_client.h"
#include "styxrpc/serve.h"
#include "client.h"
#include "drivers/posix/fd.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define STREAM (PHY_STREAM | PHY_ORDERED | PHY_RELIABLE) /* Reliable local test link. */
#define FILE_NAME "image.dis" /* The request must carry exactly this relative path. */
#define GENERATION "00000002" /* ID of the second request after the first was cancelled. */
#define STOP_AFTER_POLL 4U /* Before walk, write, first read, then cancel before second read. */

enum
{
    Root,       /* Session root. */
    Service,    /* Hidden load service directory. */
    Request,    /* Private load control fid. */
    Generation /* Callable result directory. */
};

typedef struct Fixture
{
    StyxRpcServer *server; /* Owned by the server thread once started. */
    unsigned writes;      /* Accepted requests, updated by the server thread. */
    unsigned reads;       /* Status reads across all requests. */
    unsigned closes;     /* Control fid clunks. */
    int leased;           /* The current request pins its publication. */
} Fixture;

typedef struct Stop
{
    unsigned calls; /* Polls sampled by the client thread. */
    unsigned limit; /* First call that requests cancellation. */
} Stop;

static const StyxRpcEntry entries[] = { /* Complete static set of test paths. */
    {Root, "/", 0555, 1}, {Service, STYXRPC_LIVE_MODULE_ROOT, 0555, 1},
    {Request, STYXRPC_LIVE_MODULE_REQUEST, 0666, 0}, {Generation, GENERATION, 0555, 1}
};


/* EN: List the service's request and any currently leased generation.
 * RU: Перечислить запрос службы и её текущее удерживаемое поколение. */
static const StyxRpcEntry *child(void *context, uint64_t id, size_t index)
{
    const Fixture *fixture = context;
    if (id == Root && !index)
    {
        return &entries[Service];
    }
    if (id == Service && index < 2)
    {
        return !index ? &entries[Request] :
            fixture->leased && fixture->writes == 2 ? &entries[Generation] : NULL;
    }
    return NULL;
}


/* EN: Resolve metadata only while the request pins the generation.
 * RU: Разрешить метаданные поколения, пока его удерживает запрос. */
static int lookup(void *context, uint64_t id, StyxRpcNode *node)
{
    const Fixture *fixture = context;
    if (id > Generation || (id == Generation && (!fixture->leased || fixture->writes != 2)))
    {
        return 0;
    }
    *node = (StyxRpcNode){entries[id], id < Request ? Root : Service, 0, 0};
    return 1;
}


/* EN: Borrow the fixture as cookie for the exclusive read-write request.
 * RU: Заимствовать состояние теста для эксклюзивного запроса чтения и записи. */
static int open_request(void *context, uint64_t id, unsigned access, void **cookie)
{
    Fixture *fixture = context;
    if (id != Request || access != (STYXRPC_READ | STYXRPC_WRITE) || fixture->leased)
    {
        return 0;
    }
    *cookie = fixture;
    return 1;
}


/* EN: Accept one full request at zero offset and pin the chosen publication.
 * RU: Принять один полный запрос со смещения ноль и удержать публикацию. */
static int write_request(void *cookie, uint64_t offset, const void *bytes, size_t size, size_t *written)
{
    Fixture *fixture = cookie;
    if (offset || size != sizeof(FILE_NAME) - 1 || memcmp(bytes, FILE_NAME, size) || fixture->leased)
    {
        return 0;
    }
    ++fixture->writes;
    fixture->leased = 1;
    *written = size;
    return 1;
}


/* EN: Leave the first request pending. Fragment the second generation over two reads.
 * RU: Оставить первый запрос ожидающим. Разделить ID второго поколения на два чтения. */
static int read_request(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    Fixture *fixture = cookie;
    ++fixture->reads;
    *size = 0;
    if (fixture->writes == 2 && offset < STYXRPC_LIVE_MODULE_ID_BYTES)
    {
        const size_t fragment = STYXRPC_LIVE_MODULE_ID_BYTES / 2;
        *size = capacity < fragment ? capacity : fragment;
        memcpy(bytes, GENERATION + offset, *size);
    }
    return 1;
}


/* EN: Withdraw the exact generation on clunk.
 * RU: Отозвать точное поколение по clunk. */
static void close_request(void *cookie)
{
    Fixture *fixture = cookie;
    ++fixture->closes;
    fixture->leased = 0;
}


/* EN: Serve the fixture until the client retires.
 * RU: Обслуживать тест, пока клиент не завершится. */
static void *dispatch(void *context)
{
    Fixture *fixture = context;
    while (styxrpc_server_step(fixture->server))
    {
    }
    styxrpc_server_destroy(fixture->server);
    return NULL;
}


/* EN: Request cancellation at the caller-selected check.
 * RU: Запросить отмену при выбранной вызывающим проверке. */
static int stopped(void *state)
{
    Stop *stop = state;
    return ++stop->calls >= stop->limit;
}


/* EN: Check preflight and post-write cancellation, then open the next generation.
 * RU: Проверить отмену до I/O и после записи, затем открыть следующее поколение. */
static int run(StyxClient *client)
{
    StyxFid *root = styxclientwalk(client, ".");
    StyxFid *directory = root, *lease = root;
    Stop early = {0, 1}, pending = {0, STOP_AFTER_POLL};
    int ok = root &&
        styxrpc_module_live_open_until(client, FILE_NAME, &directory, &lease, stopped, &early) == STYXRPC_IO_CANCELLED &&
        directory == root && lease == root &&
        styxrpc_module_live_open_until(client, FILE_NAME, &directory, &lease, stopped, &pending) == STYXRPC_IO_CANCELLED &&
        directory == root && lease == root;
    if (ok)
    {
        directory = styxrpc_module_live_open(client, FILE_NAME, &lease);
        ok = directory && lease != root;
    }
    styxclientclunk(directory != root ? directory : NULL);
    styxclientclunk(lease != root ? lease : NULL);
    styxclientclunk(root);
    return ok;
}


/* EN: Join the service before checking its completed request accounting.
 * RU: Дождаться завершения службы перед проверкой учёта запросов. */
int main(void)
{
    static const StyxRpcTree callbacks = {{child, open_request, read_request, write_request, close_request}, lookup};
    int descriptors[2];
    Fixture fixture = {0};
    pthread_t thread;
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, descriptors))
    {
        return 1;
    }
    fixture.server = styxrpc_server_create(phyposixfd(descriptors[0], STREAM), &callbacks, &fixture);
    if (!fixture.server || pthread_create(&thread, NULL, dispatch, &fixture))
    {
        styxrpc_server_destroy(fixture.server);
        close(descriptors[1]);
        return 1;
    }
    StyxClient *client = styxclientmountphy(phyposixfd(descriptors[1], STREAM), 4096, "test", "", 1000, 8);
    int ok = client && run(client);
    styxclientfree(client);
    pthread_join(thread, NULL);
    if (!ok || fixture.writes != 2 || fixture.reads != 3 || fixture.closes != 2 || fixture.leased)
    {
        fputs("live module cancellation or lease accounting failed\n", stderr);
        return 1;
    }
    return 0;
}
