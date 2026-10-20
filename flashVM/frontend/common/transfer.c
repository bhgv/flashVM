/* Background native-byte accumulation with synchronized publication and explicit joined lifetime.
 */
#define SDL_MAIN_HANDLED
#include "transfer.h"
#include <SDL.h>
#include <stdlib.h>
#include <string.h>
/* Handle count protects the matching SDK shutdown while thread/TLS/mutex state still exists. */
static SDL_atomic_t transfer_handles;
/* One worker owns buffer mutations; the mutex protects snapshots, cancellation and publication. */
struct FVM_DesktopTransfer
{
    SDL_mutex *mutex;   /* Owned until after worker join. */
    SDL_Thread *thread; /* Owned join handle; no detached workers. */
    SDL_threadID owner; /* Creating thread may release; provider cannot self-join. */
    FVM_DesktopTransferOptions options; /* Copied scalars; nested userdata remains borrowed. */
    char reference[FVM_DESKTOP_MAX_LOCATION + 1]; /* Copied URL/reference. */
    char base[FVM_DESKTOP_MAX_LOCATION + 1];      /* Copied optional origin; empty means no base. */
    char location[FVM_DESKTOP_MAX_LOCATION + 1];  /* Worker-private until DONE publication. */
    unsigned char *bytes;                         /* Owned counted binary allocation. */
    size_t size, capacity; /* Written size and bounded allocation capacity. */
    size_t received;       /* Retained progress after error buffer disposal. */
    unsigned int state;    /* Synchronized terminal state. */
    int cancelled;         /* Synchronized explicit cooperative stop request. */
    int timed_out;         /* First stop reason was the elapsed deadline. */
    Uint32 started;        /* Monotonic SDL tick origin, unsigned rollover subtraction. */
    Uint32 timeout;        /* Zero or bounded duration copied before worker launch. */
    FVM_Result result;     /* First sink error or final provider/validation result. */
};
/* Validate accessible bounded C text without unbounded strlen. */
static int transfer_text(const char *text, int empty)
{
    size_t i;
    if (!text)
        return 0;
    for (i = 0; i <= FVM_DESKTOP_MAX_LOCATION; ++i)
        if (!text[i])
            return i != 0 || empty;
    return 0;
}
/* Caller holds mutex. Latch one stop reason; completed publication is immutable. */
static void transfer_deadline(FVM_DesktopTransfer *job)
{
    if (job->state == FVM_DESKTOP_TRANSFER_PENDING && !job->cancelled && !job->timed_out &&
        job->timeout && (Uint32)(SDL_GetTicks() - job->started) >= job->timeout)
        job->timed_out = 1;
}
/* Read cancellation without exposing atomic/platform fields to a provider. */
static int transfer_cancelled(void *userdata)
{
    FVM_DesktopTransfer *job = (FVM_DesktopTransfer *)userdata;
    int cancelled;
    SDL_LockMutex(job->mutex);
    transfer_deadline(job);
    cancelled = job->cancelled || job->timed_out;
    SDL_UnlockMutex(job->mutex);
    return cancelled;
}
/* Accumulate arbitrary chunks, latching errors even when a provider ignores sink failure. */
static FVM_Result transfer_write(const void *bytes, size_t size, void *userdata)
{
    FVM_DesktopTransfer *job = (FVM_DesktopTransfer *)userdata;
    size_t capacity;
    unsigned char *grown;
    FVM_Result result;
    SDL_LockMutex(job->mutex);
    if (job->result != FVM_OK)
    {
        result = job->result;
        goto done;
    }
    transfer_deadline(job);
    if (job->cancelled || job->timed_out)
    {
        job->result = FVM_ERROR;
        goto failed;
    }
    if ((!bytes && size) || size > job->options.max_bytes - job->size)
    {
        job->result = FVM_INVALID_ARGUMENT;
        goto failed;
    }
    if (job->capacity - job->size < size)
    {
        capacity = job->capacity ? job->capacity : FVM_DESKTOP_TRANSFER_INITIAL;
        if (capacity > job->options.max_bytes)
            capacity = job->options.max_bytes;
        while (capacity < job->size + size)
        {
            if (capacity > job->options.max_bytes / 2)
                capacity = job->options.max_bytes;
            else
                capacity *= 2;
        }
        grown = (unsigned char *)realloc(job->bytes, capacity);
        if (!grown)
        {
            job->result = FVM_OUT_OF_MEMORY;
            goto failed;
        }
        job->bytes = grown;
        job->capacity = capacity;
    }
    if (size)
        memcpy(job->bytes + job->size, bytes, size);
    job->size += size;
    job->received = job->size;
    result = FVM_OK;
    goto done;
failed:
    result = job->result;
done:
    SDL_UnlockMutex(job->mutex);
    return result;
}
/* Publish exactly once; cancellation before publication wins even over a provider's late success.
 */
static int transfer_worker(void *userdata)
{
    FVM_DesktopTransfer *job = (FVM_DesktopTransfer *)userdata;
    FVM_NetControl control;
    FVM_Result result;
    control.cancelled = transfer_cancelled;
    control.userdata = job;
    result =
        job->options.fetch(job->reference, job->base[0] ? job->base : 0, transfer_write, job,
                           job->location, sizeof(job->location), &control, job->options.userdata);
    SDL_LockMutex(job->mutex);
    if (job->result != FVM_OK)
        result = job->result;
    if (result == FVM_OK && (!memchr(job->location, 0, sizeof(job->location)) || !job->location[0]))
        result = FVM_INVALID_ARGUMENT;
    transfer_deadline(job);
    if (job->cancelled || job->timed_out)
    {
        job->state =
            job->timed_out ? FVM_DESKTOP_TRANSFER_TIMED_OUT : FVM_DESKTOP_TRANSFER_CANCELLED;
        result = FVM_ERROR;
    }
    else
        job->state = result == FVM_OK ? FVM_DESKTOP_TRANSFER_DONE : FVM_DESKTOP_TRANSFER_FAILED;
    job->result = result;
    if (job->state != FVM_DESKTOP_TRANSFER_DONE)
    {
        free(job->bytes);
        job->bytes = 0;
        job->size = job->capacity = 0;
    }
    SDL_UnlockMutex(job->mutex);
    return 0;
}
/* Track handles in this SDK instance; SDL does not expose private C structs to client code. */
unsigned int fvm_desktop_transfer_active(void)
{
    return (unsigned int)SDL_AtomicGet(&transfer_handles);
}
/* Defaults allocate nothing and deliberately require an explicit worker-safe provider. */
int fvm_desktop_transfer_options_init(FVM_DesktopTransferOptions *out)
{
    if (!out)
        return 0;
    out->fetch = 0;
    out->userdata = 0;
    out->max_bytes = FVM_DESKTOP_TRANSFER_MAX_BYTES;
    return 1;
}
/* Reserve before starting a thread so every terminal/failure path balances the quota. */
FVM_Result fvm_desktop_transfer_begin_timed(const char *reference, const char *base,
                                            const FVM_DesktopTransferOptions *options,
                                            unsigned long timeout_ms, FVM_DesktopTransfer **out)
{
    FVM_DesktopTransfer *job;
    int handles;
    if (timeout_ms > FVM_DESKTOP_TRANSFER_MAX_TIMEOUT_MS || !out || !options || !options->fetch ||
        !options->max_bytes || options->max_bytes > FVM_DESKTOP_TRANSFER_MAX_BYTES ||
        !transfer_text(reference, 0) || (base && !transfer_text(base, 0)))
        return FVM_INVALID_ARGUMENT;
    do
    {
        handles = SDL_AtomicGet(&transfer_handles);
        if (handles >= FVM_DESKTOP_TRANSFER_MAX_JOBS)
            return FVM_ERROR;
    } while (!SDL_AtomicCAS(&transfer_handles, handles, handles + 1));
    job = (FVM_DesktopTransfer *)calloc(1, sizeof(*job));
    if (!job)
        goto failed;
    job->options = *options;
    job->owner = SDL_ThreadID();
    job->started = SDL_GetTicks();
    job->timeout = (Uint32)timeout_ms;
    job->result = FVM_OK;
    strcpy(job->reference, reference);
    if (base)
        strcpy(job->base, base);
    job->mutex = SDL_CreateMutex();
    if (!job->mutex)
        goto failed;
    job->thread = SDL_CreateThread(transfer_worker, "FlashVM resource", job);
    if (!job->thread)
        goto failed;
    *out = job;
    return FVM_OK;
failed:
    if (job)
    {
        SDL_DestroyMutex(job->mutex);
        free(job);
    }
    SDL_AtomicAdd(&transfer_handles, -1);
    return FVM_OUT_OF_MEMORY;
}
/* Original ABI has no deadline; caller retains explicit cancellation/join control. */
FVM_Result fvm_desktop_transfer_begin(const char *reference, const char *base,
                                      const FVM_DesktopTransferOptions *options,
                                      FVM_DesktopTransfer **out)
{
    return fvm_desktop_transfer_begin_timed(reference, base, options, 0, out);
}
/* Never expose an accumulating allocation that a worker might realloc or discard. */
FVM_Result fvm_desktop_transfer_poll(FVM_DesktopTransfer *job, FVM_DesktopTransferView *out)
{
    if (!job || !out)
        return FVM_INVALID_ARGUMENT;
    SDL_LockMutex(job->mutex);
    transfer_deadline(job);
    out->state = job->state;
    out->result = job->state == FVM_DESKTOP_TRANSFER_PENDING ? FVM_NOT_FOUND : job->result;
    out->received = job->received;
    out->bytes = job->state == FVM_DESKTOP_TRANSFER_DONE ? job->bytes : 0;
    out->size = job->state == FVM_DESKTOP_TRANSFER_DONE ? job->size : 0;
    out->location = job->state == FVM_DESKTOP_TRANSFER_DONE ? job->location : 0;
    SDL_UnlockMutex(job->mutex);
    return FVM_OK;
}
/* A completed snapshot cannot be retroactively revoked by cancellation. */
int fvm_desktop_transfer_cancel(FVM_DesktopTransfer *job)
{
    int accepted;
    if (!job)
        return 0;
    SDL_LockMutex(job->mutex);
    accepted = job->state == FVM_DESKTOP_TRANSFER_PENDING;
    transfer_deadline(job);
    if (accepted && !job->timed_out)
        job->cancelled = 1;
    SDL_UnlockMutex(job->mutex);
    return accepted;
}
/* Join before disposing borrowed provider state or permitting library shutdown/unload. */
FVM_Result fvm_desktop_transfer_release(FVM_DesktopTransfer *job)
{
    if (!job)
        return FVM_OK;
    if (job->owner != SDL_ThreadID())
        return FVM_INVALID_ARGUMENT;
    fvm_desktop_transfer_cancel(job);
    SDL_WaitThread(job->thread, 0);
    free(job->bytes);
    SDL_DestroyMutex(job->mutex);
    free(job);
    SDL_AtomicAdd(&transfer_handles, -1);
    return FVM_OK;
}
