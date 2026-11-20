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
#include "indexed_styx.h"

static int32_t values[8] = {11, 22}; /* Owner storage updated through the proxy. */


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


static uint32_t used = 2; /* Actual owner storage length. */


/* EN/RU: Move the suffix and insert one value within fixture storage.
 * Сдвинуть хвост и вставить значение в пределах хранилища стенда. */
static int insert(void *state, uint32_t index, int32_t value)
{
    int32_t *data = state;
    if (index > used || used == 8)
    {
        return 0;
    }
    memmove(data + index + 1, data + index, (used - index) * sizeof(*data));
    data[index] = value;
    used++;
    return 1;
}


/* EN/RU: Remove and return the last fixture value.
 * Удалить и вернуть последний элемент стенда. */
static int pop(void *state, int32_t *value)
{
    if (!used)
    {
        return 0;
    }
    *value = ((int32_t *)state)[--used];
    return 1;
}


static Styxserver server; /* Dedicated native Styx dispatcher. */
static StyxIntArray shared = {.state = values, .length = 2, .load = load, .store = store, .insert = insert, .pop = pop}; /* Exported array. */

static StyxIntArray readonly = {.state = values, .length = 1, .load = load}; /* Separate read-only export. */
static StyxArrayFiles read_files; /* Read-only adapter on the same session. */
static StyxArrayFiles files; /* Configured library adapter. */


/* EN/RU: Select the adapter by the fixture's qid partition.
 * Выбрать адаптер по диапазону qid стенда. */
static StyxArrayFiles *route(Path qid)
{
    return qid < read_files.base ? &files : &read_files;
}


/* EN/RU: Route root and array opens to their handlers.
 * Передать открытия корня и массива соответствующим обработчикам. */
static char *open_node(Qid *qid, int mode)
{
    if (qid->path == Qroot)
    {
        return mode == OREAD ? nil : Eperm;
    }
    return styxarrayfileopen(route(qid->path), qid, mode);
}


/* EN/RU: Forward the fixture's array reads to the reusable adapter.
 * Передать чтения массива библиотечному адаптеру. */
static char *read_node(Qid qid, char *bytes, ulong *count, vlong offset)
{
    return styxarrayfileread(route(qid.path), qid, bytes, count, offset);
}


/* EN/RU: Forward the fixture's array writes to the reusable adapter.
 * Передать записи массива библиотечному адаптеру. */
static char *write_node(Qid qid, char *bytes, ulong *count, vlong offset)
{
    return styxarrayfilewrite(route(qid.path), qid, bytes, count, offset);
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
    static Styxops ops = {.open = open_node, .read = read_node, .write = write_node};
    int pair[2];
    require(!socketpair(AF_UNIX, SOCK_STREAM, 0, pair), "indexed carrier");
    int breaker = dup(pair[0]);
    unsigned caps = PHY_STREAM | PHY_ORDERED | PHY_RELIABLE;
    require(breaker >= 0 && !styxphyinit(&server, &ops, phyposixfd(pair[1], caps), 0555, 1, 2),
        "indexed server");
    require(styxarrayfilepublish(&files, &server, Qroot, 1, "values", "fixture", &shared), "indexed files");
    require(styxarrayfilepublish(&read_files, &server, Qroot, 20, "readonly", "fixture", &readonly),
        "read-only indexed files");
    StyxArrayFiles invalid;
    require(!styxarrayfilepublish(&invalid, &server, Qroot, 5, "overlap", "fixture", &shared),
        "qid collision refused");
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
    StyxFid *read_index = styxclientwalk(client, "readonly/.idx");
    StyxFid *read_value = styxclientwalk(client, "readonly/.val");
    require(read_index && read_value && styxclientopen(read_index, OWRITE), "independent selector");
    memset(bytes, 0, sizeof(bytes));
    styxclientwrite(read_index, 0, bytes, sizeof(bytes), &transfer);
    require(transfer.success && !styxclientopen(read_value, OWRITE), "read-only value refuses write access");
    styxclientclunk(read_value);
    read_value = styxclientwalk(client, "readonly/.val");
    require(read_value && styxclientopen(read_value, OREAD), "read-only value open");
    styxclientread(read_value, 0, bytes, sizeof(bytes), &transfer);
    require(transfer.success && transfer.count == 4 && bytes[3] == 11, "read-only selected value");
    styxclientclunk(read_value);
    styxclientclunk(read_index);
    bytes[3] = 44;
    styxclientwrite(val, 0, bytes, sizeof(bytes), &transfer);
    require(transfer.success && transfer.count == 4, "write selected element");
    StyxFid *blocked = styxclientwalk(client, "values/.push");
    require(blocked && styxclientopen(blocked, OWRITE), "open competing push");
    styxclientwrite(blocked, 0, bytes, sizeof(bytes), &transfer);
    require(!transfer.success, "open .idx blocks mutation");
    styxclientclunk(blocked);
    styxclientclunk(idx);
    styxclientread(val, 0, bytes, sizeof(bytes), &transfer);
    require(!transfer.success, "stale value refused");
    styxclientclunk(val);
    StyxFid *push = styxclientwalk(client, "values/.push");
    require(push && styxclientopen(push, OWRITE), "push open");
    unsigned char negative[] = {255, 255, 255, 254};
    styxclientwrite(push, 0, negative, sizeof(negative), &transfer);
    require(transfer.success && transfer.count == 4, "push value");
    styxclientwrite(push, 0, negative, sizeof(negative), &transfer);
    require(!transfer.success, "one push per fid");
    styxclientclunk(push);
    StyxFid *tail = styxclientwalk(client, "values/.pop");
    require(tail && styxclientopen(tail, OREAD), "pop open");
    for (unsigned read = 0; read < 2; read++)
    {
        styxclientread(tail, 0, bytes, sizeof(bytes), &transfer);
        require(transfer.success && transfer.count == 4 && !memcmp(bytes, negative, 4), "one pop with cached result");
    }
    styxclientclunk(tail);
    StyxFid *insertion = styxclientwalk(client, "values/.ins");
    require(insertion && styxclientopen(insertion, OWRITE), "insert open");
    unsigned char insertion_frame[] = {0, 0, 0, 1, 0, 0, 0, 77};
    styxclientwrite(insertion, 0, insertion_frame, sizeof(insertion_frame), &transfer);
    require(transfer.success && transfer.count == 8, "insert at index one");
    styxclientclunk(insertion);
    length = styxclientwalk(client, "values/.len");
    require(length && styxclientopen(length, OREAD), "updated length open");
    styxclientread(length, 0, meta, sizeof(meta), &transfer);
    require(transfer.success && transfer.count == 2 && meta[0] == '3', "updated length");
    styxclientclunk(length);
    shutdown(breaker, SHUT_RDWR);
    close(breaker);
    styxclientfree(client);
    require(!pthread_join(worker, NULL) && used == 3 && values[0] == 11 && values[1] == 77 && values[2] == 44, "owner array update");
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
