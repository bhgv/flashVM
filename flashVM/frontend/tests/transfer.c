/* Worker publication, copied text, sink latching, cancellation, quota and joined SDK lifetime. */
#define SDL_MAIN_HANDLED
#include <stdio.h>
#include <stdlib.h>
#include "player.h"
#include <SDL.h>
#include <string.h>


typedef struct State
{
    SDL_sem *gate, *exit_gate;
    FVM_DesktopTransfer *job;
    int mode;
    SDL_threadID owner, worker;
    FVM_Result self_release;
} State;


static FVM_Result provider(const char *reference,
                           const char *base,
                           FVM_DesktopWrite write,
                           void *sink,
                           char *resolved,
                           size_t capacity,
                           const FVM_NetControl *control,
                           void *userdata)
{
    State *state = (State *)userdata;
    const unsigned char data[] = {'a', 0, 'b', 255};
    FVM_Result result;
    (void)capacity;
    if (state)
    {
        state->worker = SDL_ThreadID();
        if (state->gate)
        {
            SDL_SemWait(state->gate);
        }
        if (!(state->worker != state->owner))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!strcmp(reference, "http://copied/start")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!strcmp(base, "http://copied/base")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (state->mode == 12)
        {
            while (!control->cancelled(control->userdata))
            {
                SDL_Delay(1);
            }
            SDL_SemWait(state->exit_gate); /* Remain pending after acknowledging the stop latch. */
            return FVM_OK;
        }
        if (state->mode == 10)
        {
            result = write(data, sizeof(data), sink);
            if (!(result == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            while (!control->cancelled(control->userdata))
            {
                SDL_Delay(1);
            }
            return FVM_OK; /* A provider cannot publish late success past a deadline. */
        }
        if (state->mode == 11)
        {
            SDL_Delay(80);
            return FVM_OK;
        }
        if (state->mode == 6)
        {
            while (!control->cancelled(control->userdata))
            {
                SDL_Delay(1);
            }
            return FVM_ERROR;
        }
        if (state->mode == 7)
        {
            state->self_release = fvm_desktop_transfer_release(state->job);
            SDL_Delay(30);
        }
        if (state->mode == 4)
        {
            return FVM_OK; /* Missing canonical output. */
        }
        if (state->mode == 5)
        {
            memset(resolved, 'X', capacity); /* Missing NUL. */
        }
        else
        {
            strcpy(resolved, "http://canonical/resource");
        }
        if (state->mode == 8)
        {
            return FVM_OK; /* An empty form body is valid. */
        }
        if (state->mode == 9)
        {
            write(0, 1, sink);
            return FVM_OK;
        }
    }
    else
    {
        strcpy(resolved, "http://canonical/resource");
    }
    result = write(data, sizeof(data), sink);
    if (state && state->mode == 2)
    {
        return FVM_UNSUPPORTED;
    }
    if (state && state->mode == 3)
    {
        return FVM_OK; /* Deliberately ignore a failing sink. */
    }
    if (result != FVM_OK)
    {
        return result;
    }
    return write(data, sizeof(data), sink);
}


static FVM_DesktopTransferView done(FVM_DesktopTransfer *job)
{
    FVM_DesktopTransferView view;
    Uint32 start = SDL_GetTicks();
    do
    {
        if (!(fvm_desktop_transfer_poll(job, &view) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (view.state != FVM_DESKTOP_TRANSFER_PENDING)
        {
            return view;
        }
        if (!(!view.bytes && !view.size && !view.location && view.result == FVM_NOT_FOUND))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(SDL_GetTicks() - start < 3000))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
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
    if (!(!fvm_desktop_transfer_options_init(0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_transfer_options_init(&options)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!options.fetch && !options.userdata && options.max_bytes))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_transfer_begin(reference, base, &options, &job) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    options.fetch = provider;
    if (!(fvm_desktop_transfer_begin(0, base, &options, &job) == FVM_INVALID_ARGUMENT && !job))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_transfer_poll(0, &view) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!fvm_desktop_transfer_cancel(0) && fvm_desktop_transfer_release(0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(&state, 0, sizeof(state));
    state.owner = SDL_ThreadID();
    state.gate = SDL_CreateSemaphore(0);
    if (!(state.gate))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    options.userdata = &state;
    for (i = 0; i < 10; ++i)
    {
        state.mode = (int)i;
        options.max_bytes = i == 3 ? 1 : 64;
        if (!(fvm_desktop_transfer_begin(reference, base, &options, &job) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        state.job = job;
        if (!(fvm_desktop_transfer_poll(job, &view) == FVM_OK && view.state == FVM_DESKTOP_TRANSFER_PENDING))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_desktop_shutdown() == FVM_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (i == 0)
        {
            reference[0] = 'X';
            base[0] = 'Y';
        }
        if (i == 6 || i == 7)
        {
            if (!(fvm_desktop_transfer_cancel(job)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        SDL_SemPost(state.gate);
        view = done(job);
        reference[0] = base[0] = 'h';
        if (i == 6 || i == 7)
        {
            if (!(view.state == FVM_DESKTOP_TRANSFER_CANCELLED && view.result == FVM_ERROR))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(!view.bytes && !view.size && !view.location))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (i == 7)
            {
                if (!(state.self_release == FVM_INVALID_ARGUMENT))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        else if (i == 2 || i == 3 || i == 4 || i == 5 || i == 9)
        {
            if (!(view.state == FVM_DESKTOP_TRANSFER_FAILED && !view.bytes && !view.location))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(view.result == (i == 2 ? FVM_UNSUPPORTED : FVM_INVALID_ARGUMENT)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(view.state == FVM_DESKTOP_TRANSFER_DONE && view.result == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(view.size == (i == 8 ? 0 : 8)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(!strcmp(view.location, "http://canonical/resource")))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (view.size)
            {
                if (!(((const unsigned char *)view.bytes)[1] == 0 && ((const unsigned char *)view.bytes)[3] == 255))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        if (!(!fvm_desktop_transfer_cancel(job)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_desktop_transfer_active() == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_desktop_transfer_release(job) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!fvm_desktop_transfer_active()))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    /* Total job deadlines include idle workers and survive ignored late success. */
    SDL_DestroySemaphore(state.gate);
    state.gate = 0;
    for (i = 0; i < 3; ++i)
    {
        state.mode = i == 2 ? 11 : (i == 1 ? 6 : 10);
        if (!(fvm_desktop_transfer_begin_timed(reference, base, &options, 40, &job) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (i == 1)
        {
            if (!(fvm_desktop_transfer_cancel(job)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        view = done(job);
        if (!(view.state == (i == 1 ? FVM_DESKTOP_TRANSFER_CANCELLED : FVM_DESKTOP_TRANSFER_TIMED_OUT)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(view.result == FVM_ERROR && !view.bytes && !view.size && !view.location))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (i == 0)
        {
            if (!(view.received == 4))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_desktop_transfer_stop_requested(job) == view.state))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!fvm_desktop_transfer_cancel(job)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_desktop_transfer_release(job) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    state.mode = 12;
    state.exit_gate = SDL_CreateSemaphore(0);
    if (!(state.exit_gate))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_transfer_begin_timed(reference, base, &options, 20, &job) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    while (!fvm_desktop_transfer_stop_requested(job))
    {
        SDL_Delay(1);
    }
    if (!(fvm_desktop_transfer_stop_requested(job) == FVM_DESKTOP_TRANSFER_TIMED_OUT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_transfer_poll(job, &view) == FVM_OK && view.state == FVM_DESKTOP_TRANSFER_PENDING))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!view.bytes && !view.location && view.result == FVM_NOT_FOUND))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    SDL_SemPost(state.exit_gate);
    view = done(job);
    if (!(view.state == FVM_DESKTOP_TRANSFER_TIMED_OUT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_transfer_release(job) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    SDL_DestroySemaphore(state.exit_gate);
    if (!(!fvm_desktop_transfer_stop_requested(0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    job = 0;
    if (!(fvm_desktop_transfer_begin_timed(
              reference, base, &options, FVM_DESKTOP_TRANSFER_MAX_TIMEOUT_MS + 1UL, &job) == FVM_INVALID_ARGUMENT &&
          !job))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }

    options.userdata = 0;
    options.max_bytes = 64;
    for (i = 0; i < FVM_DESKTOP_TRANSFER_MAX_JOBS; ++i)
    {
        if (!(fvm_desktop_transfer_begin("http://a/", 0, &options, &jobs[i]) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        done(jobs[i]);
    }
    unchanged = jobs[0];
    job = unchanged;
    if (!(fvm_desktop_transfer_begin("http://a/", 0, &options, &job) == FVM_ERROR && job == unchanged))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_shutdown() == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < FVM_DESKTOP_TRANSFER_MAX_JOBS; ++i)
    {
        if (!(fvm_desktop_transfer_release(jobs[i]) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(!fvm_desktop_transfer_active()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_shutdown() == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
