/* Bind ordinary Dis channels to the owner-local FlashVM presentation queues. */
#include "lib9.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "idehostmod.h"
#include "disvm/dis_ui_channel.h"
#include "disvm/ui_pump.h"
#include "disvm/ui_client.h"
#include "editor_view.h"

static DVM_UIDisEndpoint endpoint; /* [GC:] Borrowed ports rooted by the live Limbo init stack. */
static DVM_UIBridge *transport;    /* Owned real FlashVM queue pair, no shared VM heap. */
static DVM_UIPump *pump;           /* Owned bounded router on this sole execution owner. */
static DVM_UIClient *client;       /* Owned FIFO retains input until transport accepts it. */

/* Register one pair of statically typed byte-array ports; frame borrows builtin arguments.
 * Returns 0/-1 in the generated native-cell return slot. No GC ownership is transferred:
 * init must keep both channels rooted until all services stop and native routing detaches. */
void Idehost_attach(void *frame)
{
    F_Idehost_attach *arguments = frame;
    WORD result = -1;
    if (!endpoint.requests && arguments->input != nil && arguments->input != H && arguments->output != nil &&
        arguments->output != H && arguments->input != arguments->output && arguments->input->mover == movp &&
        arguments->output->mover == movp)
    {
        endpoint.requests = arguments->input;
        endpoint.replies = arguments->output;
        result = 0;
    }
    memmove(arguments->ret, &result, sizeof(result));
}


/* End native borrowing before Limbo releases its stack-rooted port references.
 * frame has no arguments/result. The scheduler owner makes this mutation exclusively. */
void Idehost_detach(void *frame)
{
    USED(frame);
    endpoint.requests = nil;
    endpoint.replies = nil;
}


/* Return initial renderer line capacity in the generated native-cell result slot.
 * frame is borrowed; this reports actual font geometry without changing any document. */
void Idehost_rows(void *frame)
{
    F_Idehost_rows *arguments = frame;
    WORD rows = dvm_editor_rows();
    memmove(arguments->ret, &rows, sizeof(rows));
}


/* Return the explicit new-document flag in a matching generated native-cell slot.
 * frame borrows the builtin invocation; no heap mutation or native I/O occurs. */
void Idehost_fresh(void *frame)
{
    F_Idehost_fresh *arguments = frame;
    WORD fresh = dvm_editor_fresh();
    memmove(arguments->ret, &fresh, sizeof(fresh));
}


/* Return an independently Dis-owned configuration string in the generated frame.
 * frame is borrowed; retstr handles reference replacement and Dis allocation errors. */
void Idehost_path(void *frame)
{
    F_Idehost_path *arguments = frame;
    retstr((char *)dvm_editor_path(), arguments->ret);
}


/* Initialize the sole native UI owner before loading application modules.
 * Returns one on success; caller stops routing before destroying either VM. */
int dvm_editor_start(void)
{
    builtinmod("$Idehost", Idehostmodtab, Idehostmodlen);
    if (dvm_ui_create(0, &transport) != FVM_OK || dvm_ui_client_create(transport, &client) != FVM_OK ||
        dvm_ui_pump_create(transport, dvm_ui_dis_pump_offer, dvm_ui_dis_pump_take, &endpoint, &pump) != FVM_OK)
    {
        return 0;
    }
    return dvm_editor_create();
}


/* Observe whether the live Limbo entry stack has registered its rooted ports. */
int dvm_editor_attached(void)
{
    return endpoint.requests != nil;
}


/* Route bounded copied records and native events without running Dis or blocking.
 * Requires an existing protected owner/TLS context. Returns progress, idle, or -1 on failure.
 * Production channel movers may wake ready Dis processes; their scheduler runs afterwards. */
int dvm_editor_poll(void)
{
    DVM_UIPumpState state;
    DVM_UIMessage message;
    FVM_Result result;
    size_t sent;
    int event;
    int progress = 0;
    unsigned int count;
    if (!endpoint.requests)
    {
        return 0;
    }
    result = dvm_ui_client_flush(client, DVM_UI_CLIENT_CAPACITY, &sent);
    if (result != FVM_OK && result != FVM_NOT_FOUND)
    {
        return -1;
    }
    progress = sent != 0;
    result = dvm_ui_pump_step(pump, 16, &state);
    if (result != FVM_OK && result != FVM_NOT_FOUND)
    {
        return -1;
    }
    progress |= result == FVM_OK;
    for (count = 0; count < DVM_UI_PENDING_MAX; count++)
    {
        result = dvm_ui_receive(transport, DVM_UI_UI, &message, 0);
        if (result == FVM_NOT_FOUND)
        {
            break;
        }
        if (result != FVM_OK || !dvm_editor_present(&message))
        {
            return -1;
        }
        progress = 1;
    }
    event = dvm_editor_event(&message);
    if (event < 0 || (event && dvm_ui_client_post(client, &message) != FVM_OK))
    {
        return -1;
    }
    return progress || event;
}


/* Detach borrowed roots only after all Dis processes stop; release native queue/view owners.
 * Also accepts partial initialization on failure. No channel callback runs during teardown. */
void dvm_editor_stop(void)
{
    endpoint.requests = nil;
    endpoint.replies = nil;
    if (pump)
    {
        dvm_ui_pump_stop(pump);
        dvm_ui_pump_destroy(pump);
    }
    dvm_ui_client_destroy(client);
    dvm_ui_destroy(transport);
    dvm_editor_destroy();
}
