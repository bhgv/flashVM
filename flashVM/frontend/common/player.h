/* Shared C89 player host boundary; structured configuration and optional CLI adapter. */
#ifndef FVM_DESKTOP_PLAYER_H
#define FVM_DESKTOP_PLAYER_H
#include "flashvm/flashvm.h"
#include "options.h"
#include "source.h"
#define FVM_DESKTOP_EXIT_OK 0       /* Completed playback or a successful informational query. */
#define FVM_DESKTOP_EXIT_PLAYBACK 1 /* File, VM, presentation or device failure. */
#define FVM_DESKTOP_EXIT_ARGUMENT 2 /* Invalid CLI or structured configuration. */
/* Borrowed build metadata; no VM, device, window or process locale is initialized. */
typedef struct FVM_DesktopBuildInfo
{
    const char *version;       /* Static development version from VERSION.txt at configure time. */
    unsigned long abi_version; /* Linked VM ABI, independent of the development version. */
    unsigned int pointer_bits; /* Width of this target's data pointer, not a host CPU detector. */
    int jit_available;      /* Linked backend availability, not per-body coverage/device proof. */
    int sdl_audio_compiled; /* Queue bridge compiled in; no audio endpoint is probed. */
} FVM_DesktopBuildInfo;
#ifdef __cplusplus
extern "C"
{
#endif
    /* Fill caller storage without allocating; reject a null output pointer. */
    FVM_DESKTOP_API FVM_Result fvm_desktop_build_info(FVM_DesktopBuildInfo *out);
    /* Synchronous structured playback on the native UI/VM owner thread, without argv.
     * Validate every field/path before any startup; shallow-copy scalar configuration.
     * Paths/array remain borrowed and immutable until return. Optional error receives
     * a static message; playback diagnostics/traces still use the existing streams.
     * Returns EXIT_OK, EXIT_PLAYBACK or EXIT_ARGUMENT. No concurrent/reentrant runs.
     * Null/malformed input causes no file, SDL or VM initialization. */
    FVM_DESKTOP_API int fvm_desktop_run(const FVM_DesktopOptions *options, const char **error);
    /* Play an exact borrowed binary buffer; no temporary file or strlen(data).
     * Nonempty name labels diagnostics/window; local relative loads default to the executable
     * directory. An absolute path/HTTP(S) name explicitly supplies source provenance.
     * bytes/name remain accessible and immutable until synchronous return; never freed.
     * Size must be 1..FVM_SWF_DEFAULT_LIMIT. Playlist fields are ignored; scalar
     * options and UI/thread/error/return rules match run. Malformed SWF returns
     * EXIT_PLAYBACK before window creation; malformed descriptors return EXIT_ARGUMENT. */
    FVM_DESKTOP_API int fvm_desktop_run_memory(const FVM_DesktopOptions *options, const void *bytes,
                                               size_t length, const char *name, const char **error);
    /* End this library's SDL session on its native owner thread after playback returns.
     * OK when no subsystem references remain; ERROR leaves active host/player SDL untouched.
     * Explicit only: may clear SDL hints/TLS, so call only when the application ends that session.
     * Also works when a shared host contains a private static SDL copy. Idempotent when unused. */
    FVM_DESKTOP_API FVM_Result fvm_desktop_shutdown(void);
    /* Return 0 on help/version/normal close, 1 on playback failure, 2 on invalid command line.
     * argc must be positive; argv must contain argc non-null pointers to NUL-terminated strings.
     * Invalid pointer entries/counts fail before SDL/file/VM initialization. The caller
     * owns accessible storage throughout the synchronous call; no argv[argc] is read.
     * Playback must run on the native UI/VM owner thread; concurrent/reentrant calls
     * are not supported. The metadata query above does not enter the VM or collector. */
    FVM_DESKTOP_API int fvm_desktop_main(int argc, char **argv);
#ifdef __cplusplus
}
#endif
#endif
