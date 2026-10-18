/* SDL2 presentation boundary; VM and command parsing do not depend on SDL types. */
#ifndef FVM_DESKTOP_WINDOW_H
#define FVM_DESKTOP_WINDOW_H
#include "flashvm/raster.h"
#define FVM_DESKTOP_MAX_PIXELS 4096U    /* Match the current rasterizer coordinate extent. */
#define FVM_DESKTOP_RED 0U              /* Red offset in a packed RGBA pixel. */
#define FVM_DESKTOP_GREEN 1U            /* Green offset in a packed RGBA pixel. */
#define FVM_DESKTOP_BLUE 2U             /* Blue offset in a packed RGBA pixel. */
#define FVM_DESKTOP_ALPHA 3U            /* Alpha offset in a packed RGBA pixel. */
#define FVM_DESKTOP_WHITE 255U          /* Default white stage color channel. */
#define FVM_DESKTOP_OPAQUE 255U         /* Default fully opaque stage alpha. */
#define FVM_DESKTOP_RGBA_BYTES 4U       /* Packed straight-alpha bytes per pixel. */
#define FVM_DESKTOP_MILLISECONDS 1000.0 /* Convert seconds to scheduler milliseconds. */
#define FVM_DESKTOP_LATE_FRAMES                                                                    \
    4.0 /* Reset an excessively late deadline without skipping VM ticks. */
typedef struct FVM_DesktopWindow FVM_DesktopWindow;
/* Initialize SDL/video on the calling main thread and own window/renderer/texture. */
FVM_DesktopWindow *fvm_desktop_window_open(const char *, unsigned int, unsigned int);
/* Upload an RGBA frame and present it; source bytes are borrowed only during this call. */
int fvm_desktop_window_present(FVM_DesktopWindow *, const FVM_RasterSurface *);
/* Drain pending events, then wait at most timeout_ms; return zero on close/Escape. */
int fvm_desktop_window_events(FVM_DesktopWindow *, int timeout_ms);
/* Read monotonic elapsed milliseconds from SDL's performance counter. */
double fvm_desktop_now(void);
/* Report the latest SDL failure while the backend remains initialized. */
const char *fvm_desktop_window_error(void);
/* Release texture, renderer, window and SDL; null is accepted. */
void fvm_desktop_window_close(FVM_DesktopWindow *);
#endif
