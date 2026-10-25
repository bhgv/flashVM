/* C89 borrowed RGBA observations and validated row copies for native player hosts. */
#ifndef FVM_DESKTOP_FRAME_H
#define FVM_DESKTOP_FRAME_H /* Guard additive frame declarations. */
#include "playback.h"

#define FVM_DESKTOP_FRAME_RGBA8 1U            /* Byte-addressed straight-alpha R,G,B,A; no native endian pixel word. */
#define FVM_DESKTOP_FRAME_PIXEL_BYTES 4U      /* Visible bytes in one packed RGBA8 pixel. */
#define FVM_DESKTOP_FRAME_MAX_DIMENSION 4096U /* Current player/raster stage extent, not a window size. */
#define FVM_DESKTOP_FRAME_TICK 1U             /* A successful host timeline advance produced this raster. */
#define FVM_DESKTOP_FRAME_REFRESH 2U          /* An initial/dirty redraw produced pixels without a host tick. */
#ifdef __cplusplus
extern "C"
{
#endif
/* Borrowed owner-thread raster; retain no frame/pixels/source pointer after callback return. */
typedef struct FVM_DesktopFrame
{
    unsigned long struct_size;         /* Exact size of this frame ABI. */
    unsigned int format;               /* FRAME_RGBA8; rows run top to bottom, columns left to right. */
    unsigned int reason;               /* FRAME_TICK or FRAME_REFRESH, independent of window resize swaps. */
    unsigned int width;                /* Logical raster width in stage pixels, 1..MAX_DIMENSION. */
    unsigned int height;               /* Logical raster height in stage pixels, 1..MAX_DIMENSION. */
    size_t stride;                     /* Source byte distance between row starts, at least width * PIXEL_BYTES. */
    size_t byte_count;                 /* Accessible source extent; visible rows are bounded before copying. */
    const unsigned char *pixels;       /* Read-only native bytes borrowed until this callback returns. */
    const char *source;                /* Borrowed current-root origin; copy text separately if retaining it. */
    unsigned long movie_ticks;         /* Current-root completed host advances, including this tick if any. */
    unsigned long total_ticks;         /* Completed host advances across whole-root navigation. */
    unsigned long presentations;       /* One-based current-root successful raster presentation count. */
    unsigned long total_presentations; /* One-based raster presentations across root navigation. */
} FVM_DesktopFrame;


/* Called after raster and native presentation succeed, on the owner, outside VM rendering.
     * OK continues; every other result fails playback after already committed drawing/VM effects.
     * Do not write borrowed pixels, retain pointers, enter/reenter VM/player, destroy SDK, steal
     * SDL events or throw across C. Copy pixels here before handing independent storage to a
     * worker. */
typedef FVM_Result (*FVM_DesktopFrameNotify)(const FVM_DesktopFrame *frame, void *userdata);


/* Borrowed callback policy, separate from unchanged playback/source/loader descriptor layouts.
     */
typedef struct FVM_DesktopFrameObserver
{
    unsigned long struct_size;     /* Must equal sizeof this descriptor. */
    FVM_DesktopFrameNotify notify; /* Required for an observed runner; no callbacks on invalid input. */
    void *userdata;                /* Borrowed native host state, never serialized or sent to fetch/audio
                           workers. */
} FVM_DesktopFrameObserver;


/* Fill exact ABI size and null callback/userdata; 1/0 for nonnull/null, no allocation/startup.
     */
FVM_DESKTOP_API int fvm_desktop_frame_observer_init(FVM_DesktopFrameObserver *out);


/* Copy visible RGBA rows into caller storage without allocation, VM or SDL startup.
     * Require exact frame size, RGBA8, valid bounded dimensions/strides and sufficient extents.
     * Destination must be accessible and not overlap source (a caller ownership precondition).
     * Destination padding is untouched. All geometry is checked before writing: INVALID_ARGUMENT
     * leaves destination bytes unchanged; OK copies width*4 bytes per row. Source padding and
     * source/metadata pointers are not copied. The frame/pixels need remain valid only for this
     * call. */
FVM_DESKTOP_API FVM_Result fvm_desktop_frame_copy(const FVM_DesktopFrame *frame,
                                                  void *destination,
                                                  size_t capacity,
                                                  size_t destination_stride);


/* Synchronous FILE/MEMORY/URL observation on the native UI owner, without console parsing.
     * options/source/observer required; loader/control optional (null control means running).
     * Validate descriptors before startup; borrow inputs/callbacks/userdata through return.
     * Return 0 on completion/close/STOP, 1 on VM/provider/presentation/callback failure, 2 on
     * malformed arguments. Optional error borrows static text. Native SDL presentation remains;
     * captures exclude window scaling/chrome and cached resize swaps. No reentrant/concurrent run.
     */
FVM_DESKTOP_API int fvm_desktop_run_source_observed(const FVM_DesktopOptions *,
                                                    const FVM_DesktopSource *,
                                                    const FVM_DesktopLoader *,
                                                    const FVM_DesktopPlaybackControl *,
                                                    const FVM_DesktopFrameObserver *,
                                                    const char **error);


/* Same owner observation with required explicit background provider and load monitor.
     * monitor.asynchronous selects continuing owner turns or blocking safe-point resource waits.
     * Callback errors discard/join pending jobs before releasing native buffers and managed roots.
     */
FVM_DESKTOP_API int fvm_desktop_run_source_background_observed(const FVM_DesktopOptions *,
                                                               const FVM_DesktopSource *,
                                                               const FVM_DesktopBackgroundLoader *,
                                                               const FVM_DesktopLoadMonitor *,
                                                               const FVM_DesktopPlaybackControl *,
                                                               const FVM_DesktopFrameObserver *,
                                                               const char **error);


/* Windowless counterparts: identical source/control/observer/error and lifetime contracts.
     * Own only timer scheduling, never initialize SDL video/events, create native windows,
     * poll host events or change cursor visibility. Frame presentations mean completed software
     * rasters. Audio remains explicit options.mute policy; no native input is synthesized.
     * Still synchronous, real-time paced, owner-thread-only and non-reentrant; SDL remains a
     * timing/thread/audio link dependency. A finite frame limit or STOP callback is recommended.
     * Not a GUI widget, frame-at-a-time API or Android lifecycle adapter. */
FVM_DESKTOP_API int fvm_desktop_run_source_offscreen(const FVM_DesktopOptions *,
                                                     const FVM_DesktopSource *,
                                                     const FVM_DesktopLoader *,
                                                     const FVM_DesktopPlaybackControl *,
                                                     const FVM_DesktopFrameObserver *,
                                                     const char **error);


/* Explicit background provider/monitor variant; async and blocking safe-point loading use
     * timer waits without consuming external events. Cancel/join precedes every runner return. */
FVM_DESKTOP_API int fvm_desktop_run_source_background_offscreen(const FVM_DesktopOptions *,
                                                                const FVM_DesktopSource *,
                                                                const FVM_DesktopBackgroundLoader *,
                                                                const FVM_DesktopLoadMonitor *,
                                                                const FVM_DesktopPlaybackControl *,
                                                                const FVM_DesktopFrameObserver *,
                                                                const char **error);
#ifdef __cplusplus
}
#endif
#endif
