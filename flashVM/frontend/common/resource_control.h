/* Resource commands delivered at existing player owner safe points, without worker VM access. */
#ifndef FVM_DESKTOP_RESOURCE_CONTROL_H
#define FVM_DESKTOP_RESOURCE_CONTROL_H /* Guard additive native player control adapter. */
#include "playback.h"
#include "flashvm/rdispatch.h"

#define FVM_DESKTOP_RESOURCE_PAUSE 1UL  /* Empty payload: request host pause. */
#define FVM_DESKTOP_RESOURCE_RESUME 2UL /* Empty payload: request continuous host playback. */
#define FVM_DESKTOP_RESOURCE_STEP 3UL   /* Empty payload: request one host timeline step. */
#define FVM_DESKTOP_RESOURCE_STOP 4UL   /* Empty payload: terminal source-run stop. */
#ifdef __cplusplus
extern "C"
{
#endif
/* Caller-owned adapter; keep stable until resource unregister and synchronous runner return. */
typedef struct FVM_DesktopResourceControl
{
    unsigned long struct_size;          /* Exact adapter ABI size filled on successful init. */
    FVM_RDispatch *dispatch;            /* Borrowed native dispatcher; no implicit ownership/worker. */
    FVM_RHandle target;                 /* Registered exact control resource generation. */
    int notifying;                      /* Guard allowing delivery only while this adapter's notify is pumping. */
    FVM_DesktopPlaybackCommand pending; /* One command to return at this safe point, not a queue. */
} FVM_DesktopResourceControl;


/* Trusted owner registers PAUSE/RESUME/STEP/STOP as CONTROL-right operations under caller path.
     * On failure adapter/out handle remain unchanged. Caller grants sessions explicitly, then
     * installs resource_control_notify/userdata in an ordinary playback control descriptor. No SDK
     * startup. Do not reinitialize a live registered adapter; unregister target before releasing
     * its storage. */
FVM_DESKTOP_API FVM_RResult fvm_desktop_resource_control_init(FVM_DesktopResourceControl *,
                                                              FVM_RDispatch *,
                                                              const char *path);


/* Compatible playback notify. Pump at most one FIFO job at each owner boundary. The four
     * operations are delivered only during this notify; external pumping completes them BUSY
     * without commands. Invalid state/adapter or failed pump returns STOP. Reply OK means command
     * delivered to the player policy, not yet an executed tick/frame acknowledgement. Use
     * subsequent playback/frame snapshots for that. Do not retain state/context/root; adapter
     * performs no VM calls or pointer transfer. */
FVM_DESKTOP_API FVM_DesktopPlaybackCommand fvm_desktop_resource_control_notify(const FVM_DesktopPlaybackState *,
                                                                               void *adapter);
#ifdef __cplusplus
}
#endif
#endif
