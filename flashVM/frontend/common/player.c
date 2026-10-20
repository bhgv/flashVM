/* Host-owned desktop playback: load, root, advance, rasterize, present, release. */
#if defined(__APPLE__) && !defined(_DARWIN_C_SOURCE)
#define _DARWIN_C_SOURCE 1 /* Expose native executable-path/realpath declarations in C89. */
#endif
#if defined(__ANDROID__) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L /* Expose readlink without depending on process CWD. */
#endif
#define SDL_MAIN_HANDLED
#include "player.h"
#include "playback_internal.h"
#include "desktop_build_version.h"
#include "audio.h"
#include "flashvm/gc.h"
#include "flashvm/avm1.h"
#include "flashvm/execution.h"
#include "flashvm/broadcaster.h"
#include "flashvm/input.h"
#include "flashvm/keyboard.h"
#include "flashvm/mouse.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/timers.h"
#include "flashvm/swf.h"
#include "flashvm/swf_stream_sound.h"
#include "options.h"
#include "request_constants.h"
#include "window.h"
#ifdef FVM_DESKTOP_HTTP
#include "http.h"
#endif
#ifdef FVM_DESKTOP_SOCKET_HTTP
#include "socket_http.h"
#endif
#include <SDL.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__ANDROID__)
#include <unistd.h>
#endif

#ifndef FVM_DESKTOP_EXECUTABLE_BASE
/* Return an SDL-allocated executable directory, never a Cocoa Resources directory or CWD. */
static char *executable_base_path(void)
{
#if defined(__APPLE__)
    char path[FVM_DESKTOP_MAX_LOCATION + 1], canonical[PATH_MAX];
    uint32_t capacity = sizeof(path);
    char *slash;
    if (_NSGetExecutablePath(path, &capacity) != 0 || !realpath(path, canonical))
        return 0;
    slash = strrchr(canonical, '/');
    if (!slash)
        return 0;
    slash[1] = 0;
    return SDL_strdup(canonical);
#elif defined(__ANDROID__)
    char path[FVM_DESKTOP_MAX_LOCATION + 1];
    ssize_t size = readlink("/proc/self/exe", path, FVM_DESKTOP_MAX_LOCATION);
    char *slash;
    if (size <= 0 || (size_t)size >= FVM_DESKTOP_MAX_LOCATION)
        return 0;
    path[(size_t)size] = 0;
    slash = strrchr(path, '/');
    if (!slash)
        return 0;
    slash[1] = 0;
    return SDL_strdup(path);
#else
    return SDL_GetBasePath();
#endif
}
#define FVM_DESKTOP_EXECUTABLE_BASE                                                                \
    executable_base_path /* Isolate OS discovery in protocol tests. */
#endif

/* Query linked/build metadata without creating runtime state or probing native devices. */
FVM_Result fvm_desktop_build_info(FVM_DesktopBuildInfo *out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    out->version = FVM_DESKTOP_BUILD_VERSION;
    out->abi_version = fvm_abi_version();
    out->pointer_bits = (unsigned int)(sizeof(void *) * CHAR_BIT);
    out->jit_available = fvm_jit_available() != 0;
#ifdef FVM_HAS_SDL_AUDIO
    out->sdl_audio_compiled = 1;
#else
    out->sdl_audio_compiled = 0;
#endif
    return FVM_OK;
}

/* An opaque C host can finish the matching SDL instance without linking another SDK copy. */
FVM_Result fvm_desktop_shutdown(void)
{
    if (fvm_desktop_transfer_active() || SDL_WasInit(0))
        return FVM_ERROR;
    SDL_Quit();
    return FVM_OK;
}

/* Report only the compiled default provider; custom loader policy remains caller-owned. */
unsigned int fvm_desktop_network_capabilities(void)
{
#ifdef FVM_DESKTOP_HTTP
    return fvm_desktop_http_capabilities();
#elif defined(FVM_DESKTOP_SOCKET_HTTP)
    return fvm_desktop_socket_capabilities();
#else
    return 0;
#endif
}

/* Console adapter for the same metadata available to a C host; no SDL_Init call. */
static void print_build_info(void)
{
    FVM_DesktopBuildInfo info;
    SDL_version linked;
    unsigned int network = fvm_desktop_network_capabilities();
    fvm_desktop_build_info(&info);
    SDL_GetVersion(&linked);
    printf("FlashVM player %s\n", info.version);
    printf("abi=%lu pointer_bits=%u jit_backend=%s sdl_audio_compiled=%s\n", info.abi_version,
           info.pointer_bits, info.jit_available ? "available" : "unavailable",
           info.sdl_audio_compiled ? "yes" : "no");
    printf("sdl_headers=%u.%u.%u sdl_linked=%u.%u.%u\n", (unsigned int)SDL_MAJOR_VERSION,
           (unsigned int)SDL_MINOR_VERSION, (unsigned int)SDL_PATCHLEVEL,
           (unsigned int)linked.major, (unsigned int)linked.minor, (unsigned int)linked.patch);
    printf("network_provider=%s http=%s https=%s fetch=synchronous\n",
           network & FVM_DESKTOP_NETWORK_SOCKET ? "socket"
           : network & FVM_DESKTOP_NETWORK_CURL ? "curl"
                                                : "none",
           network & FVM_DESKTOP_NETWORK_HTTP ? "yes" : "no",
           network & FVM_DESKTOP_NETWORK_HTTPS ? "yes" : "no");
    printf("background_provider=%s\n", fvm_desktop_background_supported() ? "yes" : "no");
    puts("Build metadata only; no device or target-runtime acceptance is implied.");
}

/* Read one bounded UTF-8 path with SDL's platform file adapter, including Unicode Windows paths. */
static unsigned char *read_resource(const char *path, size_t *size, int allow_empty)
{
    SDL_RWops *file = SDL_RWFromFile(path, "rb");
    Sint64 length;
    unsigned char *bytes;
    unsigned char
        extra; /* Probe measured EOF so a growing file is not loaded as a silent prefix. */
    if (!file)
    {
        fprintf(stderr, "%s: %s\n", path, SDL_GetError());
        return 0;
    }
    length = SDL_RWsize(file);
    if (length < 0 || (!length && !allow_empty) || length > (Sint64)FVM_SWF_DEFAULT_LIMIT)
    {
        fprintf(stderr, "%s: empty, unseekable or larger than 64 MiB\n", path);
        SDL_RWclose(file);
        return 0;
    }
    bytes = (unsigned char *)malloc(length ? (size_t)length : 1);
    if (!bytes)
    {
        fprintf(stderr, "%s: input allocation failed\n", path);
        SDL_RWclose(file);
        return 0;
    }
    if (SDL_RWread(file, bytes, 1, (size_t)length) != (size_t)length)
    {
        fprintf(stderr, "%s: input read: %s\n", path, SDL_GetError());
        free(bytes);
        SDL_RWclose(file);
        return 0;
    }
    SDL_ClearError();
    if (SDL_RWread(file, &extra, 1, 1) != 0 || SDL_GetError()[0])
    {
        fprintf(stderr, "%s: input changed length or failed at EOF: %s\n", path, SDL_GetError());
        free(bytes);
        SDL_RWclose(file);
        return 0;
    }
    if (SDL_RWclose(file) != 0)
    {
        fprintf(stderr, "%s: input close: %s\n", path, SDL_GetError());
        free(bytes);
        return 0;
    }
    *size = (size_t)length;
    return bytes;
}

/* Movie input remains nonempty; form-variable resources may be empty. */
static unsigned char *read_movie(const char *path, size_t *size)
{
    return read_resource(path, size, 0);
}

/* Default provider metadata is separate from custom per-run worker policies. */
int fvm_desktop_background_supported(void)
{
#ifdef FVM_DESKTOP_SOCKET_HTTP
    return 1;
#elif defined(FVM_DESKTOP_HTTP)
    return fvm_desktop_http_background_supported();
#else
    return 0;
#endif
}

/* Print the operation and exact VM status; unsupported features must not be silently skipped. */
static int checked(FVM_Result status, const char *operation, const char *path)
{
    if (status == FVM_OK)
        return 1;
    fprintf(stderr, "%s: %s failed (VM status %d)\n", path, operation, (int)status);
    return 0;
}

/* Clear every stage pixel before rendering so old frames cannot leave trails. */
static int clear_stage(FVM_Context *context, FVM_RasterSurface *surface)
{
    FVM_RasterColor background = {FVM_DESKTOP_WHITE, FVM_DESKTOP_WHITE, FVM_DESKTOP_WHITE,
                                  FVM_DESKTOP_OPAQUE};
    FVM_Result result = fvm_stage_get_background(context, &background);
    unsigned int x, y;
    if (result != FVM_OK && result != FVM_NOT_FOUND)
        return 0;
    for (y = 0; y < surface->height; ++y)
    {
        unsigned char *row = surface->pixels + y * surface->stride;
        for (x = 0; x < surface->width; ++x)
        {
            row[x * FVM_DESKTOP_RGBA_BYTES + FVM_DESKTOP_RED] = background.r;
            row[x * FVM_DESKTOP_RGBA_BYTES + FVM_DESKTOP_GREEN] = background.g;
            row[x * FVM_DESKTOP_RGBA_BYTES + FVM_DESKTOP_BLUE] = background.b;
            row[x * FVM_DESKTOP_RGBA_BYTES + FVM_DESKTOP_ALPHA] = background.a;
        }
    }
    return 1;
}

/* Render the current scene without advancing timeline, input, timers or the authored frame count.
 */
static int present_stage(FVM_Context *context, FVM_Object *clip, FVM_DesktopWindow *window,
                         FVM_RasterSurface *surface, const FVM_Affine *viewport, const char *path)
{
    if (!clear_stage(context, surface))
        return 0;
    if (!checked(fvm_movieclip_render_view(context, clip, surface, viewport), "raster render",
                 path))
        return 0;
    if (!fvm_desktop_window_present(window, surface, surface->stride * surface->height))
    {
        fprintf(stderr, "%s: presentation: %s\n", path, fvm_desktop_window_error());
        return 0;
    }
    return 1;
}

/* Copy a scalar without retaining borrowed storage or invoking user-defined input listeners. */
static FVM_Result input_number(FVM_Context *context, FVM_Object *object, const char *name,
                               double number)
{
    FVM_Value value;
    fvm_value_number(&value, number);
    return fvm_object_set(context, object, name, &value);
}

/* Publish a frame snapshot; HostInput is a player extension, not the complete Flash Key API. */
static FVM_Result publish_input(FVM_Context *context, FVM_Object *object,
                                const FVM_DesktopInput *input, double origin_x, double origin_y)
{
    FVM_Result result;
#define FVM_PUBLISH_INPUT(name, value)                                                             \
    do                                                                                             \
    {                                                                                              \
        result = input_number(context, object, name, (double)(value));                             \
        if (result != FVM_OK)                                                                      \
            return result;                                                                         \
    } while (0)
    FVM_PUBLISH_INPUT("x", input->x + origin_x);
    FVM_PUBLISH_INPUT("y", input->y + origin_y);
    FVM_PUBLISH_INPUT("left", input->left);
    FVM_PUBLISH_INPUT("right", input->right);
    FVM_PUBLISH_INPUT("up", input->up);
    FVM_PUBLISH_INPUT("down", input->down);
    FVM_PUBLISH_INPUT("space", input->space);
    FVM_PUBLISH_INPUT("buttons", input->buttons);
    FVM_PUBLISH_INPUT("keyCode", input->key_code);
    FVM_PUBLISH_INPUT("keyDown", input->key_down);
    FVM_PUBLISH_INPUT("clicks", input->clicks);
    FVM_PUBLISH_INPUT("keyPresses", input->key_presses);
#undef FVM_PUBLISH_INPUT
    result = fvm_input_set_keyboard(context, &input->keyboard);
    return result == FVM_OK
               ? fvm_input_set_pointer(context, input->x + origin_x, input->y + origin_y)
               : result;
}

/* Explicit --mute acknowledges validated root sound events without decoding or opening audio. */
static FVM_Result muted_sound(FVM_Context *context, FVM_Object *clip, size_t ordinal,
                              void *userdata)
{
    FVM_DesktopAudio *audio = (FVM_DesktopAudio *)userdata;
    (void)context;
    (void)clip;
    (void)ordinal;
    ++audio->events;
    return FVM_OK;
}

/* Explicit background session borrows only owner-thread native window/audio pointers. */
typedef struct FVM_DesktopBackgroundSession
{
    FVM_DesktopTransferOptions options;  /* Immutable provider policy copied before playback. */
    FVM_DesktopWindow *window;           /* Borrowed only while play_bytes owns a live view. */
    FVM_DesktopAudio *audio;             /* Borrowed only while its native output is live. */
    FVM_DesktopPlaybackSession *control; /* Borrowed owner policy through source-run return. */
    int closed;       /* Owner-thread close request, independent of worker status. */
    int asynchronous; /* Keep the timeline running for deferred network child/form jobs. */
    const FVM_DesktopLoadMonitor *monitor; /* Borrowed owner-only callbacks and deadline policy. */
    unsigned long next_id; /* One-based request IDs, bounded by validated load budget. */
    unsigned int kind;     /* Source or synchronous child/form purpose for background_fetch. */
} FVM_DesktopBackgroundSession;
/* Report borrowed metadata only; terminal callbacks cannot revoke a published disposition. */
static void background_notice(FVM_DesktopBackgroundSession *session, unsigned long id,
                              unsigned int kind, unsigned int phase, const char *reference,
                              const char *base, const FVM_DesktopTransferView *view,
                              FVM_Result result)
{
    FVM_DesktopLoadEvent event;
    int stop;
    if (!session->monitor || !session->monitor->notify)
        return;
    event.id = id;
    event.kind = kind;
    event.phase = phase;
    event.received = view ? view->received : 0;
    event.result = result;
    event.reference = reference;
    event.base = base;
    event.location = phase == FVM_DESKTOP_LOAD_FETCHED && view ? view->location : 0;
    stop = session->monitor->notify(&event, session->monitor->userdata);
    if (stop && (phase == FVM_DESKTOP_LOAD_BEGIN || phase == FVM_DESKTOP_LOAD_WAIT))
        session->closed = 1;
}
/* Begin notification precedes I/O; an owner stop does not start a provider. */
static FVM_Result background_begin(FVM_DesktopBackgroundSession *session, const char *reference,
                                   const char *base, unsigned int kind, FVM_DesktopTransfer **out,
                                   unsigned long *id)
{
    FVM_Result result;
    *id = ++session->next_id;
    background_notice(session, *id, kind, FVM_DESKTOP_LOAD_BEGIN, reference, base, 0,
                      FVM_NOT_FOUND);
    result = session->closed ? FVM_ERROR
                             : fvm_desktop_transfer_begin_timed(
                                   reference, base, &session->options,
                                   session->monitor ? session->monitor->timeout_ms : 0, out);
    if (result != FVM_OK)
        background_notice(session, *id, kind,
                          session->closed ? FVM_DESKTOP_LOAD_CANCELLED : FVM_DESKTOP_LOAD_FAILED,
                          reference, base, 0, result);
    return result;
}
/* Map native result before its borrowed location is released. No VM import is implied. */
static void background_terminal(FVM_DesktopBackgroundSession *session, unsigned long id,
                                unsigned int kind, const char *reference, const char *base,
                                const FVM_DesktopTransferView *view, FVM_Result result,
                                int abandoned)
{
    unsigned int phase = abandoned ? FVM_DESKTOP_LOAD_CANCELLED : FVM_DESKTOP_LOAD_FAILED;
    if (!abandoned && view)
    {
        if (view->state == FVM_DESKTOP_TRANSFER_TIMED_OUT)
            phase = FVM_DESKTOP_LOAD_TIMED_OUT;
        else if (view->state == FVM_DESKTOP_TRANSFER_CANCELLED)
            phase = FVM_DESKTOP_LOAD_CANCELLED;
        else if (result == FVM_OK)
            phase = FVM_DESKTOP_LOAD_FETCHED;
    }
    background_notice(session, id, kind, phase, reference, base, view,
                      abandoned ? FVM_ERROR : result);
}
/* A cooperative deadline stops further VM turns even if a provider is slow to acknowledge it.
 * The actual job remains pending until its worker exits; only this owner's local view changes. */
static FVM_Result background_poll(FVM_DesktopTransfer *job, FVM_DesktopTransferView *view)
{
    FVM_Result result = fvm_desktop_transfer_poll(job, view);
    if (result == FVM_OK && view->state == FVM_DESKTOP_TRANSFER_PENDING)
    {
        unsigned int reason = fvm_desktop_transfer_stop_requested(job);
        if (reason)
        {
            view->state = reason;
            view->result = FVM_ERROR;
        }
    }
    return result;
}
/* The worker owns only bytes; event/audio pumping and the consumer callback stay on the owner. */
static FVM_Result background_fetch(const char *reference, const char *base, FVM_DesktopWrite write,
                                   void *write_userdata, char *resolved, size_t capacity,
                                   void *userdata)
{
    FVM_DesktopBackgroundSession *session = (FVM_DesktopBackgroundSession *)userdata;
    FVM_DesktopTransfer *job = 0;
    FVM_DesktopTransferView view;
    unsigned long id;
    FVM_Result result = background_begin(session, reference, base, session->kind, &job, &id);
    memset(&view, 0, sizeof(view));
    if (result != FVM_OK)
        return result;
    for (;;)
    {
        result = background_poll(job, &view);
        if (result != FVM_OK || view.state != FVM_DESKTOP_TRANSFER_PENDING)
            break;
        background_notice(session, id, session->kind, FVM_DESKTOP_LOAD_WAIT, reference, base, &view,
                          FVM_NOT_FOUND);
        if (session->closed)
            break;
        if (session->window)
        {
            if (!fvm_desktop_window_events(session->window, 0))
                session->closed = 1;
            result = fvm_desktop_window_status(session->window);
            if (result == FVM_OK && session->audio && session->audio->output)
                result = fvm_desktop_window_active(session->window)
                             ? fvm_desktop_audio_tick_paused(
                                   session->audio, session->control && session->control->paused)
                             : fvm_desktop_audio_pause(session->audio, 1);
            if (session->closed || result != FVM_OK)
                break;
        }
        SDL_Delay(FVM_DESKTOP_BACKGROUND_WAIT_MS);
    }
    if (session->closed)
        result = FVM_ERROR;
    else if (result == FVM_OK)
    {
        result = view.result;
        if (result == FVM_OK)
        {
            if (!resolved || !capacity || strlen(view.location) >= capacity)
                result = FVM_INVALID_ARGUMENT;
            else
            {
                result = write(view.bytes, view.size, write_userdata);
                if (result == FVM_OK)
                    strcpy(resolved, view.location);
            }
        }
    }
    background_terminal(session, id, session->kind, reference, base, &view, result,
                        session->closed);
    fvm_desktop_transfer_release(job); /* Always cancel/join before session or VM teardown. */
    return result;
}

/* A copied request owns its text and one explicit target root until safe-point delivery. */
typedef struct FVM_DesktopRequest
{
    char *url;                       /* Owned reference copied from the VM's counted string. */
    char *base;                      /* Owned origin provenance, independent of selected target. */
    FVM_Object *target;              /* [GC:] Explicitly pinned while queued. */
    unsigned int flags;              /* Load family or private deferred unload operation. */
    int discard;                     /* Staged cancellation decision; publication happens later. */
    struct FVM_DesktopRequest *next; /* Native FIFO link. */
} FVM_DesktopRequest;
/* Per-run immutable bindings carry the owning navigation alongside language-call shape. */
typedef struct FVM_DesktopLoadBuiltin
{
    unsigned int flags;                       /* LOAD_TARGET or LOAD_VARIABLES request family. */
    int method;                               /* Nonzero for MovieClip receiver form. */
    struct FVM_DesktopNavigation *navigation; /* Borrowed until the context is destroyed. */
} FVM_DesktopLoadBuiltin;
/* Per-run transport and request queue; no mutable process-wide registration. */
typedef struct FVM_DesktopNavigation
{
    FVM_DesktopLoadBuiltin
        builtins[FVM_DESKTOP_LOAD_BINDING_COUNT]; /* Context callback payloads. */
    char *pending;                   /* Owned next root URL after the old VM is released. */
    char *pending_base;              /* Owned provenance for the next root request. */
    FVM_DesktopRequest *head, *tail; /* Owned FIFO, drained only outside VM callbacks. */
    size_t count;                    /* Bound queued fetches by the remaining load budget. */
    size_t unload_count;           /* Independently bound unloads even when fetch budget is zero. */
    unsigned long transfer_id;     /* Native observation ordinal for the active request. */
    FVM_DesktopTransfer *transfer; /* Owned in-flight native-byte job, joined before GC unpin. */
    FVM_DesktopRequest *active_request; /* [GC:] Keeps its target pinned across VM turns. */
    FVM_Object *root;                   /* Borrowed active root, pinned by play_bytes. */
    FVM_DesktopAudio *audio;            /* Borrowed owner-thread audio until play_bytes cleanup. */
    const char *initial_base; /* Borrowed initial canonical location or executable resource path. */
    const FVM_DesktopLoader *policy;          /* Borrowed for the entire source run. */
    FVM_DesktopBackgroundSession *background; /* Optional borrowed owner-thread transfer session. */
    FVM_DesktopPlaybackSession *control; /* Borrowed state shared across whole-root navigation. */
    unsigned long remaining; /* Remaining requests, including child and variable fetches. */
} FVM_DesktopNavigation;

/* Service is defined after the bounded transport helpers below. */
static FVM_Result service_requests(FVM_Context *, FVM_DesktopNavigation *);

/* [GC:] Release all native FIFO ownership before the corresponding context is destroyed. */
static void discard_requests(FVM_Context *context, FVM_DesktopNavigation *navigation)
{
    if (navigation && navigation->transfer)
    {
        FVM_DesktopTransferView view;
        FVM_DesktopRequest *request = navigation->active_request;
        memset(&view, 0, sizeof(view));
        fvm_desktop_transfer_poll(navigation->transfer, &view);
        background_terminal(navigation->background, navigation->transfer_id,
                            request->flags & FVM_AVM1_URL_LOAD_VARIABLES
                                ? FVM_DESKTOP_LOAD_VARIABLES
                                : FVM_DESKTOP_LOAD_CHILD,
                            request->url, request->base, &view, FVM_ERROR, 1);
        fvm_desktop_transfer_release(navigation->transfer);
        navigation->transfer = 0;
    }
    if (navigation && navigation->active_request)
    {
        FVM_DesktopRequest *request = navigation->active_request;
        navigation->active_request = 0;
        fvm_gc_unpin(context, request->target);
        free(request->url);
        free(request->base);
        free(request);
    }
    while (navigation && navigation->head)
    {
        FVM_DesktopRequest *request = navigation->head;
        navigation->head = request->next;
        fvm_gc_unpin(context, request->target);
        free(request->url);
        free(request->base);
        free(request);
    }
    if (navigation)
    {
        navigation->tail = 0;
        navigation->count = 0;
        navigation->unload_count = 0;
    }
}

/* [GC:] Join an abandoned job before releasing the target pin or copied request text. */
static void abandon_active(FVM_Context *context, FVM_DesktopNavigation *navigation)
{
    FVM_DesktopRequest *request = navigation->active_request;
    FVM_DesktopTransferView view;
    if (!request)
        return;
    memset(&view, 0, sizeof(view));
    fvm_desktop_transfer_poll(navigation->transfer, &view);
    background_terminal(navigation->background, navigation->transfer_id,
                        request->flags & FVM_AVM1_URL_LOAD_VARIABLES ? FVM_DESKTOP_LOAD_VARIABLES
                                                                     : FVM_DESKTOP_LOAD_CHILD,
                        request->url, request->base, &view, FVM_ERROR, 1);
    fvm_desktop_transfer_release(navigation->transfer);
    navigation->transfer = 0;
    navigation->active_request = 0;
    fvm_gc_unpin(context, request->target);
    free(request->url);
    free(request->base);
    free(request);
}

/* [GC:] Remove one linked request after its native effects have committed or been discarded. */
static void release_request(FVM_Context *context, FVM_DesktopRequest *request)
{
    fvm_gc_unpin(context, request->target);
    free(request->url);
    free(request->base);
    free(request);
}

/* Resolve the global target argument or method receiver while still in the calling AVM1 scope. */
static FVM_Result unload_builtin(FVM_Context *context, FVM_Object *receiver, unsigned int argc,
                                 const FVM_Value *args, FVM_Value *result, void *userdata)
{
    FVM_DesktopNavigation *navigation = (FVM_DesktopNavigation *)userdata;
    FVM_AVM1TargetInfo info;
    FVM_Value target;
    FVM_DesktopRequest *request;
    FVM_Result status;
    int attached = 0;
    if (result)
        fvm_value_undefined(result);
    if (!navigation || argc > FVM_DESKTOP_UNLOAD_MAX_ARGS || (argc && !args))
        return FVM_INVALID_ARGUMENT;
    fvm_value_undefined(&target);
    if (!argc)
    {
        target.type = FVM_VALUE_OBJECT;
        target.object = receiver;
    }
    else if (args[0].type == FVM_VALUE_OBJECT)
        target = args[0];
    else if (args[0].type == FVM_VALUE_STRING)
    {
        info.struct_size = sizeof(info);
        status = fvm_avm1_get_target_info(context, &info);
        if (status != FVM_OK || !info.active || !info.base_clip)
            return FVM_INVALID_ARGUMENT;
        if (fvm_value_string_size(args) == strlen("_level0") &&
            !memcmp(args[0].string, "_level0", strlen("_level0")))
        {
            target.type = FVM_VALUE_OBJECT;
            target.object = navigation->root;
        }
        else
        {
            status =
                fvm_exec_scope_target(context, 0, info.target_clip, info.base_clip, args, &target);
            if (status != FVM_OK)
                return status;
        }
    }
    else
        return FVM_INVALID_ARGUMENT;
    if (target.type != FVM_VALUE_OBJECT || !target.object)
        return FVM_NOT_FOUND;
    status = fvm_movieclip_attached_to(context, target.object, navigation->root, &attached);
    if (status != FVM_OK || !attached)
        return status != FVM_OK ? status : FVM_NOT_FOUND;
    if (navigation->unload_count >= FVM_DESKTOP_MAX_LOADS)
        return FVM_UNSUPPORTED;
    request = (FVM_DesktopRequest *)calloc(1, sizeof(*request));
    if (!request)
        return FVM_OUT_OF_MEMORY;
    request->target = target.object;
    request->flags = FVM_DESKTOP_REQUEST_UNLOAD;
    status = fvm_gc_pin(context, request->target);
    if (status != FVM_OK)
    {
        free(request);
        return status;
    }
    if (navigation->tail)
        navigation->tail->next = request;
    else
        navigation->head = request;
    navigation->tail = request;
    ++navigation->unload_count;
    return FVM_OK;
}

/* Process urgent unloads in script order before polling an in-flight transfer.
 * Only requests preceding the unload are cancelled; later reloads of the instance survive. */
static FVM_Result service_unloads(FVM_Context *context, FVM_DesktopNavigation *navigation)
{
    while (navigation->unload_count)
    {
        FVM_DesktopRequest *unload = navigation->head, *request, *previous;
        FVM_Result status;
        int attached = 0, cancel_active = 0;
        while (unload && unload->flags != FVM_DESKTOP_REQUEST_UNLOAD)
            unload = unload->next;
        if (!unload)
            return FVM_ERROR;
        status = fvm_movieclip_attached_to(context, unload->target, navigation->root, &attached);
        if (status != FVM_OK)
            return status;
        if (attached && navigation->active_request)
        {
            status = fvm_movieclip_attached_to(context, navigation->active_request->target,
                                               unload->target, &cancel_active);
            if (status != FVM_OK)
                return status;
        }
        /* Stage all subtree decisions before the atomic clear detaches native parent edges. */
        for (request = navigation->head; request != unload; request = request->next)
        {
            request->discard = 0;
            if (attached)
            {
                status = fvm_movieclip_attached_to(context, request->target, unload->target,
                                                   &request->discard);
                if (status != FVM_OK)
                    return status;
            }
        }
        if (attached)
        {
            status = fvm_movieclip_unload(context, unload->target);
            if (status != FVM_OK)
                return status; /* No cancellation or old-content mutation on allocation failure. */
            if (unload->target == navigation->root)
            {
                /* Silence PCM before cancellation joins a potentially slow network provider. */
                status = fvm_desktop_audio_clear(context, navigation->audio);
                if (status != FVM_OK)
                    return status; /* Content is committed; owner error cleanup completes teardown.
                                    */
            }
        }
        if (cancel_active)
            abandon_active(context, navigation);
        previous = 0;
        request = navigation->head;
        while (request)
        {
            FVM_DesktopRequest *next = request->next;
            int done = request == unload;
            if (done || request->discard)
            {
                if (previous)
                    previous->next = next;
                else
                    navigation->head = next;
                if (navigation->tail == request)
                    navigation->tail = previous;
                if (done)
                    --navigation->unload_count;
                else
                    --navigation->count;
                release_request(context, request);
            }
            else
                previous = request;
            if (done)
                break;
            request = next;
        }
    }
    return FVM_OK;
}

/* Copy counted source text without accepting embedded NUL or overlong locations. */
static char *copy_location(const FVM_Value *value, FVM_Result *status)
{
    size_t size = fvm_value_string_size(value);
    char *copy;
    if (!size || size > FVM_DESKTOP_MAX_LOCATION || memchr(value->string, 0, size))
    {
        *status = FVM_INVALID_ARGUMENT;
        return 0;
    }
    if (*status != FVM_OK)
        return 0;
    copy = (char *)malloc(size + 1);
    if (!copy)
        *status = FVM_OUT_OF_MEMORY;
    if (copy)
    {
        memcpy(copy, value->string, size);
        copy[size] = 0;
    }
    return copy;
}

/* [GC:] One queue implementation copies URL/origin and pins the exact destination identity.
 * No script callback or transport occurs after membership validation. */
static FVM_Result enqueue_request(FVM_Context *context, FVM_DesktopNavigation *navigation,
                                  const FVM_Value *url, const FVM_Value *base, FVM_Object *target,
                                  unsigned int flags)
{
    FVM_DesktopRequest *request;
    FVM_Result status;
    int attached = 0;
    if (navigation->count >= navigation->remaining)
        return FVM_UNSUPPORTED;
    status = fvm_movieclip_attached_to(context, target, navigation->root, &attached);
    if (status != FVM_OK || !attached)
        return status != FVM_OK ? status : FVM_NOT_FOUND;
    request = (FVM_DesktopRequest *)calloc(1, sizeof(*request));
    if (!request)
        return FVM_OUT_OF_MEMORY;
    request->url = copy_location(url, &status);
    request->base = copy_location(base, &status);
    request->target = target;
    request->flags = flags;
    if (status == FVM_OK)
        status = fvm_gc_pin(context, request->target);
    if (status != FVM_OK)
    {
        free(request->url);
        free(request->base);
        free(request);
        return status;
    }
    if (navigation->tail)
        navigation->tail->next = request;
    else
        navigation->head = request;
    navigation->tail = request;
    ++navigation->count;
    return FVM_OK;
}

/* [GC:] Direct receiver/object targets never round-trip through replaceable names or toString.
 * The enclosing native call barrier retains arguments through URL coercion callbacks. */
static FVM_Result request_direct(FVM_Context *context, FVM_DesktopNavigation *navigation,
                                 const FVM_Value *url, FVM_Object *target, unsigned int flags)
{
    FVM_AVM1TargetInfo info;
    FVM_Value text, base;
    FVM_Result status = fvm_exec_to_string(context, url, &text);
    if (status != FVM_OK)
        return status;
    info.struct_size = sizeof(info);
    status = fvm_avm1_get_target_info(context, &info);
    if (status != FVM_OK || !info.active || !info.base_clip)
        return FVM_INVALID_ARGUMENT;
    status = fvm_movieclip_get_source(context, info.base_clip, &base);
    if (status != FVM_OK)
        return status;
    return enqueue_request(context, navigation, &text, &base, target, flags);
}

/* Resolve and retain target/origin inside the active frame; transport waits for a safe point. */
static FVM_Result request_movie(FVM_Context *context, FVM_Object *receiver, unsigned int argc,
                                const FVM_Value *args, FVM_Value *result, void *userdata)
{
    FVM_DesktopNavigation *navigation = (FVM_DesktopNavigation *)userdata;
    FVM_AVM1TargetInfo info;
    FVM_Value target, base;
    unsigned int flags;
    FVM_Result status;
    size_t target_size;
    (void)receiver;
    (void)result;
    if (argc != FVM_DESKTOP_URL_ARGUMENTS || !args || !navigation ||
        args[FVM_DESKTOP_URL_URL].type != FVM_VALUE_STRING ||
        args[FVM_DESKTOP_URL_TARGET].type != FVM_VALUE_STRING ||
        args[FVM_DESKTOP_URL_FLAGS].type != FVM_VALUE_NUMBER)
        return FVM_INVALID_ARGUMENT;
    flags = (unsigned int)args[FVM_DESKTOP_URL_FLAGS].number;
    if (((flags & ~FVM_DESKTOP_URL_METHOD_MASK) != FVM_AVM1_URL_LOAD_TARGET &&
         (flags & ~FVM_DESKTOP_URL_METHOD_MASK) != FVM_AVM1_URL_LOAD_VARIABLES) ||
        (flags & FVM_DESKTOP_URL_METHOD_MASK) > FVM_AVM1_URL_METHOD_GET ||
        navigation->count >= navigation->remaining)
        return FVM_UNSUPPORTED;
    info.struct_size = sizeof(info);
    status = fvm_avm1_get_target_info(context, &info);
    if (status != FVM_OK || !info.active || !info.base_clip)
        return FVM_INVALID_ARGUMENT;
    target_size = fvm_value_string_size(args + FVM_DESKTOP_URL_TARGET);
    fvm_value_undefined(&target);
    if (target_size == strlen("_level0") &&
        !memcmp(args[FVM_DESKTOP_URL_TARGET].string, "_level0", target_size))
    {
        target.type = FVM_VALUE_OBJECT;
        target.object = navigation->root;
    }
    else if (!target_size)
    {
        target.type = FVM_VALUE_OBJECT;
        target.object = info.target_clip;
    }
    else
        status = fvm_exec_scope_target(context, 0, info.target_clip, info.base_clip,
                                       args + FVM_DESKTOP_URL_TARGET, &target);
    if (status != FVM_OK || target.type != FVM_VALUE_OBJECT || !target.object)
        return FVM_NOT_FOUND;
    status = fvm_movieclip_get_source(context, info.base_clip, &base);
    if (status != FVM_OK)
        return status;
    return enqueue_request(context, navigation, args + FVM_DESKTOP_URL_URL, &base, target.object,
                           flags);
}

/* Standard host-facing loader spellings reuse the VM's counted/coerced URL service. */
static FVM_Result load_builtin(FVM_Context *context, FVM_Object *receiver, unsigned int argc,
                               const FVM_Value *args, FVM_Value *result, void *userdata)
{
    const FVM_DesktopLoadBuiltin *builtin = (const FVM_DesktopLoadBuiltin *)userdata;
    FVM_Value target;
    int method_form = builtin->method;
    unsigned int method_index;
    if (!argc || !args)
        return FVM_INVALID_ARGUMENT;
    /* AVM1 unqualified names may resolve through a clip prototype before globals.
     * Preserve both public argument shapes when that shared spelling is inherited. */
    if (method_form && argc >= FVM_DESKTOP_LOAD_GLOBAL_MIN_ARGS &&
        (argc >= FVM_DESKTOP_LOAD_GLOBAL_FULL_ARGS ||
         args[FVM_DESKTOP_LOAD_TARGET_INDEX].type == FVM_VALUE_OBJECT ||
         (args[FVM_DESKTOP_LOAD_TARGET_INDEX].type == FVM_VALUE_STRING &&
          !((fvm_value_string_size(args + FVM_DESKTOP_LOAD_TARGET_INDEX) == strlen("GET") &&
             !memcmp(args[FVM_DESKTOP_LOAD_TARGET_INDEX].string, "GET", strlen("GET"))) ||
            (fvm_value_string_size(args + FVM_DESKTOP_LOAD_TARGET_INDEX) == strlen("POST") &&
             !memcmp(args[FVM_DESKTOP_LOAD_TARGET_INDEX].string, "POST", strlen("POST")))))))
        method_form = 0;
    method_index =
        method_form ? FVM_DESKTOP_LOAD_METHOD_INDEX : FVM_DESKTOP_LOAD_GLOBAL_METHOD_INDEX;
    if (result)
        fvm_value_undefined(result);
    if (!method_form && argc < FVM_DESKTOP_LOAD_GLOBAL_MIN_ARGS)
        return FVM_INVALID_ARGUMENT;
    if (argc > method_index && args[method_index].type != FVM_VALUE_UNDEFINED)
    {
        const FVM_Value *method = args + method_index;
        if (method->type != FVM_VALUE_STRING || fvm_value_string_size(method) != strlen("GET") ||
            memcmp(method->string, "GET", strlen("GET")))
            return FVM_UNSUPPORTED;
    }
    if (method_form)
        return request_direct(context, builtin->navigation, args, receiver, builtin->flags);
    target = args[FVM_DESKTOP_LOAD_TARGET_INDEX];
    if (target.type == FVM_VALUE_OBJECT)
        return request_direct(context, builtin->navigation, args, target.object, builtin->flags);
    return fvm_exec_url(context, args, &target, builtin->flags);
}

/* Numbered level loading currently maps only level zero to the existing root lifecycle.
 * Numeric coercion is explicit; unsupported levels never trigger a transport request. */
static FVM_Result load_numbered_builtin(FVM_Context *context, FVM_Object *receiver,
                                        unsigned int argc, const FVM_Value *args, FVM_Value *result,
                                        void *userdata)
{
    FVM_Value target;
    FVM_Value level;
    FVM_Result status;
    (void)receiver;
    (void)userdata;
    if (result)
        fvm_value_undefined(result);
    if (!args || argc < FVM_DESKTOP_NUMBERED_LOAD_MIN_ARGS ||
        argc > FVM_DESKTOP_NUMBERED_LOAD_MAX_ARGS)
        return FVM_INVALID_ARGUMENT;
    status = fvm_exec_to_number(context, 0, args + FVM_DESKTOP_NUMBERED_LEVEL_INDEX, &level);
    if (status != FVM_OK)
        return status;
    if (level.number != 0)
        return FVM_UNSUPPORTED;
    if (argc == FVM_DESKTOP_NUMBERED_LOAD_MAX_ARGS &&
        args[FVM_DESKTOP_NUMBERED_METHOD_INDEX].type != FVM_VALUE_UNDEFINED)
    {
        const FVM_Value *method = args + FVM_DESKTOP_NUMBERED_METHOD_INDEX;
        if (method->type != FVM_VALUE_STRING || fvm_value_string_size(method) != strlen("GET") ||
            memcmp(method->string, "GET", strlen("GET")))
            return FVM_UNSUPPORTED;
    }
    fvm_value_string(&target, "_level0");
    return fvm_exec_url(context, args, &target, FVM_AVM1_URL_LOAD_TARGET);
}

/* Numbered root unloading reuses the exact same pin/queue/audio cancellation path. */
static FVM_Result unload_numbered_builtin(FVM_Context *context, FVM_Object *receiver,
                                          unsigned int argc, const FVM_Value *args,
                                          FVM_Value *result, void *userdata)
{
    FVM_DesktopNavigation *navigation = (FVM_DesktopNavigation *)userdata;
    FVM_Value target;
    FVM_Value level;
    FVM_Result status;
    if (!navigation || !args || argc != FVM_DESKTOP_UNLOAD_MAX_ARGS)
        return FVM_INVALID_ARGUMENT;
    status = fvm_exec_to_number(context, 0, args, &level);
    if (status != FVM_OK)
        return status;
    if (level.number != 0)
        return FVM_UNSUPPORTED;
    fvm_value_undefined(&target);
    target.type = FVM_VALUE_OBJECT;
    target.object = navigation->root;
    return unload_builtin(context, receiver, FVM_DESKTOP_UNLOAD_MAX_ARGS, &target, result,
                          navigation);
}

/* Install loader facades in the optional host only; the standalone VM remains transport-neutral. */
static FVM_Result install_load_functions(FVM_Context *context, FVM_DesktopNavigation *navigation)
{
    FVM_Value constructor, prototype;
    FVM_Result status;
    unsigned int i;
    for (i = 0; i < FVM_DESKTOP_LOAD_BINDING_COUNT; ++i)
    {
        navigation->builtins[i].navigation = navigation;
        navigation->builtins[i].method =
            i == FVM_DESKTOP_LOAD_MOVIE_METHOD || i == FVM_DESKTOP_LOAD_VARIABLES_METHOD;
        navigation->builtins[i].flags =
            i == FVM_DESKTOP_LOAD_VARIABLES_GLOBAL || i == FVM_DESKTOP_LOAD_VARIABLES_METHOD
                ? FVM_AVM1_URL_LOAD_VARIABLES
                : FVM_AVM1_URL_LOAD_TARGET;
    }
    status =
        fvm_register_function_rooted_bytes(context, "loadMovie", strlen("loadMovie"), load_builtin,
                                           &navigation->builtins[FVM_DESKTOP_LOAD_MOVIE_GLOBAL], 0);
    if (status == FVM_OK)
        status = fvm_register_function_rooted_bytes(
            context, "loadVariables", strlen("loadVariables"), load_builtin,
            &navigation->builtins[FVM_DESKTOP_LOAD_VARIABLES_GLOBAL], 0);
    if (status == FVM_OK)
        status = fvm_get_global(context, "MovieClip", &constructor);
    if (status == FVM_OK && constructor.type == FVM_VALUE_OBJECT)
        status = fvm_object_get(context, constructor.object, "prototype", &prototype);
    else if (status == FVM_OK)
        status = FVM_INVALID_ARGUMENT;
    if (status == FVM_OK && prototype.type == FVM_VALUE_OBJECT)
        status = fvm_object_define_method_rooted(
            context, prototype.object, "loadMovie", load_builtin,
            &navigation->builtins[FVM_DESKTOP_LOAD_MOVIE_METHOD], 0);
    else if (status == FVM_OK)
        status = FVM_INVALID_ARGUMENT;
    if (status == FVM_OK)
        status = fvm_object_define_method_rooted(
            context, prototype.object, "loadVariables", load_builtin,
            &navigation->builtins[FVM_DESKTOP_LOAD_VARIABLES_METHOD], 0);
    if (status == FVM_OK)
        status = fvm_register_function_rooted_bytes(context, "unloadMovie", strlen("unloadMovie"),
                                                    unload_builtin, navigation, 0);
    if (status == FVM_OK)
        status = fvm_object_define_method_rooted(context, prototype.object, "unloadMovie",
                                                 unload_builtin, navigation, 0);
    if (status == FVM_OK)
        status = fvm_register_function_rooted_bytes(context, "loadMovieNum", strlen("loadMovieNum"),
                                                    load_numbered_builtin, navigation, 0);
    if (status == FVM_OK)
        status =
            fvm_register_function_rooted_bytes(context, "unloadMovieNum", strlen("unloadMovieNum"),
                                               unload_numbered_builtin, navigation, 0);
    return status;
}

/* [GC:] Notify only while the owner holds its ordinary root/input pins and no VM frame is active.
 * The copied snapshot never escapes; the callback owns its independent userdata. */
static FVM_Result playback_observe(FVM_DesktopPlaybackSession *control,
                                   FVM_DesktopPlaybackPhase phase, FVM_Context *context,
                                   FVM_Object *root, const char *source, unsigned long ticks,
                                   unsigned long presentations)
{
    FVM_DesktopPlaybackState state;
    FVM_DesktopPlaybackCommand command;
    if (!control || !control->policy.notify || control->stopped)
        return FVM_OK;
    memset(&state, 0, sizeof(state));
    state.struct_size = sizeof(state);
    state.phase = phase;
    if (!control->clock_started)
    {
        control->start_ms = fvm_desktop_now();
        control->clock_started = 1;
    }
    state.elapsed_ms = fvm_desktop_now() - control->start_ms;
    state.context = context;
    state.root = root;
    state.source = source;
    state.movie_ticks = ticks;
    state.total_ticks = control->total_ticks;
    state.presentations = presentations;
    state.paused = control->paused;
    state.step_pending = control->step_pending;
    command = control->policy.notify(&state, control->policy.userdata);
    return fvm_desktop_playback_apply(control, command) ? FVM_OK : FVM_INVALID_ARGUMENT;
}

/* Poll the native error latch without resuming the PCM producer while timeline control is paused.
 */
static FVM_Result playback_audio(FVM_DesktopAudio *audio, const FVM_DesktopPlaybackSession *control)
{
    return fvm_desktop_audio_tick_paused(audio, control && control->paused);
}

/* Play borrowed binary input. VM/SDL objects stay on this thread; source bytes are never freed. */
static int play_bytes(const FVM_DesktopOptions *options, const char *path, const void *bytes,
                      size_t length, int *quit, FVM_DesktopNavigation *navigation)
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    FVM_Object *clip = 0;
    FVM_Object *host_input = 0;
    FVM_Value host_value;
    FVM_DesktopInput input;
    FVM_DesktopAudio audio;
    FVM_DesktopPlaybackSession *control = navigation ? navigation->control : 0;
    FVM_DesktopWindow *window = 0;
    FVM_SWFInfo info;
    FVM_SWFStageBounds bounds;
    FVM_RasterSurface surface;
    FVM_Affine viewport;
    FVM_ExecutionMode mode;
    size_t stream_count = 0; /* Detect unsupported root streaming playback before native setup. */
    double authored, rate, width, height, period, next;
    unsigned long ticks = 0, limit = options->frames;
    unsigned long presentations = 0,
                  refreshes = 0; /* Count successful draws separately from timeline ticks. */
    int update_requested =
        0; /* Consume coalesced requests before presentation, preserving later requests. */
    unsigned int timer_callbacks =
        0;                        /* Receive each active-loop poll's attempted delivery count. */
    unsigned long timer_wait = 0; /* Cap SDL waiting at the nearest pending timer deadline. */
    int timers_pending = 0;       /* Receive the queue hint without retaining any managed state. */
    int status = 1, pinned = 0, input_pinned = 0;
    memset(&input, 0, sizeof(input));
    memset(&audio, 0, sizeof(audio));
    memset(&surface, 0, sizeof(surface));
    if (!checked(fvm_config_init(&config), "config", path))
        goto cleanup;
    if (!checked(fvm_runtime_create(&config, &runtime), "runtime", path))
        goto cleanup;
    if (!checked(fvm_context_create(runtime, &context), "context", path))
        goto cleanup;
    if (navigation && !checked(fvm_avm1_set_url_rooted(context, request_movie, navigation, 0),
                               "movie loader", path))
        goto cleanup;
    if (!checked(fvm_load_memory(context, bytes, length), "SWF load", path))
        goto cleanup;
    info.struct_size = sizeof(info);
    bounds.struct_size = sizeof(bounds);
    if (!checked(fvm_swf_info(context, &info), "SWF metadata", path))
        goto cleanup;
    if (!checked(fvm_swf_stage_bounds(context, &bounds), "stage rectangle", path))
        goto cleanup;
    if (!checked(fvm_swf_stream_sound_count(context, &stream_count), "root stream validation",
                 path))
        goto cleanup;
    if (stream_count && !options->mute)
    {
        fprintf(stderr,
                "%s: root streaming audio playback is unsupported; use --mute to play silently\n",
                path);
        goto cleanup;
    }
    authored = (double)info.frame_rate_fixed8 / FVM_DESKTOP_FIXED8_UNIT;
    if (!fvm_desktop_rate(options, authored, &rate))
    {
        fprintf(stderr, "%s: invalid effective FPS (use --fps for zero-rate SWFs)\n", path);
        goto cleanup;
    }
    width = ((double)bounds.x_max - (double)bounds.x_min) / FVM_SWF_TWIPS_PER_PIXEL;
    height = ((double)bounds.y_max - (double)bounds.y_min) / FVM_SWF_TWIPS_PER_PIXEL;
    if (!(width > 0 && height > 0 && width <= FVM_DESKTOP_MAX_PIXELS &&
          height <= FVM_DESKTOP_MAX_PIXELS))
    {
        fprintf(stderr, "%s: stage must fit 1..4096 pixels per dimension\n", path);
        goto cleanup;
    }
    surface.width = (unsigned int)width;
    if (surface.width < width)
        ++surface.width;
    surface.height = (unsigned int)height;
    if (surface.height < height)
        ++surface.height;
    surface.stride = surface.width * FVM_DESKTOP_RGBA_BYTES;
    surface.pixels = (unsigned char *)malloc(surface.stride * surface.height);
    if (!surface.pixels)
        goto cleanup;
    if (!checked(fvm_standard_install(context), "standard objects", path))
        goto cleanup;
    if (!checked(fvm_standard_keyboard_install(context), "Key polling namespace", path))
        goto cleanup;
    /* Mouse cursor methods start at SWF5; listener methods keep their own SWF6 gate. */
    if (info.version >= FVM_MOUSE_CURSOR_MIN_SWF_VERSION &&
        !checked(fvm_standard_mouse_install(context), "Mouse pointer namespace", path))
        goto cleanup;
    if (info.version >= FVM_SELECTION_MIN_SWF_VERSION &&
        !checked(fvm_standard_selection_install(context), "Selection MovieClip namespace", path))
        goto cleanup;
    /* Generic user events share no native Key/Mouse/Selection registration state. */
    if (info.version >= FVM_BROADCASTER_MIN_SWF_VERSION &&
        !checked(fvm_standard_broadcaster_install(context), "AsBroadcaster namespace", path))
        goto cleanup;
    if (!checked(fvm_movieclip_load_swf_ex(context, bytes, length, "_root",
                                           options->mute ? FVM_SWF_IMPORT_MUTE_ROOT_STREAMS : 0,
                                           &clip),
                 "scene import", path))
        goto cleanup;
    /* [GC:] Pin the published root before any script/frame operation can collect it. */
    if (!checked(fvm_gc_pin(context, clip), "root pin", path))
        goto cleanup;
    pinned = 1;
    if (navigation)
    {
        navigation->root = clip;
        navigation->audio = &audio;
        if (!checked(fvm_movieclip_set_source(context, clip, navigation->initial_base,
                                              strlen(navigation->initial_base)),
                     "source base", path))
            goto cleanup;
    }
    if (!checked(fvm_object_create(context, &host_input), "HostInput object", path))
        goto cleanup;
    /* [GC:] Keep the host-owned input object live even if a script replaces its global binding. */
    if (!checked(fvm_gc_pin(context, host_input), "HostInput pin", path))
        goto cleanup;
    input_pinned = 1;
    fvm_value_undefined(&host_value);
    host_value.type = FVM_VALUE_OBJECT;
    host_value.object = host_input;
    if (!checked(fvm_set_global(context, "HostInput", &host_value), "HostInput binding", path))
        goto cleanup;
    if (navigation &&
        !checked(install_load_functions(context, navigation), "loader functions", path))
        goto cleanup;
    mode = options->mode;
    if (mode == FVM_DESKTOP_MODE_AUTO)
        mode = fvm_jit_available() ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER;
    if (mode == FVM_EXECUTION_JIT && !fvm_jit_available())
    {
        fprintf(stderr, "%s: requested JIT backend unavailable\n", path);
        goto cleanup;
    }
    if (!checked(fvm_execution_set_mode(context, mode, 0), "execution mode", path))
        goto cleanup;
    if (!checked(fvm_stage_set_quality(context, options->quality), "quality", path))
        goto cleanup;
    memset(&viewport, 0, sizeof(viewport));
    viewport.xx = viewport.yy = 1;
    viewport.x0 = -(double)bounds.x_min / FVM_SWF_TWIPS_PER_PIXEL;
    viewport.y0 = -(double)bounds.y_min / FVM_SWF_TWIPS_PER_PIXEL;
    window = fvm_desktop_window_open(path, surface.width, surface.height);
    if (!window)
    {
        fprintf(stderr, "%s: cannot open desktop window\n", path);
        goto cleanup;
    }
    if (navigation && navigation->background)
    {
        navigation->background->window = window;
        navigation->background->audio = &audio;
    }
    if (options->mute && !checked(fvm_movieclip_set_sound_callback(context, muted_sound, &audio),
                                  "muted event sound", path))
        goto cleanup;
    if (!options->mute &&
        !checked(fvm_desktop_audio_open_paused(context, clip, &audio, control && control->paused),
                 "event audio (use --mute to disable)", path))
        goto cleanup;
    if (!limit && (options->once || options->file_count > 1))
        limit = info.frame_count;
    fprintf(stderr, "%s: SWF%u %ux%u source=%.6g fps effective=%.6g fps\n", path, info.version,
            surface.width, surface.height, authored, rate);
    period = FVM_DESKTOP_MILLISECONDS / rate;
    next = fvm_desktop_now();
    if (!checked(playback_observe(control, FVM_DESKTOP_PLAYBACK_READY, context, clip, path, ticks,
                                  presentations),
                 "playback control", path) ||
        (control && !checked(playback_audio(&audio, control), "initial controlled PCM", path)))
        goto cleanup;
    if (control && control->stopped)
        *quit = 1;
    while (!*quit && (!limit || ticks < limit) && !(navigation && navigation->pending))
    {
        double now;
        if (!fvm_desktop_window_events(window, 0))
        {
            *quit = 1;
            break;
        }
        if (!checked(fvm_desktop_window_status(window), "window events", path))
            goto cleanup;
        {
            int was_paused = control && control->paused;
            if (!checked(playback_observe(control, FVM_DESKTOP_PLAYBACK_TURN, context, clip, path,
                                          ticks, presentations),
                         "playback control", path))
                goto cleanup;
            if (control && control->stopped)
            {
                *quit = 1;
                break;
            }
            if (control && (was_paused != control->paused || control->step_pending))
                next = fvm_desktop_now();
        }
        if (!fvm_desktop_window_active(window))
        {
            if (audio.output &&
                !checked(fvm_desktop_audio_pause(&audio, 1), "background audio pause", path))
                goto cleanup;
            if (!fvm_desktop_window_events(window, FVM_DESKTOP_AUDIO_WAIT))
                *quit = 1;
            if (!checked(fvm_desktop_window_status(window), "background window events", path))
                goto cleanup;
            if (*quit)
                break;
            next = fvm_desktop_now();
            continue;
        }
        if (!checked(playback_audio(&audio, control), "PCM queue", path))
            goto cleanup;
        if (!checked(fvm_desktop_window_pump_clip_input(window, context, clip, -viewport.x0,
                                                        -viewport.y0),
                     "input listeners", path))
            goto cleanup;
        fvm_desktop_window_input(window, &input);
        if (!checked(publish_input(context, host_input, &input,
                                   (double)bounds.x_min / FVM_SWF_TWIPS_PER_PIXEL,
                                   (double)bounds.y_min / FVM_SWF_TWIPS_PER_PIXEL),
                     "host input", path))
            goto cleanup;
        /* Input may register timers; deliver due work even between timeline frames. */
        if (!checked(fvm_timers_poll(context, &timer_callbacks), "scheduled callbacks", path))
            goto cleanup;
        if (navigation &&
            !checked(service_requests(context, navigation), "deferred resource load", path))
            goto cleanup;
        if (navigation && navigation->background && navigation->background->closed)
        {
            *quit = 1;
            break;
        }
        if (navigation && navigation->pending)
            break;
        /* [GC:] Input listeners and HostInput publication also allocate without timers.
         * Step once per foreground turn after all callbacks return; roots stay pinned.
         * The collector threshold decides whether this safe point actually collects. */
        if (!checked(fvm_gc_step(context, 0), "event GC step", path))
            goto cleanup;
        if (!checked(fvm_desktop_window_sync_cursor(window, context), "cursor visibility", path))
            goto cleanup;
        now = fvm_desktop_now();
        if (!fvm_desktop_playback_can_tick(control))
        {
            int wait;
            if (!checked(fvm_stage_take_update(context, &update_requested), "paused redraw", path))
                goto cleanup;
            if (!presentations || update_requested)
            {
                if (!present_stage(context, clip, window, &surface, &viewport, path))
                    goto cleanup;
                ++presentations;
                ++refreshes;
            }
            if (!checked(fvm_timers_next_delay(context, FVM_DESKTOP_PLAYBACK_WAIT_MS, &timer_wait,
                                               &timers_pending),
                         "paused timer wakeup", path))
                goto cleanup;
            wait = (int)timer_wait;
            if (wait && !fvm_desktop_window_events(window, wait))
                *quit = 1;
            if (!checked(fvm_desktop_window_status(window), "paused window events", path))
                goto cleanup;
            next = fvm_desktop_now();
            continue;
        }
        if (now < next && !(control && control->paused && control->step_pending))
        {
            int wait;
            if (!checked(fvm_stage_take_update(context, &update_requested), "redraw request", path))
                goto cleanup;
            if (update_requested)
            {
                if (!present_stage(context, clip, window, &surface, &viewport, path))
                    goto cleanup;
                ++presentations;
                ++refreshes;
                /* A new request made during rendering must not be lost behind a long frame wait. */
                if (!checked(fvm_stage_get_update(context, &update_requested), "redraw state",
                             path))
                    goto cleanup;
                now = fvm_desktop_now();
                if (update_requested || now >= next)
                    continue;
            }
            wait = (int)(next - now);
            if (audio.output && wait > FVM_DESKTOP_AUDIO_WAIT)
                wait = FVM_DESKTOP_AUDIO_WAIT;
            /* Poll owner commands even when a slow, timer-free movie has no SDL events. */
            if (control && control->policy.notify && wait > (int)FVM_DESKTOP_PLAYBACK_WAIT_MS)
                wait = FVM_DESKTOP_PLAYBACK_WAIT_MS;
            if (wait < 1)
                wait = 1;
            /* Do not sleep past a pending deadline when the authored frame rate is low. */
            if (!checked(fvm_timers_next_delay(context, (unsigned long)wait, &timer_wait,
                                               &timers_pending),
                         "timer wakeup", path))
                goto cleanup;
            wait = (int)timer_wait;
            if (navigation && navigation->transfer && wait > (int)FVM_DESKTOP_BACKGROUND_WAIT_MS)
                wait =
                    FVM_DESKTOP_BACKGROUND_WAIT_MS; /* Publish completions even at low movie FPS. */
            if (!wait)
                continue; /* A deadline became due after polling; retry without a blocking wait. */
            if (!fvm_desktop_window_events(window, wait))
            {
                *quit = 1;
                break;
            }
            if (!checked(fvm_desktop_window_status(window), "waiting window events", path))
                goto cleanup;
            continue;
        }
        if (!checked(fvm_movieclip_advance(context, clip, 1), "timeline tick", path))
            goto cleanup;
        fvm_desktop_playback_consume_step(control);
        /* Deliver timers on the VM thread before rendering callback-driven scene changes. */
        if (!checked(fvm_timers_poll(context, &timer_callbacks), "scheduled callbacks", path))
            goto cleanup;
        if (!checked(playback_audio(&audio, control), "frame PCM queue", path))
            goto cleanup;
        if (!checked(fvm_stage_take_update(context, &update_requested), "frame redraw request",
                     path))
            goto cleanup;
        if (!checked(fvm_desktop_window_sync_cursor(window, context), "frame cursor visibility",
                     path))
            goto cleanup;
        if (!present_stage(context, clip, window, &surface, &viewport, path))
            goto cleanup;
        ++presentations;
        ++ticks;
        if (control)
            ++control->total_ticks;
        if (navigation &&
            !checked(service_requests(context, navigation), "frame resource load", path))
            goto cleanup;
        if (navigation && navigation->background && navigation->background->closed)
        {
            *quit = 1;
            break;
        }
        /* [GC:] Collect only at a safe host boundary; the root and its library remain pinned. */
        if (!checked(fvm_gc_step(context, 0), "GC step", path))
            goto cleanup;
        if (!checked(playback_observe(control, FVM_DESKTOP_PLAYBACK_TICK, context, clip, path,
                                      ticks, presentations),
                     "playback control", path) ||
            (control && !checked(playback_audio(&audio, control), "controlled PCM queue", path)))
            goto cleanup;
        if (control && control->stopped)
        {
            *quit = 1;
            break;
        }
        next += period;
        if (now - next > FVM_DESKTOP_LATE_FRAMES * period)
            next = now;
    }
    /* Keep the last bounded frame visible for its scheduled duration, with responsive close. */
    while (!*quit && !(navigation && navigation->pending) && !(control && control->paused) &&
           fvm_desktop_now() < next)
    {
        int wait;
        /* Final-frame visibility must not postpone an embedding host's STOP command. */
        if (!checked(playback_observe(control, FVM_DESKTOP_PLAYBACK_TURN, context, clip, path,
                                      ticks, presentations),
                     "tail playback control", path))
            goto cleanup;
        if (control && control->stopped)
        {
            *quit = 1;
            break;
        }
        if (control && control->paused)
            break; /* The frame bound already completed; a paused tail need not delay return. */
        wait = (int)(next - fvm_desktop_now());
        if (control && control->policy.notify && wait > (int)FVM_DESKTOP_PLAYBACK_WAIT_MS)
            wait = FVM_DESKTOP_PLAYBACK_WAIT_MS;
        if (!fvm_desktop_window_active(window))
        {
            if (audio.output &&
                !checked(fvm_desktop_audio_pause(&audio, 1), "tail background audio pause", path))
                goto cleanup;
        }
        else if (!checked(playback_audio(&audio, control), "tail PCM queue", path))
            goto cleanup;
        if (audio.output && wait > FVM_DESKTOP_AUDIO_WAIT)
            wait = FVM_DESKTOP_AUDIO_WAIT;
        if (wait < 1)
            wait = 1;
        if (!fvm_desktop_window_events(window, wait))
            *quit = 1;
        if (!checked(fvm_desktop_window_status(window), "final window events", path))
            goto cleanup;
    }
    if (options->stats)
    {
        FVM_ExecutionStats stats;
        stats.struct_size = sizeof(stats);
        if (!checked(fvm_execution_get_stats(context, &stats), "execution statistics", path))
            goto cleanup;
        fprintf(stderr,
                "input_keys=%lu input_clicks=%lu sound_events=%lu sound_stops=%lu audio_device=%d "
                "audio_underruns=%d\n",
                input.key_presses, input.clicks, audio.events, audio.stops, audio.device != 0,
                SDL_AtomicGet(&audio.underruns));
        fprintf(stderr, "presentations=%lu refreshes=%lu\n", presentations, refreshes);
        fprintf(stderr, "frames=%lu native=%lu interpreter=%lu fallbacks=%lu\n", ticks,
                stats.native_entries, stats.interpreter_entries, stats.fallbacks);
    }
    status = 0;
cleanup:
    if (navigation && navigation->background)
    {
        navigation->background->window = 0;
        navigation->background->audio = 0;
    }
    if (context)
        discard_requests(context, navigation);
    if (navigation)
        navigation->audio = 0; /* Never retain a pointer to this play_bytes stack frame. */
    /* Release both real-output and explicit-mute registrations before audio userdata cleanup. */
    if (context && clip)
        fvm_movieclip_set_sound_callback(context, 0, 0);
    fvm_desktop_audio_close(&audio);
    fvm_desktop_window_close(window);
    free(surface.pixels);
    /* [GC:] End the host root before destroying its context; no borrowed pointer escapes. */
    if (input_pinned)
        fvm_gc_unpin(context, host_input);
    if (pinned)
        fvm_gc_unpin(context, clip);
    if (context)
        fvm_context_destroy(context);
    if (runtime)
        fvm_runtime_destroy(runtime);
    return status;
}

/* File adapter owns its temporary bytes; buffer playback never assumes ownership. */
/* Validate shared scalar settings without requiring a filesystem playlist. */
static int memory_options(const FVM_DesktopOptions *options, const char *name,
                          FVM_DesktopOptions *value, const char **error)
{
    if (!options)
    {
        if (error)
            *error = "null player options";
        return 0;
    }
    *value = *options;
    value->files = &name;
    value->file_count = 1;
    if (!fvm_desktop_options_validate(value, error))
        return 0;
    /* The temporary validation path array must never escape this helper. */
    value->files = 0;
    return 1;
}

/* Borrow exact binary bytes, including NUL, for the duration of synchronous playback. */
int fvm_desktop_run_memory(const FVM_DesktopOptions *options, const void *bytes, size_t length,
                           const char *name, const char **error)
{
    FVM_DesktopSource source;
    FVM_DesktopLoader loader;
    source.kind = FVM_DESKTOP_SOURCE_MEMORY;
    source.bytes = bytes;
    source.size = length;
    source.location = name;
    fvm_desktop_loader_init(&loader);
    if (options)
        loader.max_loads = options->max_loads;
#ifdef FVM_DESKTOP_HTTP
    loader.fetch = fvm_desktop_http_fetch;
#elif defined(FVM_DESKTOP_SOCKET_HTTP)
    loader.fetch = fvm_desktop_socket_fetch;
#endif
    return fvm_desktop_run_source(options, &source, &loader, error);
}

/* Accumulator owns only transport bytes; sink errors remain latched even if ignored by a provider.
 */
typedef struct FVM_DesktopDownload
{
    unsigned char *bytes;  /* Owned contiguous binary input. */
    size_t size, capacity; /* Used bytes and actual allocation, both bounded. */
    FVM_Result status;     /* First write failure. */
} FVM_DesktopDownload;

/* Consume chunks with checked arithmetic; a failing transport never exposes a partial movie. */
static FVM_Result collect_movie(const void *bytes, size_t size, void *userdata)
{
    FVM_DesktopDownload *download = (FVM_DesktopDownload *)userdata;
    size_t needed, capacity;
    unsigned char *grown;
    if (download->status != FVM_OK)
        return download->status;
    if ((!bytes && size) || size > FVM_SWF_DEFAULT_LIMIT - download->size)
        return download->status = FVM_INVALID_ARGUMENT;
    needed = download->size + size;
    if (needed > download->capacity)
    {
        capacity = download->capacity ? download->capacity : FVM_DESKTOP_FETCH_INITIAL;
        while (capacity < needed)
            capacity = capacity > FVM_SWF_DEFAULT_LIMIT / 2 ? FVM_SWF_DEFAULT_LIMIT : capacity * 2;
        grown = (unsigned char *)realloc(download->bytes, capacity);
        if (!grown)
            return download->status = FVM_OUT_OF_MEMORY;
        download->bytes = grown;
        download->capacity = capacity;
    }
    if (size)
        memcpy(download->bytes + download->size, bytes, size);
    download->size = needed;
    return FVM_OK;
}

/* Count only accessible, terminated location text within the explicit protocol bound. */
static size_t location_size(const char *text)
{
    size_t length = 0;
    if (!text)
        return 0;
    while (length <= FVM_DESKTOP_MAX_LOCATION && text[length])
        ++length;
    return length <= FVM_DESKTOP_MAX_LOCATION ? length : 0;
}

/* Default to startup only; the host explicitly opts in to deferred SWF root loading. */
int fvm_desktop_loader_init(FVM_DesktopLoader *out)
{
    if (!out)
        return 0;
    memset(out, 0, sizeof(*out));
    out->max_loads = 1;
    return 1;
}

/* Join local-file references. URL resolution belongs to the supplied transport. */
static int local_location(const char *reference, const char *base, char *out)
{
    size_t length = location_size(reference), prefix = 0, i;
    if (!length)
        return 0;
    if (base && reference[0] != '/' && reference[0] != '\\' && !(length > 1 && reference[1] == ':'))
        for (i = 0; base[i]; ++i)
            if (base[i] == '/' || base[i] == '\\')
                prefix = i + 1;
    if (prefix > FVM_DESKTOP_MAX_LOCATION - length)
        return 0;
    if (prefix)
        memcpy(out, base, prefix);
    memcpy(out + prefix, reference, length + 1);
    return 1;
}

/* Drain bounded queued requests on the VM owner thread after script/native callbacks return. */
static FVM_Result service_requests(FVM_Context *context, FVM_DesktopNavigation *navigation)
{
    FVM_Result unloaded = service_unloads(context, navigation);
    if (unloaded != FVM_OK)
        return unloaded;
    /* A pin retains storage, not membership: removed ancestors invalidate an active target. */
    if (navigation->active_request)
    {
        int attached = 0;
        FVM_Result status = fvm_movieclip_attached_to(context, navigation->active_request->target,
                                                      navigation->root, &attached);
        if (status != FVM_OK)
            return status;
        if (!attached)
            abandon_active(context, navigation);
    }
    /* [GC:] The active request retains its explicit target root while the VM keeps running. */
    if (navigation->active_request)
    {
        FVM_DesktopTransferView view;
        FVM_DesktopRequest *request = navigation->active_request;
        unsigned int kind = request->flags & FVM_AVM1_URL_LOAD_VARIABLES
                                ? FVM_DESKTOP_LOAD_VARIABLES
                                : FVM_DESKTOP_LOAD_CHILD;
        FVM_Result status;
        memset(&view, 0, sizeof(view));
        status = background_poll(navigation->transfer, &view);
        if (status == FVM_OK && view.state == FVM_DESKTOP_TRANSFER_PENDING)
        {
            background_notice(navigation->background, navigation->transfer_id, kind,
                              FVM_DESKTOP_LOAD_WAIT, request->url, request->base, &view,
                              FVM_NOT_FOUND);
            return FVM_OK; /* Host stop is handled by owner loop then joined cleanup. */
        }
        if (status == FVM_OK)
            status = view.result;
        background_terminal(navigation->background, navigation->transfer_id, kind, request->url,
                            request->base, &view, status, 0);
        if (status == FVM_OK)
        {
            if (request->flags & FVM_AVM1_URL_LOAD_VARIABLES)
                status = fvm_object_load_variables(context, request->target, view.bytes, view.size);
            else
                status =
                    fvm_movieclip_replace_resource(context, request->target, view.bytes, view.size,
                                                   view.location, strlen(view.location));
        }
        fvm_desktop_transfer_release(navigation->transfer);
        navigation->transfer = 0;
        navigation->active_request = 0;
        fvm_gc_unpin(context, request->target);
        free(request->url);
        free(request->base);
        free(request);
        if (status != FVM_OK)
            return status;
    }
    while (navigation->head && !navigation->pending)
    {
        FVM_DesktopRequest *request = navigation->head;
        FVM_DesktopDownload download;
        unsigned char *file_bytes = 0;
        const void *bytes = 0;
        size_t size = 0;
        char resolved[FVM_DESKTOP_MAX_LOCATION + 1];
        int variables, attached = 0;
        FVM_Result status = service_unloads(context, navigation);
        if (status != FVM_OK)
            return status;
        if (!navigation->head)
            break;
        request = navigation->head;
        variables = (request->flags & FVM_AVM1_URL_LOAD_VARIABLES) != 0;
        navigation->head = request->next;
        if (!navigation->head)
            navigation->tail = 0;
        --navigation->count;
        status = fvm_movieclip_attached_to(context, request->target, navigation->root, &attached);
        if (status != FVM_OK || !attached)
        {
            release_request(context, request);
            if (status != FVM_OK)
                return status;
            continue; /* Detached storage is not a resource destination and consumes no budget. */
        }
        if (!navigation->remaining)
            status = FVM_UNSUPPORTED;
        else
            --navigation->remaining;
        /* Whole-root replacement retains its textual request beyond old-context cleanup. */
        if (status == FVM_OK && !variables && request->target == navigation->root)
        {
            navigation->pending = request->url;
            navigation->pending_base = request->base;
            request->url = request->base = 0;
        }
        else if (status == FVM_OK && navigation->background &&
                 navigation->background->asynchronous &&
                 (strstr(request->url, "://") || strstr(request->base, "://")))
        {
            status =
                background_begin(navigation->background, request->url, request->base,
                                 variables ? FVM_DESKTOP_LOAD_VARIABLES : FVM_DESKTOP_LOAD_CHILD,
                                 &navigation->transfer, &navigation->transfer_id);
            if (status == FVM_OK)
            {
                navigation->active_request = request;
                return FVM_OK; /* One FIFO resource; owner loop continues input/timers/timeline/GC.
                                */
            }
        }
        else if (status == FVM_OK)
        {
            memset(&download, 0, sizeof(download));
            download.status = FVM_OK;
            memset(resolved, 0, sizeof(resolved));
            if (strstr(request->url, "://") || strstr(request->base, "://"))
            {
                if (navigation->background)
                    navigation->background->kind =
                        variables ? FVM_DESKTOP_LOAD_VARIABLES : FVM_DESKTOP_LOAD_CHILD;
                status = navigation->policy->fetch
                             ? navigation->policy->fetch(request->url, request->base, collect_movie,
                                                         &download, resolved, sizeof(resolved),
                                                         navigation->policy->userdata)
                             : FVM_UNSUPPORTED;
                if (download.status != FVM_OK)
                    status = download.status;
                if (!memchr(resolved, 0, sizeof(resolved)) || !location_size(resolved))
                    status = FVM_INVALID_ARGUMENT;
                bytes = download.bytes;
                size = download.size;
            }
            else if (!local_location(request->url, request->base, resolved))
                status = FVM_INVALID_ARGUMENT;
            else
            {
                file_bytes = read_resource(resolved, &size, variables);
                if (!file_bytes)
                    status = FVM_ERROR;
                bytes = file_bytes;
            }
            if (navigation->background && navigation->background->closed)
                status = FVM_OK;
            else if (status == FVM_OK)
            {
                if (variables)
                    status = fvm_object_load_variables(context, request->target, bytes, size);
                else
                    status = fvm_movieclip_replace_resource(context, request->target, bytes, size,
                                                            resolved, strlen(resolved));
            }
            free(file_bytes);
            free(download.bytes);
        }
        /* [GC:] User setters have returned; drop only this queue entry's explicit root. */
        fvm_gc_unpin(context, request->target);
        free(request->url);
        free(request->base);
        free(request);
        if (navigation->background && navigation->background->closed)
            return FVM_OK;
        if (status != FVM_OK)
            return status;
    }
    return FVM_OK;
}

/* Keep transport and successive VM lifetimes disjoint; this is root replacement, not child import.
 */
static int run_source(const FVM_DesktopOptions *options, const FVM_DesktopSource *source,
                      const FVM_DesktopLoader *loader, const char **error, int *closed,
                      FVM_DesktopBackgroundSession *background, FVM_DesktopPlaybackSession *control)
{
    FVM_DesktopOptions value;
    FVM_DesktopLoader policy;
    FVM_DesktopNavigation navigation;
    FVM_DesktopDownload download;
    char location[FVM_DESKTOP_MAX_LOCATION + 1], resolved[FVM_DESKTOP_MAX_LOCATION + 1];
    const char *reference, *base = 0;
    const void *bytes;
    unsigned char *file_bytes;
    size_t size;
    unsigned int kind;
    char provenance[FVM_DESKTOP_MAX_LOCATION + 1];
    char *executable_base = 0;
    int quit = 0, status = FVM_DESKTOP_EXIT_ARGUMENT;
    FVM_Result fetched;
    memset(&navigation, 0, sizeof(navigation));
    if (error)
        *error = "invalid source or loader policy";
    if (!source || !location_size(source->location) ||
        (source->kind != FVM_DESKTOP_SOURCE_MEMORY && source->kind != FVM_DESKTOP_SOURCE_FILE &&
         source->kind != FVM_DESKTOP_SOURCE_URL) ||
        (source->kind == FVM_DESKTOP_SOURCE_MEMORY &&
         (!source->bytes || !source->size || source->size > FVM_SWF_DEFAULT_LIMIT)) ||
        (source->kind != FVM_DESKTOP_SOURCE_MEMORY && (source->bytes || source->size)))
        return status;
    fvm_desktop_loader_init(&policy);
    if (loader)
        policy = *loader;
    if (!policy.max_loads || policy.max_loads > FVM_DESKTOP_MAX_LOADS)
        return status;
    if (!memory_options(options, source->location, &value, error))
        return status;
    navigation.background = background;
    navigation.control = control;
    navigation.policy = &policy;
    navigation.remaining = policy.max_loads - 1;
    kind = source->kind;
    reference = source->location;
    for (;;)
    {
        file_bytes = 0;
        memset(&download, 0, sizeof(download));
        download.status = FVM_OK;
        bytes = source->bytes;
        size = source->size;
        status = FVM_DESKTOP_EXIT_PLAYBACK;
        if (kind == FVM_DESKTOP_SOURCE_URL)
        {
            memset(resolved, 0, sizeof(resolved));
            if (background)
                background->kind = FVM_DESKTOP_LOAD_ROOT;
            fetched = policy.fetch ? policy.fetch(reference, base, collect_movie, &download,
                                                  resolved, sizeof(resolved), policy.userdata)
                                   : FVM_UNSUPPORTED;
            if (background && background->closed)
            {
                free(download.bytes);
                status = FVM_DESKTOP_EXIT_OK;
                quit = 1;
                break;
            }
            if (fetched != FVM_OK || download.status != FVM_OK || !download.size ||
                !memchr(resolved, 0, sizeof(resolved)) || !location_size(resolved))
            {
                fprintf(stderr, "%s: source fetch failed (transport=%d sink=%d)\n", reference,
                        (int)fetched, (int)download.status);
                free(download.bytes);
                break;
            }
            bytes = download.bytes;
            size = download.size;
        }
        else if (!local_location(reference, kind == FVM_DESKTOP_SOURCE_FILE ? base : 0, resolved))
            break;
        if (kind == FVM_DESKTOP_SOURCE_FILE)
        {
            file_bytes = read_movie(resolved, &size);
            if (!file_bytes)
                break;
            bytes = file_bytes;
        }
        memcpy(location, resolved, strlen(resolved) + 1);
        navigation.initial_base = location;
        if (kind == FVM_DESKTOP_SOURCE_MEMORY && location[0] != '/' && location[0] != '\\' &&
            !(strlen(location) > 2 && location[1] == ':' &&
              (location[2] == '/' || location[2] == '\\')) &&
            strncmp(location, "http://", strlen("http://")) &&
            strncmp(location, "https://", strlen("https://")))
        {
            executable_base = FVM_DESKTOP_EXECUTABLE_BASE();
            if (!executable_base || !local_location("embedded.swf", executable_base, provenance))
            {
                /* Hosts with restricted OS path discovery may supply an absolute source name.
                 * Never silently substitute process CWD for the executable directory. */
                navigation.initial_base = "unavailable://executable/embedded.swf";
            }
            else
                navigation.initial_base = provenance;
            SDL_free(executable_base);
        }
        /* The pending reference belongs to the previous VM and is no longer needed. */
        free(navigation.pending);
        free(navigation.pending_base);
        navigation.pending = navigation.pending_base = 0;
        status = play_bytes(&value, location, bytes, size, &quit, &navigation);
        free(file_bytes);
        free(download.bytes);
        if (status || quit || !navigation.pending)
            break;
        reference = navigation.pending;
        base = navigation.pending_base;
        kind = strstr(reference, "://") || (base && strstr(base, "://")) ? FVM_DESKTOP_SOURCE_URL
                                                                         : FVM_DESKTOP_SOURCE_FILE;
    }
    free(navigation.pending);
    free(navigation.pending_base);
    if (closed)
        *closed = quit;
    if (error)
        *error = status ? "source fetch, import, navigation limit or playback failed" : 0;
    return status;
}

/* Public source runner leaves playlist orchestration to the caller. */
int fvm_desktop_run_source(const FVM_DesktopOptions *options, const FVM_DesktopSource *source,
                           const FVM_DesktopLoader *loader, const char **error)
{
    return run_source(options, source, loader, error, 0, 0, 0);
}

/* Initialize an explicitly selected worker provider without native startup. */
int fvm_desktop_background_loader_init(FVM_DesktopBackgroundLoader *out)
{
    if (!out)
        return 0;
    fvm_desktop_transfer_options_init(&out->transfer);
    out->max_loads = 1;
    return 1;
}
/* Private session never escapes this synchronous owner-thread call. */
static int run_background(const FVM_DesktopOptions *options, const FVM_DesktopSource *source,
                          const FVM_DesktopBackgroundLoader *loader, const char **error,
                          int *closed, int asynchronous, const FVM_DesktopLoadMonitor *monitor,
                          FVM_DesktopPlaybackSession *control)
{
    FVM_DesktopBackgroundSession session;
    FVM_DesktopLoader policy;
    if (!loader || !loader->transfer.fetch || !loader->transfer.max_bytes ||
        loader->transfer.max_bytes > FVM_DESKTOP_TRANSFER_MAX_BYTES)
    {
        if (error)
            *error = "invalid background provider policy";
        return FVM_DESKTOP_EXIT_ARGUMENT;
    }
    memset(&session, 0, sizeof(session));
    session.options = loader->transfer;
    session.asynchronous = asynchronous;
    session.monitor = monitor;
    session.control = control;
    policy.fetch = background_fetch;
    policy.userdata = &session;
    policy.max_loads = loader->max_loads;
    return run_source(options, source, &policy, error, closed, &session, control);
}
/* Native app/IDE entrypoint keeps provider opt-in explicit instead of moving old callbacks. */
int fvm_desktop_run_source_background(const FVM_DesktopOptions *options,
                                      const FVM_DesktopSource *source,
                                      const FVM_DesktopBackgroundLoader *loader, const char **error)
{
    return run_background(options, source, loader, error, 0, 0, 0, 0);
}
/* Default-provider playlist shares the same import/GC/render/cleanup implementation. */
static int run_background_playlist(const FVM_DesktopOptions *options, const char **error,
                                   int asynchronous, const FVM_DesktopLoadMonitor *monitor)
{
    FVM_DesktopOptions value;
    FVM_DesktopBackgroundLoader loader;
    FVM_DesktopSource source;
    size_t i;
    int status = FVM_DESKTOP_EXIT_OK, closed = 0;
    if (!fvm_desktop_options_validate(options, error))
        return FVM_DESKTOP_EXIT_ARGUMENT;
    fvm_desktop_background_loader_init(&loader);
#ifdef FVM_DESKTOP_SOCKET_HTTP
    loader.transfer.fetch = fvm_desktop_socket_transfer_fetch;
#elif defined(FVM_DESKTOP_HTTP)
    loader.transfer.fetch = fvm_desktop_http_transfer_fetch;
#else
    return fvm_desktop_run(options, error);
#endif
    value = *options;
    loader.max_loads = value.max_loads;
    if (value.file_count > 1 && !value.frames)
        value.once = 1;
    source.bytes = 0;
    source.size = 0;
    for (i = 0; i < value.file_count; ++i)
    {
        source.location = value.files[i];
        source.kind =
            strstr(source.location, "://") ? FVM_DESKTOP_SOURCE_URL : FVM_DESKTOP_SOURCE_FILE;
        status = run_background(&value, &source, &loader, error, &closed, asynchronous, monitor, 0);
        if (status || closed)
            break;
    }
    return status;
}

/* Preserve the explicit event-pumping/waiting mode introduced before asynchronous delivery. */
int fvm_desktop_run_background(const FVM_DesktopOptions *options, const char **error)
{
    return run_background_playlist(options, error, 0, 0);
}
/* Keep child/form transfer completion separate from active timeline/timer execution. */
int fvm_desktop_run_source_async(const FVM_DesktopOptions *options, const FVM_DesktopSource *source,
                                 const FVM_DesktopBackgroundLoader *loader, const char **error)
{
    return run_background(options, source, loader, error, 0, 1, 0, 0);
}
/* Async playlist adapter retains ordinary whole-root replacement and close semantics. */
int fvm_desktop_run_async(const FVM_DesktopOptions *options, const char **error)
{
    return run_background_playlist(options, error, 1, 0);
}

/* Zero-cost observer initialization, independent of VM/window/provider startup. */
int fvm_desktop_load_monitor_init(FVM_DesktopLoadMonitor *out)
{
    if (!out)
        return 0;
    out->asynchronous = 1;
    out->timeout_ms = 0;
    out->notify = 0;
    out->userdata = 0;
    return 1;
}
/* Common validation is used by both source and playlist adapters before startup. */
static int load_monitor_valid(const FVM_DesktopLoadMonitor *monitor, const char **error)
{
    if (!monitor || monitor->asynchronous > 1 ||
        monitor->timeout_ms > FVM_DESKTOP_TRANSFER_MAX_TIMEOUT_MS)
    {
        if (error)
            *error = "invalid load monitor policy";
        return 0;
    }
    return 1;
}
/* Validate every observer scalar before routing through the existing owner-thread runner. */
int fvm_desktop_run_source_monitored(const FVM_DesktopOptions *options,
                                     const FVM_DesktopSource *source,
                                     const FVM_DesktopBackgroundLoader *loader,
                                     const FVM_DesktopLoadMonitor *monitor, const char **error)
{
    if (!load_monitor_valid(monitor, error))
        return FVM_DESKTOP_EXIT_ARGUMENT;
    return run_background(options, source, loader, error, 0, (int)monitor->asynchronous, monitor,
                          0);
}

/* Thin default-provider adapter preserves the same observation and ownership rules. */
int fvm_desktop_run_monitored(const FVM_DesktopOptions *options,
                              const FVM_DesktopLoadMonitor *monitor, const char **error)
{
    if (!load_monitor_valid(monitor, error))
        return FVM_DESKTOP_EXIT_ARGUMENT;
    return run_background_playlist(options, error, (int)monitor->asynchronous, monitor);
}

/* Validate control before any source/provider/VM/window startup. */
int fvm_desktop_run_source_controlled(const FVM_DesktopOptions *options,
                                      const FVM_DesktopSource *source,
                                      const FVM_DesktopLoader *loader,
                                      const FVM_DesktopPlaybackControl *control, const char **error)
{
    FVM_DesktopPlaybackSession session;
    if (!fvm_desktop_playback_session_init(&session, control))
    {
        if (error)
            *error = "invalid playback control policy";
        return FVM_DESKTOP_EXIT_ARGUMENT;
    }
    return run_source(options, source, loader, error, 0, 0, &session);
}

/* Preserve explicit worker/monitor policy; callbacks and all VM work remain on the owner. */
int fvm_desktop_run_source_background_controlled(const FVM_DesktopOptions *options,
                                                 const FVM_DesktopSource *source,
                                                 const FVM_DesktopBackgroundLoader *loader,
                                                 const FVM_DesktopLoadMonitor *monitor,
                                                 const FVM_DesktopPlaybackControl *control,
                                                 const char **error)
{
    FVM_DesktopPlaybackSession session;
    if (!fvm_desktop_playback_session_init(&session, control))
    {
        if (error)
            *error = "invalid playback control policy";
        return FVM_DESKTOP_EXIT_ARGUMENT;
    }
    if (!load_monitor_valid(monitor, error))
        return FVM_DESKTOP_EXIT_ARGUMENT;
    return run_background(options, source, loader, error, 0, (int)monitor->asynchronous, monitor,
                          &session);
}

/* This host initializer stays linkable in SDKs with no HTTP provider. */
int fvm_desktop_network_policy_init(FVM_DesktopHTTPPolicy *out)
{
    if (!out)
        return 0;
    out->max_redirects = 0;
    out->connect_ms = FVM_NET_CONNECT_MS;
    out->total_ms = FVM_NET_TOTAL_MS;
    out->ca_file = 0;
    return 1;
}
/* Reject malformed limits before a callback, worker, window or input source can be opened. */
static int network_policy_valid(const FVM_DesktopHTTPPolicy *policy, const char **error)
{
    if (!policy || policy->max_redirects > FVM_HTTP_REDIRECT_MAX || !policy->connect_ms ||
        policy->connect_ms > policy->total_ms || policy->total_ms > FVM_NET_MAX_MS)
    {
        if (error)
            *error = "invalid HTTP network policy";
        return 0;
    }
    return 1;
}
/* Keep close propagation private so a stopped source also ends the outer playlist. */
static int run_network_source(const FVM_DesktopOptions *options, const FVM_DesktopSource *source,
                              const FVM_DesktopHTTPPolicy *policy,
                              const FVM_DesktopLoadMonitor *monitor, const char **error,
                              int *closed)
{
#if defined(FVM_DESKTOP_SOCKET_HTTP) || defined(FVM_DESKTOP_HTTP)
    FVM_DesktopBackgroundLoader loader;
    fvm_desktop_background_loader_init(&loader);
#ifdef FVM_DESKTOP_SOCKET_HTTP
    loader.transfer.fetch = fvm_desktop_socket_transfer_policy;
#else
    loader.transfer.fetch = fvm_desktop_http_transfer_policy;
#endif
    loader.transfer.userdata = (void *)policy; /* Provider only borrows the const descriptor. */
    loader.max_loads = options ? options->max_loads : 1;
    return run_background(options, source, &loader, error, closed, (int)monitor->asynchronous,
                          monitor, 0);
#else
    FVM_DesktopLoader loader;
    (void)policy;
    (void)monitor;
    fvm_desktop_loader_init(&loader);
    loader.max_loads = options ? options->max_loads : 1;
    return run_source(options, source, &loader, error, closed, 0, 0);
#endif
}
/* IDE entrypoint uses the same resource FIFO and owner-thread monitor as CLI policy playback. */
int fvm_desktop_run_source_network(const FVM_DesktopOptions *options,
                                   const FVM_DesktopSource *source,
                                   const FVM_DesktopHTTPPolicy *policy,
                                   const FVM_DesktopLoadMonitor *monitor, const char **error)
{
    if (!network_policy_valid(policy, error) || !load_monitor_valid(monitor, error))
        return FVM_DESKTOP_EXIT_ARGUMENT;
    return run_network_source(options, source, policy, monitor, error, 0);
}
/* Logical-resource budgets and optional once semantics are shared across each source run. */
int fvm_desktop_run_network(const FVM_DesktopOptions *options, const FVM_DesktopHTTPPolicy *policy,
                            const FVM_DesktopLoadMonitor *monitor, const char **error)
{
    FVM_DesktopOptions value;
    FVM_DesktopSource source;
    size_t i;
    int status = FVM_DESKTOP_EXIT_OK, closed = 0;
    if (!network_policy_valid(policy, error) || !load_monitor_valid(monitor, error) ||
        !fvm_desktop_options_validate(options, error))
        return FVM_DESKTOP_EXIT_ARGUMENT;
    value = *options;
    if (value.file_count > 1 && !value.frames)
        value.once = 1;
    source.bytes = 0;
    source.size = 0;
    for (i = 0; i < value.file_count; ++i)
    {
        source.location = value.files[i];
        source.kind =
            strstr(source.location, "://") ? FVM_DESKTOP_SOURCE_URL : FVM_DESKTOP_SOURCE_FILE;
        status = run_network_source(&value, &source, policy, monitor, error, &closed);
        if (status || closed)
            break;
    }
    return status;
}

/* Shared synchronous runner; the CLI translates arguments into this same contract. */
int fvm_desktop_run(const FVM_DesktopOptions *options, const char **error)
{
    FVM_DesktopOptions value;
    FVM_DesktopSource source;
    FVM_DesktopLoader loader;
    size_t i;
    int status = FVM_DESKTOP_EXIT_OK, closed = 0;
    if (!fvm_desktop_options_validate(options, error))
        return FVM_DESKTOP_EXIT_ARGUMENT;
    value = *options;
    fvm_desktop_loader_init(&loader);
    loader.max_loads = value.max_loads;
#ifdef FVM_DESKTOP_HTTP
    loader.fetch = fvm_desktop_http_fetch;
#elif defined(FVM_DESKTOP_SOCKET_HTTP)
    loader.fetch = fvm_desktop_socket_fetch;
#endif
    if (value.file_count > 1 && !value.frames)
        value.once = 1;
    source.bytes = 0;
    source.size = 0;
    for (i = 0; i < value.file_count; ++i)
    {
        source.location = value.files[i];
        source.kind =
            strstr(source.location, "://") ? FVM_DESKTOP_SOURCE_URL : FVM_DESKTOP_SOURCE_FILE;
        status = run_source(&value, &source, &loader, error, &closed, 0, 0);
        if (status || closed)
            break;
    }
    return status;
}

/* Console-only coalescing state, borrowed by the same owner-thread C monitor used by IDEs. */
typedef struct FVM_DesktopCLIProgress
{
    unsigned long id; /* Last reported source-run request ordinal. */
    size_t received;  /* Last emitted body-byte count; WAIT duplicates are suppressed. */
} FVM_DesktopCLIProgress;
/* Numeric metadata avoids printing credentials or control characters from an input URL. */
static int cli_load_progress(const FVM_DesktopLoadEvent *event, void *userdata)
{
    FVM_DesktopCLIProgress *state = (FVM_DesktopCLIProgress *)userdata;
    if (event->phase != FVM_DESKTOP_LOAD_WAIT || event->id != state->id ||
        event->received != state->received)
        fprintf(stderr, "load id=%lu kind=%u phase=%u received=%lu result=%d\n", event->id,
                event->kind, event->phase, (unsigned long)event->received, (int)event->result);
    state->id = event->id;
    state->received = event->received;
    return 0;
}
/* Strict decimal integer milliseconds, independent of locale/sign/exponent and unsigned width. */
static int cli_load_timeout(const char *text, unsigned long *out)
{
    unsigned long value = 0;
    size_t length = 0;
    if (!text || !*text)
        return 0;
    while (*text)
    {
        unsigned int digit;
        if (++length > FVM_DESKTOP_DECIMAL_MAX_TEXT || *text < '0' || *text > '9')
            return 0;
        digit = (unsigned int)(*text++ - '0');
        if (value > (FVM_DESKTOP_TRANSFER_MAX_TIMEOUT_MS - digit) / 10UL)
            return 0;
        value = value * 10UL + digit;
    }
    *out = value;
    return 1;
}

/* Parse before SDL/VM startup so help and invalid arguments work without a display server. */
int fvm_desktop_main(int argc, char **argv)
{
    FVM_DesktopOptions options;
    FVM_DesktopLoadMonitor monitor;
    FVM_DesktopHTTPPolicy network;
    unsigned long redirects;
    int redirects_selected = 0, ca_selected = 0;
    FVM_DesktopCLIProgress progress;
    int timeout_selected = 0, progress_selected = 0;
    const char *error = 0;
    const char **files;
    char **arguments;
    int i, count = 1, literal = 0, background = 0;
    int parsed, status = FVM_DESKTOP_EXIT_OK;
    if (!fvm_desktop_arguments_valid(argc, argv))
    {
        fprintf(stderr, "invalid player argument vector\n");
        return FVM_DESKTOP_EXIT_ARGUMENT;
    }
    files = (const char **)malloc((size_t)argc * sizeof(*files));
    if (!files)
        return FVM_DESKTOP_EXIT_PLAYBACK;
    arguments = (char **)malloc((size_t)argc * sizeof(*arguments));
    if (!arguments)
    {
        free(files);
        return FVM_DESKTOP_EXIT_PLAYBACK;
    }
    fvm_desktop_load_monitor_init(&monitor);
    fvm_desktop_network_policy_init(&network);
    memset(&progress, 0, sizeof(progress));
    arguments[0] = argv[0];
    for (i = 1; i < argc; ++i)
    {
        if (!literal && !strcmp(argv[i], "--http-ca-file"))
        {
            if (ca_selected || i + 1 >= argc || !location_size(argv[i + 1]))
            {
                error = "invalid or duplicate --http-ca-file";
                goto argument_failure;
            }
            ca_selected = 1;
            network.ca_file = argv[++i];
        }
        else if (!literal && !strcmp(argv[i], "--redirects"))
        {
            if (redirects_selected || i + 1 >= argc || !cli_load_timeout(argv[i + 1], &redirects) ||
                redirects > FVM_HTTP_REDIRECT_MAX)
            {
                error = "invalid or duplicate --redirects (0..8 integer hops)";
                goto argument_failure;
            }
            redirects_selected = 1;
            network.max_redirects = (unsigned int)redirects;
            ++i;
        }
        else if (!literal && !strcmp(argv[i], "--load-timeout"))
        {
            if (timeout_selected || i + 1 >= argc ||
                !cli_load_timeout(argv[i + 1], &monitor.timeout_ms))
            {
                error = "invalid or duplicate --load-timeout (0..2147483647 integer ms)";
                goto argument_failure;
            }
            timeout_selected = 1;
            ++i;
        }
        else if (!literal && !strcmp(argv[i], "--load-progress"))
        {
            if (progress_selected)
            {
                error = "duplicate --load-progress";
                goto argument_failure;
            }
            progress_selected = 1;
            monitor.notify = cli_load_progress;
            monitor.userdata = &progress;
        }
        else if (!literal &&
                 (!strcmp(argv[i], "--background-loads") || !strcmp(argv[i], "--async-loads")))
        {
            if (background)
            {
                fprintf(stderr, "duplicate --background-loads/--async-loads selection\n");
                free(arguments);
                free(files);
                return FVM_DESKTOP_EXIT_ARGUMENT;
            }
            background = !strcmp(argv[i], "--async-loads") ? 2 : 1;
        }
        else
        {
            if (!strcmp(argv[i], "--"))
                literal = 1;
            arguments[count++] = argv[i];
        }
    }
    if ((timeout_selected || progress_selected || redirects_selected || ca_selected) && !background)
    {
        error =
            "--load-timeout/--load-progress/--redirects/--http-ca-file requires --async-loads or "
            "--background-loads";
        goto argument_failure;
    }
    monitor.asynchronous = background == 2;
    parsed = fvm_desktop_parse(count, arguments, files, &options, &error);
    if (parsed == FVM_DESKTOP_PARSE_HELP)
        fvm_desktop_help(argv[0]);
    else if (parsed == FVM_DESKTOP_PARSE_VERSION)
        print_build_info();
    else if (parsed == FVM_DESKTOP_PARSE_ERROR)
    {
        fprintf(stderr, "%s\nUse --help for options.\n", error ? error : "invalid arguments");
        status = FVM_DESKTOP_EXIT_ARGUMENT;
    }
    else
    {
        status = (redirects_selected || ca_selected)
                     ? fvm_desktop_run_network(&options, &network, &monitor, &error)
                 : (timeout_selected || progress_selected)
                     ? fvm_desktop_run_monitored(&options, &monitor, &error)
                 : background == 2 ? fvm_desktop_run_async(&options, &error)
                 : background == 1 ? fvm_desktop_run_background(&options, &error)
                                   : fvm_desktop_run(&options, &error);
        if (status == FVM_DESKTOP_EXIT_ARGUMENT)
            fprintf(stderr, "%s\n", error);
    }
    free(arguments);
    free(files);
    return status;
argument_failure:
    fprintf(stderr, "%s\n", error);
    free(arguments);
    free(files);
    return FVM_DESKTOP_EXIT_ARGUMENT;
}
