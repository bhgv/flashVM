/* Opt-in background provider execution with native event pumping at player safe points. */
#ifndef FVM_DESKTOP_BACKGROUND_H
#define FVM_DESKTOP_BACKGROUND_H
#include "source.h"
#include "transfer.h"
#define FVM_DESKTOP_BACKGROUND_WAIT_MS 5U /* Owner polling cadence, not a realtime guarantee. */
#ifdef __cplusplus
extern "C"
{
#endif
    /* Initialize before use; nested provider userdata remains immutable/borrowed until run returns.
     */
    typedef struct FVM_DesktopBackgroundLoader
    {
        FVM_DesktopTransferOptions
            transfer; /* Explicit worker-safe provider and native byte budget. */
        unsigned long
            max_loads; /* Startup plus movie/form requests, same bounds as ordinary loader. */
    } FVM_DesktopBackgroundLoader;
    /* Defaults: no provider, 64MiB per resource and one load; 1 success, 0 null, no startup. */
    FVM_DESKTOP_API int fvm_desktop_background_loader_init(FVM_DesktopBackgroundLoader *out);
    /* Opt-in source runner, with ordinary options/source/status rules. Native bytes are fetched
     * on joined workers. Owner thread pumps its active window/audio/input records but never
     * executes VM code during fetch. Close cancels the current job, joins, and returns OK.
     * Startup/root transitions have no active player window to pump. DNS/noncooperative
     * providers may delay the final join. Filesystem-only input still reads on the owner.
     * No concurrent/reentrant player runs; loader/strings/userdata remain borrowed until return. */
    FVM_DESKTOP_API int fvm_desktop_run_source_background(const FVM_DesktopOptions *options,
                                                          const FVM_DesktopSource *source,
                                                          const FVM_DesktopBackgroundLoader *loader,
                                                          const char **error);
    /* Playlist adapter selecting the compiled default worker provider. No provider means ordinary
     * local playback and unsupported URL input. The options struct layout remains unchanged. */
    FVM_DESKTOP_API int fvm_desktop_run_background(const FVM_DesktopOptions *options,
                                                   const char **error);
#ifdef __cplusplus
}
#endif
#endif
