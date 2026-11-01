/* Actual SDL FIFO -> native capture -> detached stamp raster. Dis admission is a
 * named boundary here; the companion real runner tests the production worker. */
#include "editor_view.h"
#include "desktop_view.h"
#include "editor_ink.h"
#include "editor_vector_layout.h"
#include "editor_ink_pointer.h"
#include "editor_wheel_batch.h"
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

/* Publish an empty real vector pane with copied ticket and authoring tool1..5.
 * Both are fixture-owned scalars. Abort on refusal; no document is mocked.
 * The pane is large enough to exercise a closed native trajectory. */
static void scene_tool(unsigned long ticket, unsigned long tool)
{
    char bytes[128];
    snprintf(bytes, sizeof(bytes), "%08lx000000010000000000000064", ticket);
    send(ticket, DVM_DESK_BEGIN, 960, 640, bytes);
    send(ticket, DVM_DESK_ITEM, 1, 0, "000000040000005400000030000003280000024400000001ffffffff00000000ffffffffVector");
    send(ticket, DVM_DESK_VECTOR_BEGIN, 1, 56, "0000000000000002");
    snprintf(bytes, sizeof(bytes), "%08lx00245ac400000008%08lx000000000000000000000000", tool, tool == 5 ? 1UL : 0UL);
    send(ticket, DVM_DESK_VECTOR_DATA, 1, 0, bytes);
    send(ticket, DVM_DESK_END, 0, 0, "");
}

/* Default Brush fixture preserves all existing trajectory tests. */
static void scene(unsigned long ticket)
{
    scene_tool(ticket, 1);
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

/* Exercise repeated toolbar clicks as ordinary controls, never maximize/draw.
 * ticket advances at the copied Dis-scene boundary; x is a canvas pixel, offset
 * is the expected negative control hit, and clicks is the SDL click count.
 * No document is mocked;
 * negative hit offsets and actual SDL click counts come from the native adapter.
 */
static void toolbar_click(unsigned long *ticket, int x, long offset, unsigned int clicks)
{
    SDL_Event click;
    DVM_UIMessage event;
    char expected[33];
    SDL_zero(click);
    click.type = SDL_MOUSEBUTTONDOWN;
    click.button.windowID = 1;
    click.button.which = 1;
    click.button.button = SDL_BUTTON_LEFT;
    click.button.clicks = (Uint8)clicks;
    {
        double bx;
        double by;
        require(dvm_vector_control_rect((unsigned int)(-offset - 2), 808, &bx, &by), "control layout");
        x = (int)bx + 84 + 14;
        click.button.x = x;
        click.button.y = (int)by + 48 + 12;
    }
    require(SDL_PushEvent(&click) == 1, "queue toolbar click");
    event = proposal();
    snprintf(expected, sizeof(expected), "%08x%08x%08x%08x", 1, 1, 1, (unsigned int)offset);
    require(event.code == DVM_DESK_POINTER && event.size == 32 && !memcmp(event.text, expected, 32),
            "repeated toolbar click stays a control, with no ink token or maximize code");
    scene(++*ticket);
    pointer(SDL_MOUSEBUTTONUP, x, click.button.y);
    event = proposal();
    require(event.code == DVM_DESK_POINTER && event.size == 32, "toolbar release creates no stroke");
    scene(++*ticket);
}

/* A missing release is repaired at the last admitted point. Foreign pointers
 * cannot append/end this capture, and hover coordinates must not become a tail.
 */
static void capture_recovery(unsigned long *ticket)
{
    DVM_UIMessage event;
    SDL_Event foreign;
    SDL_zero(foreign);
    foreign.type = SDL_MOUSEBUTTONDOWN;
    foreign.button.windowID = 1;
    foreign.button.which = 1;
    foreign.button.button = SDL_BUTTON_LEFT;
    foreign.button.clicks = 2;
    foreign.button.x = 160;
    foreign.button.y = 220;
    require(SDL_PushEvent(&foreign) == 1, "repeated canvas press");
    event = proposal();
    require(event.size == 40, "repeated canvas press begins ink, never maximize");
    scene(++*ticket);
    SDL_zero(foreign);
    foreign.type = SDL_MOUSEMOTION;
    foreign.motion.windowID = 1;
    foreign.motion.which = 2;
    foreign.motion.state = SDL_BUTTON_LMASK;
    foreign.motion.x = 700;
    foreign.motion.y = 500;
    require(SDL_PushEvent(&foreign) == 1, "foreign mouse motion");
    foreign.type = SDL_MOUSEBUTTONUP;
    foreign.button.windowID = 1;
    foreign.button.which = 2;
    foreign.button.button = SDL_BUTTON_LEFT;
    foreign.button.x = 700;
    foreign.button.y = 500;
    require(SDL_PushEvent(&foreign) == 1, "foreign mouse release");
    SDL_zero(foreign);
    foreign.type = SDL_MOUSEMOTION;
    foreign.motion.windowID = 1;
    foreign.motion.which = 1;
    foreign.motion.state = 0;
    foreign.motion.x = 800;
    foreign.motion.y = 550;
    require(SDL_PushEvent(&foreign) == 1, "own hover proves release was lost");
    event = proposal();
    require(event.code == DVM_DESK_POINTER && event.size == 40 && !memcmp(event.text, "00000003", 8),
            "only the owning lost-release fence ends the stroke");
    require(strlen(dvm_ink_read(2)) == 8, "foreign/hover positions never become ink or a connecting line");
    scene(++*ticket);
    toolbar_click(ticket, 510, -14, 2);
    toolbar_click(ticket, 710, -16, 2);
    toolbar_click(ticket, 810, -17, 2);
}

/* SDL's synthetic mouse copies must not fence an active capture. The real
 * release is queued last and must be consumed before any End is delivered.
 * Finger identity checks cover separate contacts on the same touch surface.
 */
static void synthetic_copies(unsigned long *ticket)
{
    SDL_Event event;
    SDL_Event begin;
    DVM_UIMessage out;
    pointer(SDL_MOUSEBUTTONDOWN, 160, 220);
    out = proposal();
    require(out.size == 40, "capture before synthetic copies");
    scene(++*ticket);
    SDL_zero(event);
    event.type = SDL_MOUSEBUTTONDOWN;
    event.button.windowID = 1;
    event.button.which = SDL_TOUCH_MOUSEID;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = 710;
    event.button.y = 108;
    require(SDL_PushEvent(&event) == 1, "synthetic mouse down");
    event.type = SDL_MOUSEBUTTONUP;
    require(SDL_PushEvent(&event) == 1, "synthetic mouse up");
    pointer(SDL_MOUSEBUTTONUP, 160, 220);
    out = proposal();
    require(out.size == 40 && !memcmp(out.text, "00000003", 8) && !SDL_HasEvent(SDL_MOUSEBUTTONUP),
            "synthetic copies cannot seal before the actual owner release");
    require(strlen(dvm_ink_read(3)) == 8, "synthetic toolbar coordinates never enter the trajectory");
    scene(++*ticket);
    SDL_zero(begin);
    begin.type = SDL_FINGERDOWN;
    begin.tfinger.windowID = 1;
    begin.tfinger.touchId = 17;
    begin.tfinger.fingerId = 31;
    event = begin;
    event.type = SDL_FINGERMOTION;
    require(dvm_editor_ink_pointer(&begin, &event) == 1, "same finger owns motion");
    event.tfinger.fingerId++;
    require(dvm_editor_ink_pointer(&begin, &event) == 0, "another finger cannot append a connecting segment");
    event = begin;
    event.type = SDL_FINGERUP;
    event.tfinger.windowID++;
    require(dvm_editor_ink_pointer(&begin, &event) == 0, "another window cannot release a finger capture");
}

/* A native wheel burst must not build an unbounded per-tick UI backlog.
 * ticket is the completed copied scene boundary. Preserve all128 packet steps
 * as eight bounded messages, keeping the direction reversal as a FIFO fence.
 */
static void wheel_bursts(unsigned long *ticket)
{
    SDL_Event wheel;
    DVM_UIMessage out;
    unsigned int index;
    SDL_zero(wheel);
    wheel.type = SDL_MOUSEWHEEL;
    wheel.wheel.windowID = 1;
    wheel.wheel.which = 1;
#if SDL_VERSION_ATLEAST(2, 26, 0)
    wheel.wheel.mouseX = 700;
    wheel.wheel.mouseY = 500;
#endif
    for (index = 0; index < 128; index++)
    {
        wheel.wheel.y = index < 64 ? -1 : 1;
        require(SDL_PushEvent(&wheel) == 1, "queue wheel burst and reversal");
    }
    for (index = 0; index < 8; index++)
    {
        char prefix[17];
        out = proposal();
        snprintf(prefix, sizeof(prefix), "%08x%08x", 32, index < 4 ? 5 : 4);
        require(out.code == DVM_DESK_POINTER && out.size == 32 && !memcmp(out.text, prefix, 16),
                "sixteen wheel packets become one bounded scroll message");
#if SDL_VERSION_ATLEAST(2, 26, 0)
        require(out.first == 700 && out.second == 500, "wheel uses its recorded pointer position");
#endif
        scene(++*ticket);
    }
    require(!SDL_HasEvent(SDL_MOUSEWHEEL), "wheel burst drained without deferred tick debt");
}

/* A partial burst must stop at reversal, key or another device/position.
 * The head is already removed; this fixture owns only the copied SDL queue.
 */
static void wheel_fences(void)
{
    SDL_Event head;
    SDL_Event next;
    unsigned int variant;
    SDL_zero(head);
    head.type = SDL_MOUSEWHEEL;
    head.wheel.windowID = 1;
    head.wheel.which = 1;
    head.wheel.y = 1;
    for (variant = 0; variant < 4; variant++)
    {
        SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
        next = head;
        if (variant == 0)
        {
            next.wheel.y = -1;
        }
        if (variant == 1)
        {
            next.wheel.which = 2;
        }
        if (variant == 2)
        {
            next.type = SDL_KEYDOWN;
        }
#if SDL_VERSION_ATLEAST(2, 26, 0)
        if (variant == 3)
        {
            next.wheel.mouseX = 900;
        }
#else
        if (variant == 3)
        {
            next.wheel.windowID = 2;
        }
#endif
        require(SDL_PushEvent(&next) == 1, "queue wheel fence");
        require(dvm_editor_wheel_batch(&head) == 1 && SDL_HasEvents(SDL_FIRSTEVENT, SDL_LASTEVENT),
                "partial wheel batch leaves the FIFO fence untouched");
    }
    SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
}

/* Geometric primitives need only endpoints. Drive the actual native adapter
 * with600 queued motions and preserve the release/next control FIFO fence.
 * ticket is borrowed monotonically updated scene identity, not document state. */
static void primitive_bursts(unsigned long *ticket)
{
    unsigned long tool;
    for (tool = 2; tool <= 5; tool++)
    {
        DVM_UIMessage event;
        unsigned int index;
        unsigned int updates = 0;
        scene_tool(++*ticket, tool);
        pointer(SDL_MOUSEBUTTONDOWN, 160, 220);
        event = proposal();
        require(event.size == 40 && !memcmp(event.text, "00000001", 8), "primitive begins native endpoint capture");
        scene_tool(++*ticket, tool);
        for (index = 0; index < 600; index++)
        {
            pointer(SDL_MOUSEMOTION, 160 + index % 300, 220 + index % 120);
        }
        pointer(SDL_MOUSEBUTTONUP, 459, 339);
        for (;;)
        {
            event = proposal();
            require(event.size == 40, "primitive emits only bounded frozen endpoint release");
            if (!memcmp(event.text, "00000003", 8))
            {
                break;
            }
            require(!memcmp(event.text, "00000002", 8) && ++updates <= 3,
                    "600 primitive motions collapse into at most three latest endpoints");
            scene_tool(++*ticket, tool);
        }
        require(updates == 0, "native geometric/Pen motions never wait on a Limbo proposal");
        {
            unsigned long token = 0;
            require(sscanf(event.text + 32, "%8lx", &token) == 1, "frozen token");
            require(tool == 5 ? strlen(dvm_ink_read(token)) > 16 : strlen(dvm_ink_read(token)) == 16,
                    "Fill-on Pen keeps its trajectory; primitives retain just two endpoints");
        }
        dvm_ink_drop(1);
        scene_tool(++*ticket, tool);
        require(!SDL_HasEvents(SDL_MOUSEMOTION, SDL_MOUSEMOTION), "primitive has no delayed motion tail after release");
    }
    scene(++*ticket);
}


/* Added contacts cancel provisional native paint before the frame/worker
 * handoff. Real SDL values must produce ContactCancel, never a frozen dot. */
static void multicontact_cancel(unsigned long *ticket)
{
    unsigned long tool;
    for (tool = 1; tool <= 5; ++tool)
    {
        SDL_Event event;
        DVM_UIMessage message;
        unsigned int attempt;
        scene_tool(++*ticket, tool);
        SDL_zero(event);
        event.type = SDL_FINGERDOWN;
        event.tfinger.windowID = 1;
        event.tfinger.touchId = 50;
        event.tfinger.fingerId = 1;
        event.tfinger.x = 160.0f / 960;
        event.tfinger.y = 220.0f / 640;
        require(SDL_PushEvent(&event) == 1, "first canvas contact");
        message = proposal();
        require(message.size == 40, "first finger owns provisional native paint");
        scene_tool(++*ticket, tool);
        event.tfinger.fingerId = 2;
        event.tfinger.x = 400.0f / 960;
        require(SDL_PushEvent(&event) == 1, "second canvas contact");
        message = proposal();
        require(message.size == 32 && !memcmp(message.text + 8, "0000000b", 8) &&
                !dvm_ink_has_preview(1), "added contact cancels paint without committing a frozen dot");
        scene_tool(++*ticket, tool);
        event.type = SDL_FINGERUP;
        require(SDL_PushEvent(&event) == 1, "second contact release");
        event.tfinger.fingerId = 1;
        require(SDL_PushEvent(&event) == 1, "first contact release");
        for (attempt = 0; attempt < 500 && SDL_HasEvents(SDL_FINGERDOWN, SDL_FINGERMOTION); ++attempt)
        {
            require(dvm_editor_event(&message) == 0, "lifting converted contacts creates no new tap");
            SDL_Delay(1);
        }
        require(!SDL_HasEvents(SDL_FINGERDOWN, SDL_FINGERMOTION), "all converted contacts retired");
    }
    scene(++*ticket);
}


/* First drive600 real queued motions with only Begin/frozen End proposals and
 * inspect detached pixels. Then check controls and recovered pointer ownership. */
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
    {
        unsigned long ticket = 9;
        scene(ticket);
        toolbar_click(&ticket, 510, -14, 1);
        toolbar_click(&ticket, 510, -14, 2);
        toolbar_click(&ticket, 710, -16, 2);
        toolbar_click(&ticket, 810, -17, 2);
        capture_recovery(&ticket);
        synthetic_copies(&ticket);
        wheel_bursts(&ticket);
        wheel_fences();
        primitive_bursts(&ticket);
        multicontact_cancel(&ticket);
    }
    dvm_editor_destroy();
    printf(
        "PASS native SDL ink: initial600 motions/2 proposals, repeated controls, pointer recovery, synthetic copies; frozen bytes=%lu, stamped pixels=%u\n",
        (unsigned long)(event.size - 32),
        ink);
    return 0;
}
