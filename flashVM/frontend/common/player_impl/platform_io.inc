/* Copyright (c) 2026 bhgv
 * EN: Shared player platform_io implementation; included in owner-thread dependency order.
 * RU: Shared player platform_io; подключается в порядке owner-thread зависимостей.
 */
/* Host-owned desktop playback: load, root, advance, rasterize, present, release. */
#if defined(__APPLE__) && !defined(_DARWIN_C_SOURCE)
#define _DARWIN_C_SOURCE 1 /* Expose native executable-path/realpath declarations in C89. */
#endif
#if defined(__ANDROID__) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L /* Expose readlink without depending on process CWD. */
#endif
#define SDL_MAIN_HANDLED
#include "../player.h"
#include "../managed_source.h"
#include "../playback_internal.h"
#include "../frame_internal.h"
#include "desktop_build_version.h"
#include "../audio.h"
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
#include "../options.h"
#include "../request_constants.h"
#include "../window.h"

#ifdef FVM_DESKTOP_HTTP
#include "../http.h"

#endif
#ifdef FVM_DESKTOP_SOCKET_HTTP
#include "../socket_http.h"

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
    {
        return 0;
    }
    slash = strrchr(canonical, '/');
    if (!slash)
    {
        return 0;
    }
    slash[1] = 0;
    return SDL_strdup(canonical);
#elif defined(__ANDROID__)
    char path[FVM_DESKTOP_MAX_LOCATION + 1];
    ssize_t size = readlink("/proc/self/exe", path, FVM_DESKTOP_MAX_LOCATION);
    char *slash;
    if (size <= 0 || (size_t)size >= FVM_DESKTOP_MAX_LOCATION)
    {
        return 0;
    }
    path[(size_t)size] = 0;
    slash = strrchr(path, '/');
    if (!slash)
    {
        return 0;
    }
    slash[1] = 0;
    return SDL_strdup(path);
#else
    return SDL_GetBasePath();
#endif
}

#define FVM_DESKTOP_EXECUTABLE_BASE executable_base_path /* Isolate OS discovery in protocol tests. */
#endif


/* Query linked/build metadata without creating runtime state or probing native devices. */
FVM_Result fvm_desktop_build_info(FVM_DesktopBuildInfo *out)
{
    if (!out)
    {
        return FVM_INVALID_ARGUMENT;
    }
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
    if (fvm_desktop_transfer_active() || fvm_desktop_playback_mailbox_active() || SDL_WasInit(0))
    {
        return FVM_ERROR;
    }
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
    printf("abi=%lu pointer_bits=%u jit_backend=%s sdl_audio_compiled=%s\n",
           info.abi_version,
           info.pointer_bits,
           info.jit_available ? "available" : "unavailable",
           info.sdl_audio_compiled ? "yes" : "no");
    printf("sdl_headers=%u.%u.%u sdl_linked=%u.%u.%u\n",
           (unsigned int)SDL_MAJOR_VERSION,
           (unsigned int)SDL_MINOR_VERSION,
           (unsigned int)SDL_PATCHLEVEL,
           (unsigned int)linked.major,
           (unsigned int)linked.minor,
           (unsigned int)linked.patch);
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
    unsigned char extra; /* Probe measured EOF so a growing file is not loaded as a silent prefix. */
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
    {
        return 1;
    }
    fprintf(stderr, "%s: %s failed (VM status %d)\n", path, operation, (int)status);
    return 0;
}
