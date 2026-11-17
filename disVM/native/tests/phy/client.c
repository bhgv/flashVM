/* Copyright (c) 2026 bhgv
 * EN: Real native Styx engine over a granted nonsocket PTY, including layered chunking and a final confirmed-prefix error.
 * RU: Настоящий native Styx engine через granted nonsocket PTY, включая layered chunking и final confirmed-prefix error.
 */
#include "client.h"
#include "drivers/posix/fd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FINAL_REPLY_BYTES 89U /* Independent replies:19+20+9+24+17, including all 9P headers. */


/* EN/RU: Explicit active invariant, without assert; diagnostic borrowed.
 * Явный active invariant без assert; diagnostic borrowed. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "PHY Styx: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Test-only stacked byte carrier; no 9P decoding, counters belong to native owner.
 * Test-only stacked byte carrier без 9P decoding; counters принадлежат native owner. */
typedef struct Layer
{
    RemotePhy *child; /* Owned lower carrier. */
    size_t received; /* Counted confirmed bytes. */
    int terminal; /* Inject failure at the final known independent response boundary. */
} Layer;


/* EN/RU: Read exact lower prefixes; an error following a full reply must preserve its acknowledgement.
 * Читать lower prefixes; ошибка после полного reply должна сохранить acknowledgement. */
static PhyTransfer readlayer(void *state, void *bytes, size_t size)
{
    Layer *layer = state;
    PhyTransfer out = phyread(layer->child, bytes, size);
    layer->received += out.count;
    if (layer->terminal && layer->received == FINAL_REPLY_BYTES && out.count)
    {
        out.status = PHY_ERROR;
    }
    return out;
}


/* EN/RU: Forward one borrowed write with no replay or packet interpretation.
 * Передать один borrowed write без replay/packet interpretation. */
static PhyTransfer writelayer(void *state, const void *bytes, size_t size)
{
    return phywrite(((Layer *)state)->child, bytes, size);
}


/* EN/RU: Forward native wait; no descriptor appears at this adapter boundary.
 * Передать native wait; на границе adapter нет descriptor. */
static int waitlayer(void *state, unsigned wanted, int timeout, unsigned *ready)
{
    return phywait(((Layer *)state)->child, wanted, timeout, ready);
}


/* EN/RU: Forward nonwaiting native interrupt through immutable lower-carrier pin.
 * Передать native interrupt под immutable lower-carrier pin. */
static void wakelayer(void *state)
{
    phywake(((Layer *)state)->child);
}


/* EN/RU: Close lower wire once; callback state remains alive for wake until destroy.
 * Закрыть lower wire один раз; state остаётся для wake до destroy. */
static void disconnectlayer(void *state)
{
    phydisconnect(((Layer *)state)->child);
}


/* EN/RU: Consume lower carrier and native wrapper after all I/O/wake users retire.
 * Поглотить lower carrier и wrapper после I/O/wake users. */
static void destroylayer(void *state)
{
    Layer *layer = state;
    phyfree(layer->child);
    free(layer);
}


/* EN/RU: Consume lower carrier on every outcome; cap reads3/writes2 to exercise real frame segmentation.
 * Принять lower carrier всегда; cap reads3/writes2 проверяет actual frame segmentation. */
static RemotePhy *stack(RemotePhy *child, int terminal)
{
    static const PhyOps ops = {readlayer, writelayer, waitlayer, wakelayer, NULL, NULL, disconnectlayer, destroylayer};
    Layer *layer = calloc(1, sizeof(*layer));
    if (!layer)
    {
        phyfree(child);
        return NULL;
    }
    layer->child = child;
    layer->terminal = terminal;
    PhyInfo info = {PHY_STREAM|PHY_ORDERED|PHY_RELIABLE, 0, 3, 2, 1};
    RemotePhy *phy = phycreate(&ops, layer, &info);
    if (!phy)
    {
        destroylayer(layer);
    }
    return phy;
}


/* EN/RU: Read one original 9P file over nonsocket carrier; optional pending watch is cancelled by host.
 * Читать original 9P file через nonsocket carrier; pending watch отменяется host. */
int main(int argc, char **argv)
{
    check(argc == 3, "inherited PTY descriptor/mode");
    int reject = !strcmp(argv[2], "reject");
    RemotePhy *phy = phyposixfd(atoi(argv[1]), PHY_STREAM|PHY_ORDERED|(reject ? 0 : PHY_RELIABLE));
    check(phy != NULL, "granted PTY driver");
    if (!reject)
    {
        phy = stack(phy, !strcmp(argv[2], "prefix-error"));
        check(phy != NULL, "stacked carrier");
    }
    StyxClient *client = styxclientmountphy(phy, 256, "fixture", "", 3000, 4);
    if (reject)
    {
        check(!client, "unreliable carrier admitted to 9P");
        puts("PASS unreliable PHY rejected before 9P wire effects");
        return 0;
    }
    check(client && styxclientwatchable(client), "carrier-neutral native mount");
    StyxFid *fid = styxclientwalk(client, ".");
    check(fid && styxclientopen(fid, 0), "open remote file");
    char bytes[32];
    StyxTransfer result;
    styxclientread(fid, 0, bytes, sizeof(bytes), &result);
    check(result.success && result.confirmed && result.count == 6 && !memcmp(bytes, "serial", 6), "remote PTY bytes");
    if (!strcmp(argv[2], "prefix-error"))
    {
        check(!styxclientalive(client), "final carrier error was ignored");
    }
    styxclientclunk(fid);
    styxclientshutdown(client);
    styxclientfree(client);
    puts("PASS real native Styx transactions over nonsocket PTY PHY");
    return 0;
}
