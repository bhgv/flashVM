/* Protocol-only presenter: actual VM/player/raster code, deterministic clock, no native window. */
#include "../common/window.h"
#include "../../ports/monotonic_port.h"
#include <stdio.h>
#include <string.h>
struct FVM_DesktopWindow { unsigned int width, height; };
static struct FVM_DesktopWindow view;
static double milliseconds;
/* Override the port clock so event waits and VM deadlines advance together without wall-clock sleeps. */
int fvm_port_monotonic_ms(double *out) { if (!out) return 0; *out = milliseconds; return 1; }
/* Return a stable owned test window descriptor; the real host SDL window path is tested separately. */
FVM_DesktopWindow *fvm_desktop_window_open(const char *name, unsigned int width, unsigned int height)
{ (void)name; view.width=width; view.height=height; return &view; }
/* Inspect actual rasterized RGBA bytes at an interior shape pixel, not merely a redraw flag. */
int fvm_desktop_window_present(FVM_DesktopWindow *window,const FVM_RasterSurface *surface,size_t count)
{
    const unsigned char *p;
    if(window!=&view || !surface || surface->width<=8 || surface->height<=8 || count<surface->stride*surface->height)return 0;
    p=surface->pixels+8*surface->stride+8*FVM_DESKTOP_RGBA_BYTES;
    fprintf(stderr,"CAPTURE ms=%.0f rgba=%u,%u,%u,%u\n",milliseconds,(unsigned)p[0],(unsigned)p[1],(unsigned)p[2],(unsigned)p[3]);
    return 1;
}
/* Advance only for an actual blocking wait; a zero-time event drain leaves deadlines unchanged. */
int fvm_desktop_window_events(FVM_DesktopWindow *window,int timeout)
{ if(window!=&view || timeout<0)return 0;if(timeout>0)milliseconds+=timeout;return 1; }
/* Publish a valid all-up input snapshot without generating any input events. */
void fvm_desktop_window_input(FVM_DesktopWindow *window,FVM_DesktopInput *input)
{ (void)window;memset(input,0,sizeof(*input));input->keyboard.struct_size=sizeof(input->keyboard); }
/* There are no synthetic listeners or OS messages in this protocol fixture. */
FVM_Result fvm_desktop_window_pump_input(FVM_DesktopWindow *window,FVM_Context *c,double x,double y)
{ (void)c;(void)x;(void)y;return window==&view?FVM_OK:FVM_INVALID_ARGUMENT; }
/* This empty event source has no input records to broadcast; the real SDL adapter is tested separately. */
FVM_Result fvm_desktop_window_pump_clip_input(FVM_DesktopWindow *window,FVM_Context *c,FVM_Object *root,double x,double y)
{ (void)root;return fvm_desktop_window_pump_input(window,c,x,y); }
/* Keep the compatibility keyboard entry routed to the same empty event source. */
FVM_Result fvm_desktop_window_pump_keyboard(FVM_DesktopWindow *window,FVM_Context *c)
{ return fvm_desktop_window_pump_input(window,c,0,0); }
/* The fixture never enters background suspension. */
int fvm_desktop_window_active(FVM_DesktopWindow *window) { return window==&view; }
/* Share the deterministic clock with authored frame scheduling. */
double fvm_desktop_now(void) { return milliseconds; }
/* Return a static diagnostic without allocating or hiding a protocol failure. */
const char *fvm_desktop_window_error(void) { return "capture presenter protocol failure"; }
/* Release no OS resource; each test runs in a separate process with a fresh static clock. */
void fvm_desktop_window_close(FVM_DesktopWindow *window) { (void)window; }
/* Protocol presenter has no SDL storage failures; keep the host status contract explicit. */
FVM_Result fvm_desktop_window_status(FVM_DesktopWindow *window)
{ return window == &view ? FVM_OK : FVM_INVALID_ARGUMENT; }

/* This protocol host has no native cursor; still validate the owning VM query. */
FVM_Result fvm_desktop_window_sync_cursor(FVM_DesktopWindow *window, FVM_Context *context)
{ int visible; return window == &view ? fvm_input_get_cursor_visible(context, &visible) : FVM_INVALID_ARGUMENT; }
