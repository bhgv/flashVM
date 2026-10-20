/* Host-owned desktop playback: load, root, advance, rasterize, present, release. */
#if defined(__APPLE__) && !defined(_DARWIN_C_SOURCE)
#define _DARWIN_C_SOURCE 1 /* Expose native executable-path/realpath declarations in C89. */
#endif
#if defined(__ANDROID__) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L /* Expose readlink without depending on process CWD. */
#endif
#define SDL_MAIN_HANDLED
#include "player.h"
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
    if (SDL_WasInit(0))
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

/* A copied request owns its text and one explicit target root until safe-point delivery. */
typedef struct FVM_DesktopRequest
{
    char *url;                       /* Owned reference copied from the VM's counted string. */
    char *base;                      /* Owned origin provenance, independent of selected target. */
    FVM_Object *target;              /* [GC:] Explicitly pinned while queued. */
    unsigned int flags;              /* LOAD_TARGET or LOAD_VARIABLES with optional GET. */
    struct FVM_DesktopRequest *next; /* Native FIFO link. */
} FVM_DesktopRequest;
/* Per-run transport and request queue; no mutable process-wide registration. */
typedef struct FVM_DesktopNavigation
{
    char *pending;                   /* Owned next root URL after the old VM is released. */
    char *pending_base;              /* Owned provenance for the next root request. */
    FVM_DesktopRequest *head, *tail; /* Owned FIFO, drained only outside VM callbacks. */
    size_t count;                    /* Bound queued requests by the remaining load budget. */
    FVM_Object *root;                /* Borrowed active root, pinned by play_bytes. */
    const char *initial_base; /* Borrowed initial canonical location or executable resource path. */
    const FVM_DesktopLoader *policy; /* Borrowed for the entire source run. */
    unsigned long remaining;         /* Remaining requests, including child and variable fetches. */
} FVM_DesktopNavigation;

/* Service is defined after the bounded transport helpers below. */
static FVM_Result service_requests(FVM_Context *, FVM_DesktopNavigation *);

/* [GC:] Release all native FIFO ownership before the corresponding context is destroyed. */
static void discard_requests(FVM_Context *context, FVM_DesktopNavigation *navigation)
{
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
    }
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

/* Resolve and retain target/origin inside the active frame; transport waits for a safe point. */
static FVM_Result request_movie(FVM_Context *context, FVM_Object *receiver, unsigned int argc,
                                const FVM_Value *args, FVM_Value *result, void *userdata)
{
    FVM_DesktopNavigation *navigation = (FVM_DesktopNavigation *)userdata;
    FVM_DesktopRequest *request;
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
    request = (FVM_DesktopRequest *)calloc(1, sizeof(*request));
    if (!request)
        return FVM_OUT_OF_MEMORY;
    request->url = copy_location(args + FVM_DESKTOP_URL_URL, &status);
    request->base = copy_location(&base, &status);
    request->target = target.object;
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

/* Immutable descriptors distinguish global and MovieClip entrypoints without pointer casts. */
typedef struct FVM_DesktopLoadBuiltin
{
    unsigned int flags; /* LOAD_TARGET or LOAD_VARIABLES request family. */
    int method;         /* Nonzero for MovieClip receiver form. */
} FVM_DesktopLoadBuiltin;
static const FVM_DesktopLoadBuiltin load_movie_global = {FVM_AVM1_URL_LOAD_TARGET, 0};
static const FVM_DesktopLoadBuiltin load_variables_global = {FVM_AVM1_URL_LOAD_VARIABLES, 0};
static const FVM_DesktopLoadBuiltin load_movie_method = {FVM_AVM1_URL_LOAD_TARGET, 1};
static const FVM_DesktopLoadBuiltin load_variables_method = {FVM_AVM1_URL_LOAD_VARIABLES, 1};

/* Standard host-facing loader spellings reuse the VM's counted/coerced URL service. */
static FVM_Result load_builtin(FVM_Context *context, FVM_Object *receiver, unsigned int argc,
                               const FVM_Value *args, FVM_Value *result, void *userdata)
{
    const FVM_DesktopLoadBuiltin *builtin = (const FVM_DesktopLoadBuiltin *)userdata;
    FVM_Value target, owner;
    int method_form = builtin->method;
    unsigned int method_index;
    FVM_Result status;
    if (!argc || !args)
        return FVM_INVALID_ARGUMENT;
    /* AVM1 unqualified names may resolve through a clip prototype before globals.
     * Preserve both public argument shapes when that shared spelling is inherited. */
    if (method_form && argc >= FVM_DESKTOP_LOAD_GLOBAL_MIN_ARGS &&
        (argc >= FVM_DESKTOP_LOAD_GLOBAL_FULL_ARGS ||
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
    {
        fvm_value_undefined(&owner);
        owner.type = FVM_VALUE_OBJECT;
        owner.object = receiver;
        status = fvm_exec_target_path(context, &owner, &target);
        if (status != FVM_OK)
            return status;
    }
    else
        target = args[FVM_DESKTOP_LOAD_TARGET_INDEX];
    return fvm_exec_url(context, args, &target, builtin->flags);
}

/* Install loader facades in the optional host only; the standalone VM remains transport-neutral. */
static FVM_Result install_load_functions(FVM_Context *context)
{
    FVM_Value constructor, prototype;
    FVM_Result status = fvm_register_function_rooted_bytes(
        context, "loadMovie", strlen("loadMovie"), load_builtin, (void *)&load_movie_global, 0);
    if (status == FVM_OK)
        status =
            fvm_register_function_rooted_bytes(context, "loadVariables", strlen("loadVariables"),
                                               load_builtin, (void *)&load_variables_global, 0);
    if (status == FVM_OK)
        status = fvm_get_global(context, "MovieClip", &constructor);
    if (status == FVM_OK && constructor.type == FVM_VALUE_OBJECT)
        status = fvm_object_get(context, constructor.object, "prototype", &prototype);
    else if (status == FVM_OK)
        status = FVM_INVALID_ARGUMENT;
    if (status == FVM_OK && prototype.type == FVM_VALUE_OBJECT)
        status = fvm_object_define_method_rooted(context, prototype.object, "loadMovie",
                                                 load_builtin, (void *)&load_movie_method, 0);
    else if (status == FVM_OK)
        status = FVM_INVALID_ARGUMENT;
    if (status == FVM_OK)
        status = fvm_object_define_method_rooted(context, prototype.object, "loadVariables",
                                                 load_builtin, (void *)&load_variables_method, 0);
    return status;
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
    if (navigation && !checked(install_load_functions(context), "loader functions", path))
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
    if (options->mute && !checked(fvm_movieclip_set_sound_callback(context, muted_sound, &audio),
                                  "muted event sound", path))
        goto cleanup;
    if (!options->mute && !checked(fvm_desktop_audio_open(context, clip, &audio),
                                   "event audio (use --mute to disable)", path))
        goto cleanup;
    if (!limit && (options->once || options->file_count > 1))
        limit = info.frame_count;
    fprintf(stderr, "%s: SWF%u %ux%u source=%.6g fps effective=%.6g fps\n", path, info.version,
            surface.width, surface.height, authored, rate);
    period = FVM_DESKTOP_MILLISECONDS / rate;
    next = fvm_desktop_now();
    while ((!limit || ticks < limit) && !(navigation && navigation->pending))
    {
        double now;
        if (!fvm_desktop_window_events(window, 0))
        {
            *quit = 1;
            break;
        }
        if (!checked(fvm_desktop_window_status(window), "window events", path))
            goto cleanup;
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
        if (!checked(fvm_desktop_audio_tick(&audio), "PCM queue", path))
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
        if (now < next)
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
            if (wait < 1)
                wait = 1;
            /* Do not sleep past a pending deadline when the authored frame rate is low. */
            if (!checked(fvm_timers_next_delay(context, (unsigned long)wait, &timer_wait,
                                               &timers_pending),
                         "timer wakeup", path))
                goto cleanup;
            wait = (int)timer_wait;
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
        /* Deliver timers on the VM thread before rendering callback-driven scene changes. */
        if (!checked(fvm_timers_poll(context, &timer_callbacks), "scheduled callbacks", path))
            goto cleanup;
        if (!checked(fvm_desktop_audio_tick(&audio), "frame PCM queue", path))
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
        if (navigation &&
            !checked(service_requests(context, navigation), "frame resource load", path))
            goto cleanup;
        /* [GC:] Collect only at a safe host boundary; the root and its library remain pinned. */
        if (!checked(fvm_gc_step(context, 0), "GC step", path))
            goto cleanup;
        next += period;
        if (now - next > FVM_DESKTOP_LATE_FRAMES * period)
            next = now;
    }
    /* Keep the last bounded frame visible for its scheduled duration, with responsive close. */
    while (!*quit && !(navigation && navigation->pending) && fvm_desktop_now() < next)
    {
        int wait = (int)(next - fvm_desktop_now());
        if (!fvm_desktop_window_active(window))
        {
            if (audio.output &&
                !checked(fvm_desktop_audio_pause(&audio, 1), "tail background audio pause", path))
                goto cleanup;
        }
        else if (!checked(fvm_desktop_audio_tick(&audio), "tail PCM queue", path))
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
    if (context)
        discard_requests(context, navigation);
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
    while (navigation->head && !navigation->pending)
    {
        FVM_DesktopRequest *request = navigation->head;
        FVM_DesktopDownload download;
        unsigned char *file_bytes = 0;
        const void *bytes = 0;
        size_t size = 0;
        char resolved[FVM_DESKTOP_MAX_LOCATION + 1];
        int variables = (request->flags & FVM_AVM1_URL_LOAD_VARIABLES) != 0;
        FVM_Result status = FVM_OK;
        navigation->head = request->next;
        if (!navigation->head)
            navigation->tail = 0;
        --navigation->count;
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
        else if (status == FVM_OK)
        {
            memset(&download, 0, sizeof(download));
            download.status = FVM_OK;
            memset(resolved, 0, sizeof(resolved));
            if (strstr(request->url, "://") || strstr(request->base, "://"))
            {
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
            if (status == FVM_OK)
            {
                if (variables)
                    status = fvm_object_load_variables(context, request->target, bytes, size);
                else
                    status = fvm_movieclip_replace_swf(context, request->target, bytes, size,
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
        if (status != FVM_OK)
            return status;
    }
    return FVM_OK;
}

/* Keep transport and successive VM lifetimes disjoint; this is root replacement, not child import.
 */
static int run_source(const FVM_DesktopOptions *options, const FVM_DesktopSource *source,
                      const FVM_DesktopLoader *loader, const char **error, int *closed)
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
            fetched = policy.fetch ? policy.fetch(reference, base, collect_movie, &download,
                                                  resolved, sizeof(resolved), policy.userdata)
                                   : FVM_UNSUPPORTED;
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
    return run_source(options, source, loader, error, 0);
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
        status = run_source(&value, &source, &loader, error, &closed);
        if (status || closed)
            break;
    }
    return status;
}

/* Parse before SDL/VM startup so help and invalid arguments work without a display server. */
int fvm_desktop_main(int argc, char **argv)
{
    FVM_DesktopOptions options;
    const char *error = 0;
    const char **files;
    int parsed, status = FVM_DESKTOP_EXIT_OK;
    if (!fvm_desktop_arguments_valid(argc, argv))
    {
        fprintf(stderr, "invalid player argument vector\n");
        return FVM_DESKTOP_EXIT_ARGUMENT;
    }
    files = (const char **)malloc((size_t)argc * sizeof(*files));
    if (!files)
        return FVM_DESKTOP_EXIT_PLAYBACK;
    parsed = fvm_desktop_parse(argc, argv, files, &options, &error);
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
        status = fvm_desktop_run(&options, &error);
        if (status == FVM_DESKTOP_EXIT_ARGUMENT)
            fprintf(stderr, "%s\n", error);
    }
    free(files);
    return status;
}
