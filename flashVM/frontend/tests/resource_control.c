/* Native adapter contract without SDK startup: delivery acknowledgements are not frame commits. */
#include "../common/resource_control.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x)                                                                                   \
    do                                                                                             \
    {                                                                                              \
        if (!(x))                                                                                  \
        {                                                                                          \
            fprintf(stderr, "resource adapter line %d: %s\n", __LINE__, #x);                       \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)
/* Only a playback notify may deliver a queued control operation; external pumping returns BUSY. */
int main(void)
{
    FVM_RDispatch *dispatch = 0;
    FVM_DesktopResourceControl adapter, failed, before;
    FVM_DesktopPlaybackState state;
    FVM_RRequest request;
    FVM_RReply reply;
    size_t done;
    unsigned int i;
    unsigned char byte = 1;
    static const unsigned long operations[] = {
        FVM_DESKTOP_RESOURCE_PAUSE, FVM_DESKTOP_RESOURCE_RESUME, FVM_DESKTOP_RESOURCE_STEP,
        FVM_DESKTOP_RESOURCE_STOP};
    static const FVM_DesktopPlaybackCommand commands[] = {
        FVM_DESKTOP_PLAYBACK_PAUSE, FVM_DESKTOP_PLAYBACK_RESUME, FVM_DESKTOP_PLAYBACK_STEP,
        FVM_DESKTOP_PLAYBACK_STOP};
    CHECK(fvm_rdispatch_create(0, &dispatch) == FVM_R_OK);
    CHECK(fvm_desktop_resource_control_init(&adapter, dispatch, "/timeline/ctl") == FVM_R_OK);
    memset(&failed, 0xa5, sizeof(failed));
    memcpy(&before, &failed, sizeof(failed));
    CHECK(fvm_desktop_resource_control_init(&failed, dispatch, "/timeline/ctl") == FVM_R_CONFLICT &&
          !memcmp(&before, &failed, sizeof(failed)));
    CHECK(fvm_rdispatch_grant(dispatch, adapter.target, 1, FVM_R_CONTROL) == FVM_R_OK);
    memset(&state, 0, sizeof(state));
    state.struct_size = sizeof(state);
    state.phase = FVM_DESKTOP_PLAYBACK_TURN;
    memset(&request, 0, sizeof(request));
    request.session = 1;
    request.target = adapter.target;
    request.id = 1;
    request.operation = FVM_DESKTOP_RESOURCE_STEP;
    CHECK(fvm_rdispatch_post(dispatch, &request) == FVM_R_OK);
    CHECK(fvm_rdispatch_pump(dispatch, 1, &done) == FVM_R_OK && done == 1);
    CHECK(fvm_rdispatch_take(dispatch, 1, 1, &reply) == FVM_R_OK && reply.result == FVM_R_BUSY);
    CHECK(fvm_desktop_resource_control_notify(&state, &adapter) == FVM_DESKTOP_PLAYBACK_KEEP);
    for (i = 0; i < 4; ++i)
    {
        request.id = i + 2;
        request.operation = operations[i];
        CHECK(fvm_rdispatch_post(dispatch, &request) == FVM_R_OK);
        CHECK(fvm_desktop_resource_control_notify(&state, &adapter) == commands[i]);
        CHECK(fvm_rdispatch_take(dispatch, 1, request.id, &reply) == FVM_R_OK &&
              reply.result == FVM_R_OK && !reply.size);
    }
    request.id = 6;
    request.size = 1;
    request.data = &byte;
    request.operation = FVM_DESKTOP_RESOURCE_STEP;
    CHECK(fvm_rdispatch_post(dispatch, &request) == FVM_R_OK);
    CHECK(fvm_desktop_resource_control_notify(&state, &adapter) == FVM_DESKTOP_PLAYBACK_KEEP);
    CHECK(fvm_rdispatch_take(dispatch, 1, 6, &reply) == FVM_R_OK && reply.result == FVM_R_INVALID);
    request.id = 7;
    request.size = 0;
    CHECK(fvm_rdispatch_post(dispatch, &request) == FVM_R_OK);
    state.phase = (FVM_DesktopPlaybackPhase)0;
    CHECK(fvm_desktop_resource_control_notify(&state, &adapter) == FVM_DESKTOP_PLAYBACK_STOP);
    CHECK(fvm_rdispatch_take(dispatch, 1, 7, &reply) == FVM_R_PENDING);
    CHECK(fvm_rdispatch_cancel(dispatch, 1, 7) == FVM_R_OK);
    CHECK(fvm_rdispatch_take(dispatch, 1, 7, &reply) == FVM_R_OK &&
          reply.result == FVM_R_CANCELLED);
    CHECK(fvm_rdispatch_unregister(dispatch, adapter.target) == FVM_R_OK);
    CHECK(fvm_rdispatch_destroy(&dispatch) == FVM_R_OK && !dispatch);
    puts("PASS pause/resume/step/stop delivery, external-pump rejection, invalid phase/payload and "
         "explicit native cleanup");
    return 0;
}
