/* Actual SDL FIFO -> native capture -> detached stamp raster. Dis admission is a
 * named boundary here; the companion real runner tests the production worker. */
#include "editor_view.h"
#include "desktop_view.h"
#include "editor_ink.h"
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Preserve active invariants in Release and print a useful ownership-stage label. */
static void require(int ok, const char *label)
{
    if (!ok)
    {
        fprintf(stderr, "FAIL live native ink: %s\n", label);
        exit(1);
    }
}


/* Send an admitted copied scene value to the real native editor adapter. */
static void send(unsigned long ticket, unsigned long code, long first, long second, const char *bytes)
{
    DVM_UIMessage out;
    memset(&out, 0, sizeof(out));
    out.kind = DVM_UI_VIEW;
    out.code = code;
    out.flags = ticket;
    out.first = first;
    out.second = second;
    out.size = strlen(bytes);
    memcpy(out.text, bytes, out.size);
    require(dvm_editor_present(&out), "actual copied scene admission");
}


/* An empty real vector pane, large enough for a closed native trajectory. The
 * scene is the acknowledged layout boundary, not a mock hit/paint implementation. */
static void scene(unsigned long ticket)
{
    char bytes[128];
    snprintf(bytes, sizeof(bytes), "%08lx000000010000000000000064", ticket);
    send(ticket, DVM_DESK_BEGIN, 960, 640, bytes);
    send(ticket, DVM_DESK_ITEM, 1, 0, "000000040000005400000030000003280000024400000001ffffffff00000000ffffffffVector");
    send(ticket, DVM_DESK_VECTOR_BEGIN, 1, 56, "0000000000000002");
    send(ticket, DVM_DESK_VECTOR_DATA, 1, 0, "0000000100245ac40000000800000000000000000000000000000000");
    send(ticket, DVM_DESK_END, 0, 0, "");
}


/* Inject copied SDL values into the real FIFO. No Limbo/UI message is synthesized. */
static void pointer(Uint32 type, int x, int y)
{
    SDL_Event event;
    SDL_zero(event);
    event.type = type;
    if (type == SDL_MOUSEMOTION)
    {
        event.motion.windowID = 1;
        event.motion.which = 1;
        event.motion.state = SDL_BUTTON_LMASK;
        event.motion.x = x;
        event.motion.y = y;
    }
    else
    {
        event.button.windowID = 1;
        event.button.which = 1;
        event.button.button = SDL_BUTTON_LEFT;
        event.button.x = x;
        event.button.y = y;
    }
    require(SDL_PushEvent(&event) == 1, "queue native pointer");
}


/* Poll the actual owner/presentation gate until a channel proposal is produced.
 * Native batch-only progress returns zero and must not be counted as a Dis event. */
static DVM_UIMessage proposal(void)
{
    DVM_UIMessage event;
    unsigned int attempt;
    for (attempt = 0; attempt < 10000; ++attempt)
    {
        int status = dvm_editor_event(&event);
        require(status >= 0, "native event/raster poll");
        if (status)
        {
            return event;
        }
        SDL_Delay(1);
    }
    require(0, "bounded native proposal wait");
    memset(&event, 0, sizeof(event));
    return event;
}


/* Drive600 real queued motions through the production C owner. The only emitted
 * proposals are Begin and a frozen-token End. Inspect actual detached pixels. */
int dvm_test_live_ink(const char *font)
{
    DVM_UIMessage event;
    unsigned int index;
    unsigned int ink = 0;
    DVM_EditorFrame *frame;
    const FVM_RasterSurface *surface;
    require(dvm_editor_option("--font", font) && dvm_editor_create(), "real native editor initialization");
    SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
    scene(7);
    pointer(SDL_MOUSEBUTTONDOWN, 160, 220);
    event = proposal();
    require(event.code == DVM_DESK_POINTER && event.size == 40 && !memcmp(event.text, "00000001", 8),
            "one initial pointer proposal");
    scene(8); /* The sole initial Dis-layout acknowledgement. */
    for (index = 0; index < 600; ++index)
    {
        int x = index < 200 ? 160 + (int)index : index < 400 ? 360 : 360 - (int)(index - 400);
        int y = index < 200 ? 220 : index < 400 ? 220 + (int)(index - 200) : 420;
        pointer(SDL_MOUSEMOTION, x, y);
    }
    {
        SDL_Event expose;
        SDL_zero(expose);
        expose.type = SDL_WINDOWEVENT;
        expose.window.windowID = 1;
        expose.window.event = SDL_WINDOWEVENT_EXPOSED;
        require(SDL_PushEvent(&expose) == 1, "queue expose without ending capture");
    }
    require(!dvm_ink_read(1)[0], "live trajectory cannot be copied to Dis");
    pointer(SDL_MOUSEBUTTONUP, 160, 420);
    event = proposal();
    require(event.code == DVM_DESK_POINTER && event.size == 40 && !memcmp(event.text, "00000003", 8),
            "motions emitted no Dis proposals, release carries a token");
    require(strlen(dvm_ink_read(1)) >= 32 && strlen(dvm_ink_read(1)) < 600 * 8,
            "native batch kept turns and compacted close collinear samples");
    frame = dvm_editor_frame_create(960, 640);
    require(frame && dvm_desktop_prepare_cached(frame, NULL) && dvm_editor_frame_incremental(frame, NULL) &&
                dvm_editor_frame_render(frame) == FVM_OK,
            "actual detached native preview raster");
    surface = dvm_editor_frame_surface(frame);
    for (index = 0; index < surface->width * surface->height; ++index)
    {
        const unsigned char *pixel = surface->pixels + (size_t)index * 4;
        if (index % surface->width >= 130 && index % surface->width < 390 && index / surface->width >= 200 &&
            index / surface->width < 445)
        {
            ink += pixel[2] > pixel[0] + 60 && pixel[2] > pixel[1] + 30;
        }
    }
    for (index = 0; index < 3; ++index)
    {
        unsigned int x[] = {200, 360, 220};
        unsigned int y[] = {220, 300, 420};
        const unsigned char *pixel = surface->pixels + y[index] * surface->stride + x[index] * 4;
        require(pixel[2] > pixel[0] + 60 && pixel[2] > pixel[1] + 30, "actual retained turn/return pixels");
    }
    require(ink > 1000, "buffered trajectory is visibly stamped across its actual pane");
    dvm_editor_frame_destroy(frame);
    dvm_editor_destroy();
    printf("PASS native SDL ink:600 motions,2 Dis proposals, frozen bytes=%lu, stamped pixels=%u\n",
           (unsigned long)(event.size - 32),
           ink);
    return 0;
}
