/* Bind ordinary Dis channels to the owner-local FlashVM presentation queues. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "raise.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "idehostmod.h"
#include "disvm/dis_ui_channel.h"
#include "disvm/ui_pump.h"
#include "disvm/ui_client.h"
#include "editor_view.h"
#include "editor_ink.h"
#include "editor_bulk.h"
#include "editor_thread.h"
#include "sys_capture.h"
#include <stddef.h>

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

/* Register one typed private geometry sender, or explicitly detach with nil.
 * frame is borrowed under the same Dis owner as attach. Cross-use of ordinary
 * input/output ports is refused; the sender stack roots any admitted channel. */
void Idehost_attachbulk(void *frame)
{
    F_Idehost_attachbulk *arguments = frame;
    WORD result = -1;
    if (arguments->output == nil || (arguments->output != endpoint.requests && arguments->output != endpoint.replies))
    {
        result = dvm_editor_bulk_attach(arguments->slot, arguments->output);
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

/* Transfer a malloc-owned native copy into the current Dis result slot.
 * The original generated frame roots target while the only Dis owner is parked.
 * Release host storage on both allocation failure and normal return. */
static void return_owned_text(char *text, String **target)
{
    if (waserror())
    {
        free(text);
        nexterror();
    }
    retstr(text,target);
    poperror();
    free(text);
}


/* Return initial renderer line capacity in the generated native-cell result slot.
 * frame is borrowed; this reports actual font geometry without changing any document. */
void Idehost_rows(void *frame)
{
    F_Idehost_rows captured;
    F_Idehost_rows *arguments = &captured;
    dvm_sys_capture(frame,arguments,sizeof(*arguments),offsetof(F_Idehost_rows,ret),sizeof(*arguments->ret));
    long copied = 0;
    WORD rows;
    if (dvm_editor_threaded())
    {
        if (!dvm_editor_thread_rpc(DVM_EDITOR_RPC_ROWS,0,NULL,0,&copied,NULL))
        {
            error("UI rows request failed");
        }
    }
    else
    {
        copied = dvm_editor_rows();
    }
    rows = (WORD)copied;
    memmove(arguments->ret, &rows, sizeof(rows));
}

/* Copy the initial line-number preference into the generated native-cell result slot.
 * frame is borrowed; application visibility is subsequently controlled only by Dis envelopes. */
void Idehost_linenumbers(void *frame)
{
    F_Idehost_linenumbers captured;
    F_Idehost_linenumbers *arguments = &captured;
    dvm_sys_capture(frame,arguments,sizeof(*arguments),offsetof(F_Idehost_linenumbers,ret),sizeof(*arguments->ret));
    long copied = 0;
    WORD visible;
    if (dvm_editor_threaded())
    {
        if (!dvm_editor_thread_rpc(DVM_EDITOR_RPC_LINES,0,NULL,0,&copied,NULL))
        {
            error("UI line preference request failed");
        }
    }
    else
    {
        copied = dvm_editor_line_numbers();
    }
    visible = (WORD)copied;
    memmove(arguments->ret, &visible, sizeof(visible));
}

/* Return the explicit new-document flag in a matching generated native-cell slot.
 * frame borrows the builtin invocation; no heap mutation or native I/O occurs. */
void Idehost_fresh(void *frame)
{
    F_Idehost_fresh captured;
    F_Idehost_fresh *arguments = &captured;
    dvm_sys_capture(frame,arguments,sizeof(*arguments),offsetof(F_Idehost_fresh,ret),sizeof(*arguments->ret));
    long copied = 0;
    WORD fresh;
    if (dvm_editor_threaded())
    {
        if (!dvm_editor_thread_rpc(DVM_EDITOR_RPC_FRESH,0,NULL,0,&copied,NULL))
        {
            error("UI startup request failed");
        }
    }
    else
    {
        copied = dvm_editor_fresh();
    }
    fresh = (WORD)copied;
    memmove(arguments->ret, &fresh, sizeof(fresh));
}

/* Return an independently Dis-owned configuration string in the generated frame.
 * frame is borrowed; retstr handles reference replacement and Dis allocation errors. */
void Idehost_path(void *frame)
{
    F_Idehost_path captured;
    F_Idehost_path *arguments = &captured;
    dvm_sys_capture(frame,arguments,sizeof(*arguments),offsetof(F_Idehost_path,ret),sizeof(*arguments->ret));
    char *copied = NULL;
    if (dvm_editor_threaded())
    {
        if (!dvm_editor_thread_rpc(DVM_EDITOR_RPC_PATH,0,NULL,0,NULL,&copied))
        {
            error("UI path request failed");
        }
        return_owned_text(copied,arguments->ret);
    }
    else
    {
        retstr((char *)dvm_editor_path(), arguments->ret);
    }
}

/* Initialize the sole native UI owner before loading application modules.
 * Returns one on success; caller stops routing before destroying either VM. */
int dvm_editor_transport_prepare(void)
{
    return dvm_ui_create(0,&transport) == FVM_OK && dvm_ui_client_create(transport,&client) == FVM_OK;
}


/* Borrow a native queue pair, whose lifetime is bounded by worker join. */
DVM_UIBridge *dvm_editor_transport(void)
{
    return transport;
}


/* Free main-owned queue/client only after the Dis owner has stopped. */
void dvm_editor_transport_finish(void)
{
    dvm_ui_client_destroy(client);
    client = NULL;
    dvm_ui_destroy(transport);
    transport = NULL;
}


/* Register only on the Dis owner; create UI on main through a copied RPC. */
int dvm_editor_start(const char *directory)
{
    builtinmod("$Idehost", Idehostmodtab, Idehostmodlen);
    if ((!transport && !dvm_editor_transport_prepare()) ||
        dvm_ui_pump_create(transport,dvm_ui_dis_pump_offer,dvm_ui_dis_pump_take,&endpoint,&pump) != FVM_OK)
    {
        return 0;
    }
    if (dvm_editor_threaded())
    {
        return dvm_editor_thread_rpc(DVM_EDITOR_RPC_START,0,directory,strlen(directory),NULL,NULL);
    }
    return dvm_editor_default_font(directory) && dvm_editor_create();
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
    FVM_Result result;
    int progress;
    int bulk;
    if (dvm_editor_thread_failed())
    {
        return -1;
    }
    if (!endpoint.requests)
    {
        return 0;
    }
    result = dvm_ui_pump_step(pump,16,&state);
    if (result != FVM_OK && result != FVM_NOT_FOUND)
    {
        return -1;
    }
    progress = result == FVM_OK;
    bulk = dvm_editor_bulk_poll();
    if (bulk < 0)
    {
        return -1;
    }
    progress |= bulk;
    if (!dvm_editor_threaded())
    {
        int ui = dvm_editor_ui_poll(0);
        if (ui < 0)
        {
            return -1;
        }
        progress |= ui;
    }
    return progress;
}


/* UI main alone flushes captured input, decodes copied presentations and polls
 * SDL/FlashVM. No Dis channel mover, register, GC root or endpoint is touched. */
int dvm_editor_ui_poll(int stopping)
{
    DVM_UIMessage message;
    FVM_Result result;
    size_t sent = 0;
    unsigned int count;
    int progress = 0;
    if (!stopping)
    {
        result = dvm_ui_client_flush(client,DVM_UI_CLIENT_CAPACITY,&sent);
        if (result != FVM_OK && result != FVM_NOT_FOUND && !dvm_editor_thread_is_stopping())
        {
            return -1;
        }
        progress = sent != 0;
    }
    for (count = 0; count < DVM_UI_PENDING_MAX; ++count)
    {
        result = dvm_ui_receive(transport,DVM_UI_UI,&message,0);
        if (result == FVM_NOT_FOUND || ((stopping || dvm_editor_thread_is_stopping()) && result == FVM_ERROR))
        {
            break;
        }
        if (result != FVM_OK || !dvm_editor_present(&message))
        {
            return -1;
        }
        progress = 1;
    }
    if (!stopping && !dvm_editor_thread_is_stopping())
    {
        int event = dvm_editor_event(&message);
        if (event < 0 || (event && dvm_ui_client_post(client,&message) != FVM_OK))
        {
            return dvm_editor_thread_is_stopping() ? progress : -1;
        }
        progress |= event;
    }
    return progress;
}

/* Detach borrowed roots only after all Dis processes stop; release native queue/view owners.
 * Also accepts partial initialization on failure. No channel callback runs during teardown. */
void dvm_editor_stop(void)
{
    dvm_editor_thread_stopping();
    endpoint.requests = nil;
    endpoint.replies = nil;
    if (pump)
    {
        dvm_ui_pump_stop(pump);
        dvm_ui_pump_destroy(pump);
        pump = NULL;
    }
    dvm_editor_bulk_stop();
    if (!dvm_editor_threaded())
    {
        dvm_editor_transport_finish();
        dvm_editor_destroy();
    }
}

/* [GC:] retstr installs a private Dis string in the generated mapped return slot.
 * The borrowed native frozen buffer remains registered on allocation failure. */
void Idehost_ink(void *frame)
{
    F_Idehost_ink captured;
    F_Idehost_ink *arguments = &captured;
    dvm_sys_capture(frame,arguments,sizeof(*arguments),offsetof(F_Idehost_ink,ret),sizeof(*arguments->ret));
    char *copied = NULL;
    if (dvm_editor_threaded())
    {
        if (!dvm_editor_thread_rpc(DVM_EDITOR_RPC_INK,(unsigned long)arguments->token,NULL,0,NULL,&copied))
        {
            error("native ink snapshot request failed");
        }
        return_owned_text(copied,arguments->ret);
    }
    else
    {
        retstr((char *)dvm_ink_read((unsigned long)arguments->token), arguments->ret);
    }
}

/* Borrow a rooted managed input only during this native call. Copy hash/extent,
 * never the String address, into the native pending-presentation registry. */
void Idehost_inkdone(void *frame)
{
    F_Idehost_inkdone captured;
    F_Idehost_inkdone *arguments = &captured;
    if (!frame || frame == H)
    {
        error(exNilref);
    }
    memmove(arguments,frame,sizeof(*arguments));
    char *bytes = string2c(arguments->drawing);
    if (dvm_editor_threaded())
    {
        if (!dvm_editor_thread_rpc(DVM_EDITOR_RPC_ACK,(unsigned long)arguments->token,bytes,strlen(bytes),NULL,NULL))
        {
            error("native ink acknowledgement failed");
        }
    }
    else
    {
        dvm_ink_ack((unsigned long)arguments->token,bytes,strlen(bytes));
    }
}

/* Apply scalar document admission before any live native samples are consumed.
 * Refusal retires only that capture; no document or VM reference is retained. */
void Idehost_inkadmit(void *frame)
{
    F_Idehost_inkadmit captured;
    F_Idehost_inkadmit *arguments = &captured;
    if (!frame || frame == H)
    {
        error(exNilref);
    }
    memmove(arguments,frame,sizeof(*arguments));
    if (arguments->accepted != 1)
    {
        if (dvm_editor_threaded())
        {
            if (!dvm_editor_thread_rpc(DVM_EDITOR_RPC_CANCEL,(unsigned long)arguments->token,NULL,0,NULL,NULL))
            {
                error("native ink cancellation failed");
            }
        }
        else
        {
            dvm_ink_cancel((unsigned long)arguments->token);
        }
    }
}
