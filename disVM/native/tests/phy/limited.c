/* Copyright (c) 2026 bhgv
 * EN: Shared test-only carrier wrapper for real client/server PHY fragmentation acceptance.
 * RU: Общий test-only carrier wrapper для real client/server PHY fragmentation acceptance.
 */
#include "limited.h"
#include <stdio.h>
#include <stdlib.h>
#define FINAL_REPLY_BYTES 89U /* Independent replies:19+20+9+24+17, including all 9P headers. */


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
RemotePhy *phyfixture(RemotePhy *child, int terminal)
{
    static const PhyOps ops = {readlayer, writelayer, waitlayer, wakelayer, NULL, NULL, disconnectlayer, destroylayer};
    PhyInfo info;
    if (!phyinfo(child, &info))
    {
        phyfree(child);
        return NULL;
    }
    Layer *layer = calloc(1, sizeof(*layer));
    if (!layer)
    {
        phyfree(child);
        return NULL;
    }
    layer->child = child;
    layer->terminal = terminal;
    info.max_read = 3;
    info.max_write = 2;
    RemotePhy *phy = phycreate(&ops, layer, &info);
    if (!phy)
    {
        destroylayer(layer);
    }
    return phy;
}


