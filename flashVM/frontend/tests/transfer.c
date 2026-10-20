/* Worker publication, copied text, sink latching, cancellation, quota and joined SDK lifetime. */
#include "player.h"
#include <SDL.h>
#include <assert.h>
#include <string.h>

typedef struct State {
    SDL_sem *gate;
    FVM_DesktopTransfer *job;
    int mode;
    SDL_threadID owner, worker;
    FVM_Result self_release;
} State;
static FVM_Result provider(const char *reference, const char *base, FVM_DesktopWrite write,
    void *sink, char *resolved, size_t capacity, const FVM_NetControl *control, void *userdata)
{
    State *state = (State *)userdata;
    const unsigned char data[] = {'a', 0, 'b', 255};
    FVM_Result result;
    (void)capacity;
    if (state) {
        state->worker = SDL_ThreadID();
        if (state->gate) SDL_SemWait(state->gate);
        assert(state->worker != state->owner);
        assert(!strcmp(reference, "http://copied/start"));
        assert(!strcmp(base, "http://copied/base"));
        if (state->mode == 6) {
            while (!control->cancelled(control->userdata)) SDL_Delay(1);
            return FVM_ERROR;
        }
        if (state->mode == 7) {
            state->self_release = fvm_desktop_transfer_release(state->job);
            SDL_Delay(30);
        }
        if (state->mode == 4) return FVM_OK; /* Missing canonical output. */
        if (state->mode == 5) memset(resolved, 'X', capacity); /* Missing NUL. */
        else strcpy(resolved, "http://canonical/resource");
        if (state->mode == 8) return FVM_OK; /* An empty form body is valid. */
        if (state->mode == 9) { write(0, 1, sink); return FVM_OK; }
    } else strcpy(resolved, "http://canonical/resource");
    result = write(data, sizeof(data), sink);
    if (state && state->mode == 2) return FVM_UNSUPPORTED;
    if (state && state->mode == 3) return FVM_OK; /* Deliberately ignore a failing sink. */
    if (result != FVM_OK) return result;
    return write(data, sizeof(data), sink);
}
static FVM_DesktopTransferView done(FVM_DesktopTransfer *job)
{
    FVM_DesktopTransferView view;
    Uint32 start = SDL_GetTicks();
    do {
        assert(fvm_desktop_transfer_poll(job, &view) == FVM_OK);
        if (view.state != FVM_DESKTOP_TRANSFER_PENDING) return view;
        assert(!view.bytes && !view.size && !view.location && view.result == FVM_NOT_FOUND);
        assert(SDL_GetTicks() - start < 3000);
        SDL_Delay(1);
    } while (1);
}
int main(void)
{
    FVM_DesktopTransferOptions options;
    FVM_DesktopTransferView view;
    FVM_DesktopTransfer *job = 0, *unchanged;
    FVM_DesktopTransfer *jobs[FVM_DESKTOP_TRANSFER_MAX_JOBS];
    State state;
    unsigned int i;
    char reference[] = "http://copied/start", base[] = "http://copied/base";
    assert(!fvm_desktop_transfer_options_init(0));
    assert(fvm_desktop_transfer_options_init(&options));
    assert(!options.fetch && !options.userdata && options.max_bytes);
    assert(fvm_desktop_transfer_begin(reference, base, &options, &job) == FVM_INVALID_ARGUMENT);
    options.fetch = provider;
    assert(fvm_desktop_transfer_begin(0, base, &options, &job) == FVM_INVALID_ARGUMENT && !job);
    assert(fvm_desktop_transfer_poll(0, &view) == FVM_INVALID_ARGUMENT);
    assert(!fvm_desktop_transfer_cancel(0) && fvm_desktop_transfer_release(0) == FVM_OK);
    memset(&state, 0, sizeof(state)); state.owner = SDL_ThreadID();
    state.gate = SDL_CreateSemaphore(0); assert(state.gate);
    options.userdata = &state;
    for (i = 0; i < 10; ++i) {
        state.mode = (int)i; options.max_bytes = i == 3 ? 1 : 64;
        assert(fvm_desktop_transfer_begin(reference, base, &options, &job) == FVM_OK);
        state.job = job;
        assert(fvm_desktop_transfer_poll(job, &view) == FVM_OK && view.state == FVM_DESKTOP_TRANSFER_PENDING);
        assert(fvm_desktop_shutdown() == FVM_ERROR);
        if (i == 0) { reference[0] = 'X'; base[0] = 'Y'; }
        if (i == 6 || i == 7) assert(fvm_desktop_transfer_cancel(job));
        SDL_SemPost(state.gate);
        view = done(job);
        reference[0] = base[0] = 'h';
        if (i == 6 || i == 7) {
            assert(view.state == FVM_DESKTOP_TRANSFER_CANCELLED && view.result == FVM_ERROR);
            assert(!view.bytes && !view.size && !view.location);
            if (i == 7) assert(state.self_release == FVM_INVALID_ARGUMENT);
        } else if (i == 2 || i == 3 || i == 4 || i == 5 || i == 9) {
            assert(view.state == FVM_DESKTOP_TRANSFER_FAILED && !view.bytes && !view.location);
            assert(view.result == (i == 2 ? FVM_UNSUPPORTED : FVM_INVALID_ARGUMENT));
        } else {
            assert(view.state == FVM_DESKTOP_TRANSFER_DONE && view.result == FVM_OK);
            assert(view.size == (i == 8 ? 0 : 8));
            assert(!strcmp(view.location, "http://canonical/resource"));
            if (view.size) assert(((const unsigned char *)view.bytes)[1] == 0 &&
                                  ((const unsigned char *)view.bytes)[3] == 255);
        }
        assert(!fvm_desktop_transfer_cancel(job));
        assert(fvm_desktop_transfer_active() == 1);
        assert(fvm_desktop_transfer_release(job) == FVM_OK);
        assert(!fvm_desktop_transfer_active());
    }
    SDL_DestroySemaphore(state.gate);
    options.userdata = 0; options.max_bytes = 64;
    for (i = 0; i < FVM_DESKTOP_TRANSFER_MAX_JOBS; ++i) {
        assert(fvm_desktop_transfer_begin("http://a/", 0, &options, &jobs[i]) == FVM_OK);
        done(jobs[i]);
    }
    unchanged = jobs[0]; job = unchanged;
    assert(fvm_desktop_transfer_begin("http://a/", 0, &options, &job) == FVM_ERROR && job == unchanged);
    assert(fvm_desktop_shutdown() == FVM_ERROR);
    for (i = 0; i < FVM_DESKTOP_TRANSFER_MAX_JOBS; ++i)
        assert(fvm_desktop_transfer_release(jobs[i]) == FVM_OK);
    assert(!fvm_desktop_transfer_active());
    assert(fvm_desktop_shutdown() == FVM_OK);
    return 0;
}
