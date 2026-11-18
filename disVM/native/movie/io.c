/* Copyright (c) 2026 bhgv
 * EN: Movie stream RPCs use the existing VFS native-scope-to-Styx Tflush binding.
 * RU: Movie stream RPCs используют существующий VFS binding native scope в Styx Tflush.
 */
#include "fetch_internal.h"
#include "mount_cancel.h"
#include <string.h>


/* EN/RU: See fetch_internal.h; setup failure has no RPC effect, each admitted token targets only this call.
 * См. fetch_internal.h; setup failure без RPC effects, admitted token относится только к данному call.
 */
void movie_transfer(StyxClient *client, StyxFid *fid, unsigned long long offset, void *bytes,
    size_t size, int writing, StyxTransfer *result)
{
    memset(result, 0, sizeof(*result));
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        return;
    }
    StyxMountCancel binding; /* Owns source subscription; token outlives late admitted callbacks. */
    if (!styxmountcancel(client, &binding))
    {
        return;
    }
    if (binding.token && writing)
    {
        styxclientwritecancel(fid, offset, bytes, size, binding.token, result);
    }
    else if (binding.token)
    {
        styxclientreadcancel(fid, offset, bytes, size, binding.token, result);
    }
    else if (writing)
    {
        styxclientwrite(fid, offset, bytes, size, result);
    }
    else
    {
        styxclientread(fid, offset, bytes, size, result);
    }
    styxmountcancelend(&binding);
}
