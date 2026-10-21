/* Portable state contract: no SDK/SDL startup; native allocation and epoch exhaustion injected. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
static int fail;
static void *allocate(size_t n, size_t size)
{
    return fail ? 0 : calloc(n, size);
}
#define calloc allocate
#include "../common/resource_state.c"
#undef calloc
#define CHECK(x)                                                                                   \
    do                                                                                             \
    {                                                                                              \
        if (!(x))                                                                                  \
        {                                                                                          \
            fprintf(stderr, "player-state:%d %s\n", __LINE__, #x);                                 \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)
static int query(FVM_RDispatch *d, FVM_RHandle target, unsigned long id, FVM_RReply *reply)
{
    FVM_RRequest request;
    size_t done;
    memset(&request, 0, sizeof(request));
    request.target = target;
    request.session = 1;
    request.id = id;
    request.operation = FVM_DESKTOP_STATE_SNAPSHOT;
    return fvm_rdispatch_post(d, &request) == FVM_R_OK &&
           fvm_rdispatch_pump(d, 1, &done) == FVM_R_OK && done == 1 &&
           fvm_rdispatch_take(d, 1, id, reply) == FVM_R_OK;
}
int main(void)
{
    FVM_RDispatch *d = 0;
    FVM_DesktopResourceState *adapter = 0, *saved;
    FVM_RHandle target, unchanged;
    FVM_RWatch a, b;
    FVM_REvent event, other;
    FVM_DesktopPlaybackState state;
    FVM_DesktopResourceSnapshot snapshot, before;
    FVM_RReply reply;
    FVM_RRequest request;
    unsigned char payload[FVM_R_BYTES];
    char source[FVM_DESKTOP_STATE_SOURCE + 2];
    size_t i, n, done;
    CHECK(fvm_rdispatch_create(0, &d) == FVM_R_OK);
    memset(&target, 0xa5, sizeof(target));
    unchanged = target;
    saved = adapter;
    fail = 1;
    CHECK(fvm_desktop_resource_state_create(d, "/timeline/state", &adapter, &target) ==
              FVM_R_NOMEM &&
          adapter == saved && !memcmp(&target, &unchanged, sizeof(target)));
    fail = 0;
    CHECK(fvm_desktop_resource_state_create(d, "/timeline/state", &adapter, &target) == FVM_R_OK);
    CHECK(fvm_rdispatch_grant(d, target, 1, FVM_R_READ | FVM_R_SUBSCRIBE) == FVM_R_OK);
    CHECK(fvm_rdispatch_grant(d, target, 2, FVM_R_SUBSCRIBE) == FVM_R_OK);
    CHECK(fvm_rdispatch_subscribe(d, 1, target, &a) == FVM_R_OK &&
          fvm_rdispatch_subscribe(d, 2, target, &b) == FVM_R_OK);
    CHECK(query(d, target, 1, &reply) && reply.result == FVM_R_BUSY);
    memset(&state, 0, sizeof(state));
    state.struct_size = sizeof(state);
    state.source = "memory://host/main.swf";
    state.phase = FVM_DESKTOP_PLAYBACK_TURN;
    CHECK(fvm_desktop_resource_state_observe(adapter, &state) == FVM_R_BUSY);
    state.phase = FVM_DESKTOP_PLAYBACK_READY;
    state.paused = 1;
    CHECK(fvm_desktop_resource_state_observe(adapter, &state) == FVM_R_OK);
    CHECK(fvm_rdispatch_event_take(d, 1, a, &event) == FVM_R_OK &&
          event.kind == FVM_DESKTOP_STATE_READY);
    CHECK(fvm_rdispatch_event_take(d, 2, b, &other) == FVM_R_OK &&
          other.sequence == event.sequence && other.size == event.size &&
          !memcmp(other.data, event.data, event.size));
    CHECK(fvm_desktop_resource_state_read(event.data, event.size, &snapshot) == FVM_R_OK &&
          snapshot.epoch == 1 && snapshot.paused == 1 &&
          snapshot.phase == FVM_DESKTOP_PLAYBACK_READY && !strcmp(snapshot.source, state.source));
    before = snapshot;
    for (i = 0; i < event.size; ++i)
        CHECK(fvm_desktop_resource_state_read(event.data, i, &snapshot) == FVM_R_INVALID &&
              !memcmp(&snapshot, &before, sizeof(snapshot)));
    memcpy(payload, event.data, event.size);
    payload[0] = 0x82;
    CHECK(fvm_desktop_resource_state_read(payload, event.size, &snapshot) == FVM_R_INVALID);
    memcpy(payload, event.data, event.size);
    payload[event.size - 1] = 0;
    CHECK(fvm_desktop_resource_state_read(payload, event.size, &snapshot) == FVM_R_INVALID);
    state.phase = FVM_DESKTOP_PLAYBACK_TURN;
    for (i = 0; i < 100; ++i)
        CHECK(fvm_desktop_resource_state_observe(adapter, &state) == FVM_R_OK);
    CHECK(fvm_rdispatch_event_take(d, 1, a, &event) == FVM_R_PENDING);
    CHECK(query(d, target, 2, &reply) && reply.result == FVM_R_OK &&
          fvm_desktop_resource_state_read(reply.data, reply.size, &snapshot) == FVM_R_OK &&
          snapshot.phase == FVM_DESKTOP_PLAYBACK_TURN);
    state.step_pending = 1;
    CHECK(fvm_desktop_resource_state_observe(adapter, &state) == FVM_R_OK);
    CHECK(fvm_rdispatch_event_take(d, 1, a, &event) == FVM_R_OK &&
          event.kind == FVM_DESKTOP_STATE_POLICY);
    state.phase = FVM_DESKTOP_PLAYBACK_TICK;
    state.movie_ticks = state.total_ticks = 1;
    state.presentations = 2;
    state.step_pending = 0;
    CHECK(fvm_desktop_resource_state_observe(adapter, &state) == FVM_R_OK);
    CHECK(fvm_rdispatch_event_take(d, 1, a, &event) == FVM_R_OK &&
          event.kind == FVM_DESKTOP_STATE_TICK);
    CHECK(fvm_desktop_resource_state_read(event.data, event.size, &snapshot) == FVM_R_OK &&
          snapshot.movie_ticks == 1 && snapshot.total_ticks == 1 && snapshot.presentations == 2);
    memset(source, 'x', sizeof(source) - 1);
    source[sizeof(source) - 1] = 0;
    state.source = source;
    state.phase = FVM_DESKTOP_PLAYBACK_READY;
    state.movie_ticks = 0;
    if (ULONG_MAX > FVM_SRECORD_U32)
        state.total_ticks = FVM_SRECORD_U32 + 1UL;
    CHECK(fvm_desktop_resource_state_observe(adapter, &state) == FVM_R_OK);
    CHECK(fvm_rdispatch_event_take(d, 1, a, &event) == FVM_R_OK &&
          event.kind == FVM_DESKTOP_STATE_READY);
    CHECK(fvm_desktop_resource_state_read(event.data, event.size, &snapshot) == FVM_R_OK &&
          snapshot.epoch == 2 && strlen(snapshot.source) == FVM_DESKTOP_STATE_SOURCE &&
          (snapshot.flags & FVM_DESKTOP_STATE_SOURCE_TRUNCATED));
    if (ULONG_MAX > FVM_SRECORD_U32)
        CHECK(snapshot.total_ticks == FVM_SRECORD_U32 &&
              (snapshot.flags & FVM_DESKTOP_STATE_COUNTERS_TRUNCATED));
    before = adapter->snapshot;
    state.paused = 2;
    CHECK(fvm_desktop_resource_state_observe(adapter, &state) == FVM_R_INVALID &&
          !memcmp(&before, &adapter->snapshot, sizeof(before)));
    state.paused = 1;
    adapter->snapshot.epoch = FVM_SRECORD_U32;
    before = adapter->snapshot;
    CHECK(fvm_desktop_resource_state_observe(adapter, &state) == FVM_R_LIMIT &&
          !memcmp(&before, &adapter->snapshot, sizeof(before)));
    adapter->snapshot.epoch = 2;
    memset(&request, 0, sizeof(request));
    request.session = 1;
    request.id = 3;
    request.target = target;
    request.operation = 1;
    request.data = "x";
    request.size = 1;
    CHECK(fvm_rdispatch_post(d, &request) == FVM_R_OK &&
          fvm_rdispatch_pump(d, 1, &done) == FVM_R_OK &&
          fvm_rdispatch_take(d, 1, 3, &reply) == FVM_R_OK && reply.result == FVM_R_INVALID);
    request.size = 0;
    request.id = 4;
    CHECK(fvm_rdispatch_post(d, &request) == FVM_R_OK);
    CHECK(fvm_desktop_resource_state_destroy(&adapter) == FVM_R_OK && !adapter);
    CHECK(fvm_rdispatch_pump(d, 1, &n) == FVM_R_OK &&
          fvm_rdispatch_take(d, 1, 4, &reply) == FVM_R_OK && reply.result == FVM_R_UNKNOWN);
    CHECK(fvm_rdispatch_event_take(d, 2, b, &event) == FVM_R_UNKNOWN &&
          fvm_rdispatch_destroy(&d) == FVM_R_OK);
    puts("PASS copied player state, independent events, quiet turns, epochs, strict decoding and "
         "teardown");
    return 0;
}
