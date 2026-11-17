/* Copyright (c) 2026 bhgv
 * EN: Native VFS data-operation cancellation reaches Styx Tflush without guest callbacks.
 * RU: Native VFS data-operation cancellation доходит до Styx Tflush без guest callbacks.
 */
#include "mount_cancel.h"
#include <string.h>


/* EN/RU: Idempotent native wake only; callback owns no provider or guest state.
 * Только idempotent native wake; callback не владеет provider/guest state. */
static void interrupt(void *state)
{
    styxcancelrequest(state);
}


/* EN/RU: Consume token only after every admitted source callback has returned.
 * Поглотить token после завершения всех admitted source callbacks. */
static void release(void *state)
{
    styxcancelfree(state);
}


/* EN/RU: See mount_cancel.h; subscribe/replay precedes native RPC admission.
 * См. mount_cancel.h; subscribe/replay до native RPC admission. */
int styxmountcancel(StyxClient *client, StyxMountCancel *output)
{
    memset(output, 0, sizeof(*output));
    DVM_Cancel *source = dvm_cancel_current();
    if (!source || !styxclientcancellable(client))
    {
        return 1;
    }
    StyxCancel *token = styxcancelcreate();
    if (!token)
    {
        return 0;
    }
    output->tap = dvm_cancel_subscribe(source, interrupt, token, release);
    if (!output->tap)
    {
        styxcancelfree(token);
        return 0;
    }
    output->token = token;
    return 1;
}


/* EN/RU: See mount_cancel.h; late signals retain the old token, never a later RPC.
 * См. mount_cancel.h; late signals удерживают старый token, не следующий RPC. */
void styxmountcancelend(StyxMountCancel *binding)
{
    DVM_CancelTap *tap = binding->tap;
    memset(binding, 0, sizeof(*binding));
    dvm_cancel_unsubscribe(tap);
}


/* EN/RU: See mount_cancel.h; original reply wins and no post-cancel RPC is sent.
 * См. mount_cancel.h; original reply приоритетен, после cancellation RPC не отправляется. */
int styxmountstat(StyxClient *client, StyxFid *fid, StyxMeta *output)
{
    StyxMountCancel binding;
    if (!styxmountcancel(client, &binding))
    {
        return 0;
    }
    StyxTransfer result;
    int success = binding.token ? styxclientstatcancel(fid, output, binding.token, &result) : styxclientstat(fid, output);
    styxmountcancelend(&binding);
    return success;
}


/* EN/RU: See mount_cancel.h; each logical walk uses one fresh token across fragments.
 * См. mount_cancel.h; logical walk использует fresh token на все fragments. */
StyxFid *styxmountwalkcancel(StyxClient *client, StyxFid *base, const char *path)
{
    StyxMountCancel binding;
    if (!styxmountcancel(client, &binding))
    {
        return NULL;
    }
    StyxTransfer result;
    StyxFid *fid = binding.token ?
        (base ? styxclientwalkfromcancel(base, path, binding.token, &result) : styxclientwalkcancel(client, path, binding.token, &result)) :
        (base ? styxclientwalkfrom(base, path) : styxclientwalk(client, path));
    styxmountcancelend(&binding);
    return fid;
}


/* EN/RU: See mount_cancel.h; acknowledged Ropen remains authoritative during cancellation races.
 * См. mount_cancel.h; acknowledged Ropen приоритетен при cancellation races. */
int styxmountopen(StyxClient *client, StyxFid *fid, int mode)
{
    StyxMountCancel binding;
    if (!styxmountcancel(client, &binding))
    {
        return 0;
    }
    StyxTransfer result;
    int success = binding.token ? styxclientopencancel(fid, mode, binding.token, &result) : styxclientopen(fid, mode);
    styxmountcancelend(&binding);
    return success;
}


/* EN/RU: See mount_cancel.h; late cancellation cannot force an uninterruptible cleanup RPC.
 * См. mount_cancel.h; late cancellation не вызывает uninterruptible cleanup RPC. */
void styxmountclunk(StyxClient *client, StyxFid *fid)
{
    if (!fid)
    {
        return;
    }
    StyxMountCancel binding;
    if (!styxmountcancel(client, &binding))
    {
        styxclientclunklater(fid);
        return;
    }
    if (binding.token)
    {
        styxclientclunkcancel(fid, binding.token);
    }
    else
    {
        styxclientclunk(fid);
    }
    styxmountcancelend(&binding);
}


/* EN/RU: See mount_cancel.h; cancellation setup cannot fabricate a missing remote name.
 * См. mount_cancel.h; setup cancellation не превращается в отсутствие имени. */
StyxFid *styxmountlookup(StyxClient *client, StyxFid *base, const char *path, int *missing)
{
    *missing = 0;
    StyxMountCancel binding;
    if (!styxmountcancel(client, &binding))
    {
        return NULL;
    }
    StyxFid *fid = base ? styxclientlookupfrom(base, path, binding.token, missing) :
        styxclientlookup(client, path, binding.token, missing);
    styxmountcancelend(&binding);
    return fid;
}
