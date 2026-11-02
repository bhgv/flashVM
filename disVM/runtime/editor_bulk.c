/* Independent bounded geometry egress, one ordinary Dis owner per private port. */
#include "lib9.h"
#include "isa.h"
#include "interp.h"
#include "disvm/dis_ui_channel.h"
#include "disvm/ui_pump.h"
#include "editor_bulk.h"
#include "editor_view.h"
#include "editor_thread.h"
#include <stdio.h>
#include "desktop_view.h"

#define BULK_SLOTS 8 /* Stable source-pane identities; no native addresses on wire. */

/* Each port has its own backpressure state and queue pair. Only the scheduler
 * owner touches these fields; channel roots belong to the live Limbo sender. */
typedef struct BulkPort
{
    DVM_UIDisEndpoint endpoint; /* [GC:] Borrowed rooted egress; ingress is unused. */
    DVM_UIBridge *bridge;       /* Owned private FlashVM queue pair. */
    int trace;                  /* Opt-in metadata-only transport diagnostics. */
    DVM_UIPump *pump;           /* Owned single-record bounded router. */
} BulkPort;

static BulkPort ports[BULK_SLOTS]; /* Independent slot registries, initially detached. */

/* Release one owner's native resources, including partial construction. port
 * is exclusively borrowed; no Dis call or managed reference release occurs. */
static void release_port(BulkPort *port)
{
    port->endpoint.replies = nil;
    if (port->pump)
    {
        dvm_ui_pump_stop(port->pump);
        dvm_ui_pump_destroy(port->pump);
    }
    dvm_ui_destroy(port->bridge);
    memset(port, 0, sizeof(*port));
}


/* Register the statically typed port at a checked bounded identity. See header
 * for root lifetime. Duplicate channels/registrations refuse without mutation;
 * a failed private broker construction releases only its partial native state. */
int dvm_editor_bulk_attach(int slot, Channel *output)
{
    BulkPort *port;
    unsigned int index;
    if (slot < 1 || slot > BULK_SLOTS)
    {
        return -1;
    }
    port = &ports[slot - 1];
    if (output == nil || output == H)
    {
        release_port(port);
        return 0;
    }
    if (port->bridge || output->mover != movp)
    {
        return -1;
    }
    for (index = 0; index < BULK_SLOTS; index++)
    {
        if (ports[index].endpoint.replies == output)
        {
            return -1;
        }
    }
    port->trace = getenv("DVM_EDITOR_BULK_STATS") != NULL;
    port->endpoint.replies = output;
    if (dvm_ui_create(0, &port->bridge) != FVM_OK ||
        dvm_ui_pump_create(port->bridge, dvm_ui_dis_pump_offer, dvm_ui_dis_pump_take, &port->endpoint, &port->pump) !=
            FVM_OK)
    {
        release_port(port);
        return -1;
    }
    return 0;
}


/* Fair one-record visits keep any large sender from occupying a different
 * slot's broker or the ordinary input/control queue. Delivery completes in this
 * poll before the unblocked sender can publish its scalar acknowledgement. */
int dvm_editor_bulk_poll(void)
{
    unsigned int index;
    int progress = 0;
    for (index = 0; index < BULK_SLOTS; index++)
    {
        BulkPort *port = &ports[index];
        DVM_UIPumpState state;
        DVM_UIMessage message;
        FVM_Result result;
        if (!port->pump)
        {
            continue;
        }
        result = dvm_ui_pump_step(port->pump, 1, &state);
        if (result != FVM_OK && result != FVM_NOT_FOUND)
        {
            return -1;
        }
        progress |= result == FVM_OK;
        result = dvm_ui_receive(port->bridge, DVM_UI_UI, &message, 0);
        if (result == FVM_NOT_FOUND)
        {
            continue;
        }
        if (result != FVM_OK || message.kind != DVM_UI_VIEW || message.code != DVM_DESK_VECTOR_DATA ||
            message.first != (long)index + 1 || !dvm_editor_deliver(&message))
        {
            return -1;
        }
        if (port->trace)
        {
            printf("BULK %lu %ld %ld %lu\n", message.flags, message.first, message.second, (unsigned long)message.size);
        }
        progress = 1;
    }
    return progress;
}


/* Tear down after routing stops, without entering Dis or touching borrowed roots. */
void dvm_editor_bulk_stop(void)
{
    unsigned int index;
    for (index = 0; index < BULK_SLOTS; index++)
    {
        release_port(&ports[index]);
    }
}
