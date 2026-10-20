/* SDL2 presentation boundary; VM and command parsing do not depend on SDL types. */
#ifndef FVM_DESKTOP_WINDOW_H
#define FVM_DESKTOP_WINDOW_H
#include "flashvm/host.h"
#include "flashvm/keyboard.h"
#include "flashvm/mouse.h"
#include "flashvm/input.h"
#define FVM_DESKTOP_PENDING_EVENTS                                                                 \
    128U /* Fixed main-thread FIFO capacity; overflow is an explicit error. */
#define FVM_DESKTOP_TOUCH_MOUSE_HINT                                                               \
    "SDL_TOUCH_MOUSE_EVENTS" /* Disable generated mouse duplicates. */
#define FVM_DESKTOP_MOUSE_TOUCH_HINT                                                               \
    "SDL_MOUSE_TOUCH_EVENTS"            /* Disable generated finger duplicates. */
#define FVM_DESKTOP_EVENT_KEY 1U        /* Shared FIFO keyboard variant. */
#define FVM_DESKTOP_EVENT_MOUSE 2U      /* Shared FIFO primary-pointer variant. */
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
    4.0                          /* Reset an excessively late deadline without skipping VM ticks. */
#define FVM_DESKTOP_KEY_LEFT 37  /* Flash-style code for the left arrow. */
#define FVM_DESKTOP_KEY_UP 38    /* Flash-style code for the up arrow. */
#define FVM_DESKTOP_KEY_RIGHT 39 /* Flash-style code for the right arrow. */
#define FVM_DESKTOP_KEY_DOWN 40  /* Flash-style code for the down arrow. */
#define FVM_DESKTOP_KEY_ASCII_LIMIT 128 /* Non-ASCII/non-arrow keys report code zero. */
#define FVM_DESKTOP_BUTTON_LEFT 1U      /* HostInput buttons bit for the primary pointer button. */
#define FVM_DESKTOP_BUTTON_MIDDLE 2U    /* HostInput buttons bit for the middle pointer button. */
#define FVM_DESKTOP_BUTTON_RIGHT 4U /* HostInput buttons bit for the secondary pointer button. */
/* Copied main-thread input state, with counters preserving taps between movie ticks. */
typedef struct FVM_DesktopInput
{
    FVM_KeyboardState keyboard; /* Full held-key/toggle snapshot for standard Key polling. */
    double x, y; /* Logical stage pixels, before authored stage-origin restoration. */
    int left, right, up, down, space; /* Held directional/space keys, cleared on focus loss. */
    int key_code, key_down; /* Latest key transition: ASCII uppercase/digits or arrow 37..40. */
    unsigned int buttons;   /* Held pointer-button bits; no global OS mouse capture. */
    unsigned long clicks,
        key_presses; /* Counts of physical down transitions, excluding key repeats. */
} FVM_DesktopInput;
typedef struct FVM_DesktopWindow FVM_DesktopWindow;
/* Initialize SDL/video on the calling main thread and own window/renderer/texture. */
FVM_DesktopWindow *fvm_desktop_window_open(const char *, unsigned int, unsigned int);
/* Upload/present and retain one owned packed RGBA snapshot; source bytes are borrowed
 * only during this call. Closing the window releases the native copy. */
int fvm_desktop_window_present(FVM_DesktopWindow *, const FVM_RasterSurface *, size_t byte_count);
/* Drain pending events, then wait at most timeout_ms; return zero on close/Escape. */
int fvm_desktop_window_events(FVM_DesktopWindow *, int timeout_ms);
/* Copy current input state without retaining caller storage or entering the VM. */
void fvm_desktop_window_input(FVM_DesktopWindow *, FVM_DesktopInput *);
/* Deliver queued native snapshots in order on the VM owner thread, before its next tick.
 * Focus loss cancels pending transitions. Errors consume the current event and retain later ones.
 */
/* Supply a borrowed, pinned scene root for retained records then eligible named input methods.
 * The root must belong to context and outlive this synchronous main-thread pump.
 * Keyboard observers precede scene methods without an extra implicit focused-clip call. */
FVM_Result fvm_desktop_window_pump_clip_input(FVM_DesktopWindow *, FVM_Context *, FVM_Object *,
                                              double origin_x, double origin_y);
FVM_Result fvm_desktop_window_pump_input(FVM_DesktopWindow *, FVM_Context *, double origin_x,
                                         double origin_y);
/* Compatibility internal entrypoint: deliver the same shared FIFO with zero stage origin. */
FVM_Result fvm_desktop_window_pump_keyboard(FVM_DesktopWindow *, FVM_Context *);
/* Read whether the Android application is foreground; ordinary desktop focus loss only clears held
 * input. */
int fvm_desktop_window_active(FVM_DesktopWindow *);
/* Read monotonic elapsed milliseconds from SDL's performance counter. */
double fvm_desktop_now(void);
/* Report the latest SDL failure while the backend remains initialized. */
const char *fvm_desktop_window_error(void);
/* Release texture, renderer, window and SDL; null is accepted. */
void fvm_desktop_window_close(FVM_DesktopWindow *);
#endif
