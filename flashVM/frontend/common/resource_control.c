/* Bridge native queued resource commands to the established synchronous playback policy boundary.
 */
#include "resource_control.h"
#include <string.h>


/* Only the player's own notify may translate a delivered command into its returned policy action.
 */
static FVM_RResult deliver(const FVM_RRequest *request, void *output, size_t capacity, size_t *used, void *raw)
{
    FVM_DesktopResourceControl *adapter = (FVM_DesktopResourceControl *)raw;
    (void)output;
    (void)capacity;
    *used = 0;
    if (!adapter->notifying)
    {
        return FVM_R_BUSY;
    }
    if (request->size)
    {
        return FVM_R_INVALID;
    }
    switch (request->operation)
    {
    case FVM_DESKTOP_RESOURCE_PAUSE:
        adapter->pending = FVM_DESKTOP_PLAYBACK_PAUSE;
        break;
    case FVM_DESKTOP_RESOURCE_RESUME:
        adapter->pending = FVM_DESKTOP_PLAYBACK_RESUME;
        break;
    case FVM_DESKTOP_RESOURCE_STEP:
        adapter->pending = FVM_DESKTOP_PLAYBACK_STEP;
        break;
    case FVM_DESKTOP_RESOURCE_STOP:
        adapter->pending = FVM_DESKTOP_PLAYBACK_STOP;
        break;
    default:
        return FVM_R_UNKNOWN;
    }
    return FVM_R_OK;
}


/* Publish native adapter state only after complete owner registration succeeds. */
FVM_RResult
fvm_desktop_resource_control_init(FVM_DesktopResourceControl *adapter, FVM_RDispatch *dispatch, const char *path)
{
    static const FVM_ROperation operations[] = {{FVM_DESKTOP_RESOURCE_PAUSE, FVM_R_CONTROL},
                                                {FVM_DESKTOP_RESOURCE_RESUME, FVM_R_CONTROL},
                                                {FVM_DESKTOP_RESOURCE_STEP, FVM_R_CONTROL},
                                                {FVM_DESKTOP_RESOURCE_STOP, FVM_R_CONTROL}};
    FVM_RHandle target;
    FVM_RResult result;
    if (!adapter || !dispatch)
    {
        return FVM_R_INVALID;
    }
    result = fvm_rdispatch_register(
        dispatch, path, operations, sizeof(operations) / sizeof(operations[0]), deliver, adapter, &target);
    if (result != FVM_R_OK)
    {
        return result;
    }
    memset(adapter, 0, sizeof(*adapter));
    adapter->struct_size = sizeof(*adapter);
    adapter->dispatch = dispatch;
    adapter->target = target;
    return FVM_R_OK;
}


/* No managed state is retained; player applies the returned command after this callback returns. */
FVM_DesktopPlaybackCommand fvm_desktop_resource_control_notify(const FVM_DesktopPlaybackState *state, void *raw)
{
    FVM_DesktopResourceControl *adapter = (FVM_DesktopResourceControl *)raw;
    size_t completed;
    FVM_RResult result;
    if (!state || state->struct_size != sizeof(*state) || !adapter || adapter->struct_size != sizeof(*adapter) ||
        !adapter->dispatch || adapter->notifying ||
        (state->phase != FVM_DESKTOP_PLAYBACK_READY && state->phase != FVM_DESKTOP_PLAYBACK_TURN &&
         state->phase != FVM_DESKTOP_PLAYBACK_TICK))
    {
        return FVM_DESKTOP_PLAYBACK_STOP;
    }
    adapter->pending = FVM_DESKTOP_PLAYBACK_KEEP;
    adapter->notifying = 1;
    result = fvm_rdispatch_pump(adapter->dispatch, 1, &completed);
    adapter->notifying = 0;
    return result == FVM_R_OK ? adapter->pending : FVM_DESKTOP_PLAYBACK_STOP;
}
