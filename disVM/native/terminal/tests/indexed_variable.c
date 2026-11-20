/* Copyright (c) 2026 bhgv
 * EN: Check index lease exclusivity, validation and retirement.
 * RU: Проверка исключительности, валидации и завершения выбора индекса.
 */
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "server.h"
#include "client.h"
#include "drivers/posix/fd.h"
#include "indexed_variable.h"

static int32_t values[] = {11, 22}; /* Owner storage updated through the proxy. */


/* EN/RU: Read one checked array element from the native fixture.
 * Прочитать проверенный элемент нативного стенда. */
static int load(void *state, uint32_t index, int32_t *value)
{
    *value = ((int32_t *)state)[index];
    return 1;
}


/* EN/RU: Store one checked array element in the native fixture.
 * Записать проверенный элемент нативного стенда. */
static int store(void *state, uint32_t index, int32_t value)
{
    ((int32_t *)state)[index] = value;
    return 1;
}


static Styxserver server; /* Dedicated native Styx dispatcher. */
static StyxIntArray shared = {.state = values, .length = 2, .load = load, .store = store}; /* Exported array. */

typedef struct Opened
{
    uint64_t generation; /* Selection held by this fid. */
    int selector; /* Closing .idx retires the selection. */
} Opened;


/* EN/RU: Open a selector or value on the connection that owns the selection.
 * Открыть выбор или значение в соединении, которому принадлежит выбор. */
static char *open_node(Qid *qid, int mode)
{
    if (qid->path <= 1 || qid->path == 4 || qid->path == 5)
    {
        return mode == OREAD ? nil : Eperm;
    }
    int selector = qid->path == 2;
    if (selector && mode != OWRITE)
    {
        return Eperm;
    }
    Opened *fid = calloc(1, sizeof(*fid));
    if (!fid)
    {
        return Enomem;
    }
    fid->selector = selector;
    fid->generation = styxintarrayopen(&shared, styxclient(&server), selector);
    if (!fid->generation || styxsetaux(&server, fid) < 0)
    {
        if (selector)
        {
            styxintarrayclose(&shared, fid->generation);
        }
        free(fid);
        return Eperm;
    }
    return nil;
}


/* EN/RU: Retire selectors on clunk or disconnect; consume the fid context.
 * Отозвать выбор при clunk или отключении и освободить контекст fid. */
static void close_node(void *state)
{
    Opened *fid = state;
    if (fid->selector)
    {
        styxintarrayclose(&shared, fid->generation);
    }
    free(fid);
}


/* EN/RU: Read the element selected by this value fid.
 * Прочитать элемент, выбранный этим fid значения. */
static char *read_node(Qid qid, char *bytes, ulong *count, vlong offset)
{
    if ((qid.path == 4 || qid.path == 5) && offset >= 0)
    {
        *count = styxreadstr(offset, bytes, *count, qid.path == 4 ? "2\n" : "array of int\n");
        return nil;
    }
    Opened *fid = styxgetaux(&server);
    size_t size = *count;
    if (qid.path != 3 || !fid || offset < 0 ||
        !styxintarrayread(&shared, fid->generation, offset, bytes, &size))
    {
        return Ebadarg;
    }
    *count = size;
    return nil;
}


/* EN/RU: Write one index or update the selected value according to the opened file.
 * Записать индекс или заменить выбранное значение в зависимости от открытого файла. */
static char *write_node(Qid qid, char *bytes, ulong *count, vlong offset)
{
    Opened *fid = styxgetaux(&server);
    if (!fid || offset < 0)
    {
        return Ebadarg;
    }
    int accepted = qid.path == 2 ? styxintarrayselect(&shared, fid->generation, offset, bytes, *count) :
        styxintarraywrite(&shared, fid->generation, offset, bytes, *count);
    return accepted ? nil : Ebadarg;
}


/* EN/RU: Serve the fixture until the client closes the carrier.
 * Обслуживать стенд до закрытия соединения клиентом. */
static void *serve(void *unused)
{
    (void)unused;
    while (!styxwait(&server) && !styxprocess(&server))
    {
    }
    styxend(&server);
    return NULL;
}


/* EN/RU: Exit the isolated fixture on a failed wire invariant.
 * Завершить отдельный стенд при нарушении протокола. */
static void require(int ok, const char *message)
{
    if (!ok)
    {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}


/* EN/RU: Open one selector and verify release and stale-value refusal across Styx.
 * Открыть выбор и проверить отзыв старого значения через Styx. */
static void wire(void)
{
    static Styxops ops = {.open = open_node, .read = read_node, .write = write_node, .freefid = close_node};
    int pair[2];
    require(!socketpair(AF_UNIX, SOCK_STREAM, 0, pair), "indexed carrier");
    int breaker = dup(pair[0]);
    unsigned caps = PHY_STREAM | PHY_ORDERED | PHY_RELIABLE;
    require(breaker >= 0 && !styxphyinit(&server, &ops, phyposixfd(pair[1], caps), 0555, 1, 2),
        "indexed server");
    require(styxadddir(&server, Qroot, 1, "values", 0555, "fixture") &&
        styxaddfile(&server, 1, 2, ".idx", 0222, "fixture") &&
        styxaddfile(&server, 1, 3, ".val", 0666, "fixture") &&
        styxaddfile(&server, 1, 4, ".len", 0444, "fixture") &&
        styxaddfile(&server, 1, 5, ".type", 0444, "fixture"), "indexed files");
    pthread_t worker;
    require(!pthread_create(&worker, NULL, serve, NULL), "indexed dispatcher");
    StyxClient *client = styxclientmountphy(phyposixfd(pair[0], caps), 256, "fixture", "", 3000, 4);
    require(client != NULL, "indexed client");
    StyxFid *idx = styxclientwalk(client, "values/.idx");
    StyxFid *val = styxclientwalk(client, "values/.val");
    require(idx && val && styxclientopen(idx, OWRITE), "index open");
    unsigned char bytes[4] = {0, 0, 0, 1};
    StyxTransfer transfer;
    StyxFid *length = styxclientwalk(client, "values/.len");
    require(length && styxclientopen(length, OREAD), "length open");
    unsigned char meta[16];
    styxclientread(length, 0, meta, sizeof(meta), &transfer);
    require(transfer.success && transfer.count == 2 && meta[0] == '2', "array length");
    styxclientclunk(length);
    StyxFid *type = styxclientwalk(client, "values/.type");
    require(type && styxclientopen(type, OREAD), "type open");
    styxclientread(type, 0, meta, sizeof(meta), &transfer);
    require(transfer.success && transfer.count == 13 && !memcmp(meta, "array of int\n", 13), "array type");
    styxclientclunk(type);
    styxclientwrite(idx, 0, bytes, sizeof(bytes), &transfer);
    require(transfer.success && transfer.count == 4 && styxclientopen(val, ORDWR), "select value");
    StyxFid *competing = styxclientwalk(client, "values/.idx");
    require(competing && !styxclientopen(competing, OWRITE), "competing index refused");
    styxclientclunk(competing);
    styxclientread(val, 0, bytes, sizeof(bytes), &transfer);
    require(transfer.success && transfer.count == 4 && bytes[3] == 22, "read selected element");
    bytes[3] = 44;
    styxclientwrite(val, 0, bytes, sizeof(bytes), &transfer);
    require(transfer.success && transfer.count == 4, "write selected element");
    styxclientclunk(idx);
    styxclientread(val, 0, bytes, sizeof(bytes), &transfer);
    require(!transfer.success, "stale value refused");
    styxclientclunk(val);
    shutdown(breaker, SHUT_RDWR);
    close(breaker);
    styxclientfree(client);
    require(!pthread_join(worker, NULL) && values[0] == 11 && values[1] == 44, "owner array update");
    values[1] = 22;
}


/* EN/RU: Interleave two clients and reject out-of-range and stale operations.
 * Чередовать два клиента и отклонить неверные индексы и старые открытия. */
int main(void)
{
    wire();
    StyxIntArray array = {.state = values, .length = 2, .load = load, .store = store};
    char first, second;
    unsigned char index[] = {0, 0, 0, 1};
    unsigned char invalid[] = {0, 0, 0, 2};
    unsigned char bytes[4];
    unsigned char update[] = {255, 255, 255, 254};
    size_t size = sizeof(bytes);
    uint64_t a = styxintarrayopen(&array, &first, 1);
    if (!a || styxintarrayopen(&array, &second, 1) || styxintarrayopen(&array, &first, 0) ||
        styxintarrayselect(&array, a, 0, invalid, sizeof(invalid)) ||
        !styxintarrayselect(&array, a, 0, index, sizeof(index)) ||
        styxintarrayselect(&array, a, 0, index, sizeof(index)) ||
        styxintarrayopen(&array, &second, 0) || styxintarrayopen(&array, &first, 0) != a ||
        !styxintarrayread(&array, a, 0, bytes, &size) || size != 4 || bytes[3] != 22 ||
        styxintarraywrite(&array, a, 0, update, 3) || values[1] != 22 ||
        !styxintarraywrite(&array, a, 0, update, sizeof(update)) || values[0] != 11 || values[1] != -2)
    {
        fputs("indexed variable admission or update failed\n", stderr);
        return 1;
    }
    styxintarrayclose(&array, a);
    uint64_t b = styxintarrayopen(&array, &second, 1);
    index[3] = 0;
    styxintarrayclose(&array, a);
    if (!b || b == a || !styxintarrayselect(&array, b, 0, index, sizeof(index)) ||
        styxintarraywrite(&array, a, 0, update, sizeof(update)) ||
        !styxintarrayread(&array, b, 0, bytes, &size) || size != 4 || bytes[3] != 11)
    {
        fputs("indexed variable retirement failed\n", stderr);
        return 1;
    }
    styxintarrayclose(&array, b);
    return 0;
}
