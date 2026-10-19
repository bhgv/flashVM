/* SDL2 frontend tutorial: real frame upload and first-contact input, without private player APIs. */
#define SDL_MAIN_HANDLED /* This desktop tutorial supplies its own portable main entrypoint. */
#include "../common/wheel.h" /* Share the platform-independent VM/frame owner. */
#include <SDL.h> /* Explicit external SDK; no SDL types enter FlashVM public headers. */
#include <stdio.h> /* Report backend/VM failures and bounded smoke completion. */
/* SDL resources belong to the application, not the VM. */
typedef struct SDLHost /* Keep native ownership separate from Demo. */
{ /* Initialize all fields before partial setup can fail. */
    SDL_Window *window; /* Owned resizable native window. */
    SDL_Renderer *renderer; /* Owned renderer with fixed logical coordinates. */
    SDL_Texture *texture; /* Owned streaming RGBA texture, recreated only with the host. */
    SDL_TouchID device; /* Numeric device identity of the first active finger. */
    SDL_FingerID finger; /* Numeric finger identity of that contact. */
    int touching; /* Additional fingers are ignored until the selected finger ends. */
    int cursor_applied; /* Last request applied by this standalone one-window SDL owner, or -1. */
    int cursor_original; /* Original SDL-global visibility restored before SDL_Quit, or -1 if unacquired. */
} SDLHost; /* Texture/renderer/window are released in that order. */
/* Apply the VM request to SDL's global cursor under this standalone single-window owner. */
static FVM_Result sdl_cursor(SDLHost *host, Demo *demo) /* Call only on the SDL/VM owner thread. */
{ /* Do not infer visibility from pointer/contact events. */
    int visible; /* Requested 0/1 from the context, not a device query. */
    if (demo->status != FVM_OK) return demo->status; /* Preserve earlier native/VM failure. */
    demo->status = fvm_input_get_cursor_visible(demo->context, &visible); /* Allocation-free query. */
    if (demo->status == FVM_OK && visible != host->cursor_applied) { /* Avoid redundant SDK calls. */
        if (SDL_ShowCursor(visible ? SDL_ENABLE : SDL_DISABLE) < 0) demo->status = FVM_ERROR; /* Real SDK status. */
        else host->cursor_applied = visible; /* Cache only an accepted native request. */
    } /* SDL acceptance is not physical screen acknowledgement. */
    return demo->status; /* Main stops drawing after any query/SDK error. */
} /* Multiple SDL windows need an explicit global cursor owner policy beyond this tutorial. */
/* Copy borrowed RGBA bytes into the SDL-owned texture and present synchronously. */
static FVM_Result sdl_present(const FVM_RasterSurface *frame, void *userdata) /* Borrow frame. */
{ /* All SDL rendering remains on the window/VM owner thread. */
    SDLHost *host = (SDLHost *)userdata; /* Main keeps native owners alive through this call. */
    if (SDL_UpdateTexture(host->texture, 0, frame->pixels, (int)frame->stride) != 0) /* Upload whole image. */
        return FVM_ERROR; /* Preserve the SDL diagnostic for main. */
    if (SDL_RenderClear(host->renderer) != 0 || /* Clear letterbox bars. */
        SDL_RenderCopy(host->renderer, host->texture, 0, 0) != 0) /* Scale the logical frame once. */
        return FVM_ERROR; /* Do not claim presentation succeeded. */
    SDL_RenderPresent(host->renderer); /* SDL owns the uploaded copy after return. */
    return FVM_OK; /* The host may overwrite source pixels for the next frame. */
} /* No borrowed VM/frame pointer is retained. */
/* Handle a copied SDL event immediately on the VM owner; return zero on close. */
static int sdl_event(SDLHost *host, Demo *demo, const SDL_Event *event) /* Borrow event only here. */
{ /* SDL's logical-size filter already transforms both mouse and normalized touch coordinates. */
    FVM_Affine identity = {1, 0, 0, 1, 0, 0}; /* Event logical pixels equal this demo's world pixels. */
    FVM_MouseEventType type; /* Set only for a recognized primary transition. */
    if (event->type == SDL_QUIT) /* Native close request ends the owning loop. */
        return 0; /* Caller stops all delivery before cleanup. */
    if (event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_FOCUS_LOST) /* Cancel hold. */
    { /* Focus loss must not leave a stuck pointer marker. */
        demo_cancel(demo); /* Explicit synthetic-up tutorial policy at last valid position. */
        host->touching = 0; /* Clear native contact ownership too. */
    } /* No promotion of a previously ignored finger occurs. */
    if (event->type == SDL_FINGERDOWN || event->type == SDL_FINGERMOTION || /* Native touch stream. */
        event->type == SDL_FINGERUP) /* End only the same selected contact. */
    { /* Ignore physical/synthetic duplication before entering VM. */
#ifdef SDL_MOUSE_TOUCHID /* SDL 2.0.9 headers may not expose the later synthetic device macro. */
        if (event->tfinger.touchId == SDL_MOUSE_TOUCHID) /* Mouse-generated touch is not a second contact. */
            return 1; /* Physical mouse handling below supplies that event. */
#endif /* Real touch support remains available with older SDL2 headers. */
        if (event->type == SDL_FINGERDOWN && !host->touching && !demo->held) /* First contact wins. */
        { /* Keep exact IDs, never just a finger count. */
            host->touching = 1; /* Own contact until its up or focus reset. */
            host->device = event->tfinger.touchId; /* Copy numeric device identity. */
            host->finger = event->tfinger.fingerId; /* Copy numeric finger identity. */
        } /* A held physical mouse prevents touch takeover. */
        if (!host->touching || host->device != event->tfinger.touchId || /* Ignore other devices. */
            host->finger != event->tfinger.fingerId) /* Ignore secondary fingers, including their up. */
            return 1; /* They cannot cancel or move the primary contact. */
        type = event->type == SDL_FINGERDOWN ? FVM_MOUSE_EVENT_DOWN : /* Preserve actual transition. */
               event->type == SDL_FINGERUP ? FVM_MOUSE_EVENT_UP : FVM_MOUSE_EVENT_MOVE; /* Move otherwise. */
        demo_pointer(demo, type, &identity, event->tfinger.x * DEMO_WIDTH, /* Logical normalized X once. */
                     event->tfinger.y * DEMO_HEIGHT); /* SDL already removed window letterbox scaling. */
        if (event->type == SDL_FINGERUP) /* End contact even when VM dispatch failed. */
            host->touching = 0; /* Never promote an ignored secondary contact. */
        return 1; /* The caller checks the recorded VM result. */
    } /* End native finger handling. */
    if (host->touching) /* A physical mouse cannot steal an active finger contact. */
        return 1; /* The selected touch stream alone drives the pointer. */
    if (event->type == SDL_MOUSEWHEEL && event->wheel.which != SDL_TOUCH_MOUSEID) /* Real vertical wheel only. */
    { /* The baseline SDL2 protocol has no event position: use the last accepted logical VM pointer. */
        double x, y, delta = event->wheel.y; /* Legacy integer detents on SDL2 before 2.0.18. */
        if (demo->status != FVM_OK) return 1; /* Preserve an already latched failure before querying position. */
#if SDL_VERSION_ATLEAST(2, 0, 18) /* Newer SDKs expose fractional vertical wheel units. */
        delta = event->wheel.preciseY; /* Keep native fractional resolution instead of truncating. */
#endif /* Older supported SDK headers never reference unavailable fields. */
        if (event->wheel.direction == SDL_MOUSEWHEEL_FLIPPED) delta = -delta; /* Normalize direction once. */
        if (delta != 0) { /* Horizontal-only/stop events do not create a synthetic vertical wheel. */
            demo->status = fvm_input_get_pointer(demo->context, &x, &y); /* Explicit last-known-position policy. */
            if (demo->status == FVM_OK) /* Do not use uninitialized position on query failure. */
                demo_wheel(demo, &identity, x, y, delta * DEMO_WHEEL_LINES_PER_DETENT); /* Detents to lines. */
        } /* No physical mouse poll or duplicate logical-size transform is applied. */
        return 1; /* Native owner loop checks the actual VM status. */
    } /* Active touch suppression above also prevents wheel takeover of its primary stream. */
    if (event->type == SDL_MOUSEMOTION && event->motion.which != SDL_TOUCH_MOUSEID) /* Ignore generated mouse. */
        demo_pointer(demo, FVM_MOUSE_EVENT_MOVE, &identity, event->motion.x, event->motion.y); /* Logical pixels. */
    if ((event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_MOUSEBUTTONUP) && /* Primary button only. */
        event->button.which != SDL_TOUCH_MOUSEID && event->button.button == SDL_BUTTON_LEFT) /* No duplicate touch. */
        demo_pointer(demo, event->type == SDL_MOUSEBUTTONDOWN ? FVM_MOUSE_EVENT_DOWN : /* Native down. */
                     FVM_MOUSE_EVENT_UP, &identity, event->button.x, event->button.y); /* Native up. */
    return 1; /* Keep running until close or a recorded error. */
} /* No keyboard or IME semantics are implied by this pointer-focused tutorial. */
/* Run with no arguments interactively, or --smoke for a bounded frame loop. */
int main(int argc, char **argv) /* Desktop launcher; Android supplies SDLActivity separately. */
{ /* Keep all native/VM state on one thread. */
    SDLHost host; /* Zero before partial native acquisition. */
    Demo demo; /* Shared portable VM/frame owner. */
    SDL_Event event; /* Reused copied native event storage. */
    int running = 1, smoke = 0, hide_cursor = 0, argument, previous; /* Explicit bounded mode. */
    int result = 1; /* Fail unless the entire requested run succeeds. */
    memset(&host, 0, sizeof(host)); /* Partial cleanup is safe. */
    host.cursor_applied = host.cursor_original = -1; /* Zero is a valid acquired hidden state. */
    memset(&demo, 0, sizeof(demo)); /* Even an SDL failure leaves VM cleanup safe. */
    for (argument = 1; argument < argc; ++argument) { /* Explicit finite and cursor-request options. */
        if (!strcmp(argv[argument], "--smoke")) smoke = 1; /* Bounded frame loop. */
        else if (!strcmp(argv[argument], "--hide-cursor")) hide_cursor = 1; /* Context request, not synthetic input. */
        else { fprintf(stderr, "usage: %s [--smoke] [--hide-cursor]\n", argv[0]); return 1; } /* No owners yet. */
    } /* Flags can be combined for the finite hidden-cursor SDK smoke run. */
    SDL_SetMainReady(); /* Pair with SDL_MAIN_HANDLED for explicit main ownership. */
    SDL_SetHint("SDL_TOUCH_MOUSE_EVENTS", "0"); /* Real touch events are handled directly once. */
    SDL_SetHint("SDL_MOUSE_TOUCH_EVENTS", "0"); /* Physical mouse does not create fake fingers. */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) /* Acquire only the required native subsystems. */
        goto cleanup; /* Report SDL's diagnostic below. */
    host.cursor_original = SDL_ShowCursor(SDL_QUERY); /* Save the native global state before applying a request. */
    if (host.cursor_original < 0) goto cleanup; /* Cannot safely restore an unknown SDK state. */
    host.window = SDL_CreateWindow("FlashVM SDL tutorial", SDL_WINDOWPOS_UNDEFINED, /* Native title/position. */
                                   SDL_WINDOWPOS_UNDEFINED, DEMO_WIDTH, DEMO_HEIGHT, SDL_WINDOW_RESIZABLE); /* Size. */
    if (host.window) /* Acquire renderer only for a valid window. */
        host.renderer = SDL_CreateRenderer(host.window, -1, 0); /* Permit the SDK's software/GPU choice. */
    if (!host.renderer || SDL_RenderSetLogicalSize(host.renderer, DEMO_WIDTH, DEMO_HEIGHT) != 0) /* Normalize once. */
        goto cleanup; /* Preserve the native error. */
    host.texture = SDL_CreateTexture(host.renderer, SDL_PIXELFORMAT_RGBA32, /* Byte-order-safe RGBA. */
                                     SDL_TEXTUREACCESS_STREAMING, DEMO_WIDTH, DEMO_HEIGHT); /* Fixed logical image. */
    if (!host.texture || SDL_SetTextureBlendMode(host.texture, SDL_BLENDMODE_NONE) != 0) /* Opaque demo frame. */
        goto cleanup; /* Destroy only acquired owners. */
    if (demo_open(&demo) != FVM_OK) /* Create VM/frame after native setup. */
        goto cleanup; /* Report actual VM status. */
    if (hide_cursor) demo.status = fvm_input_set_cursor_visible(demo.context, 0, &previous); /* Explicit startup request. */
    while (running && demo.status == FVM_OK) /* No worker may enter the VM or graphics APIs. */
    { /* Process events before generating each visible frame. */
        while (running && demo.status == FVM_OK && SDL_PollEvent(&event)) /* Drain pending events in order. */
            running = sdl_event(&host, &demo, &event); /* Dispatch only on this owning thread. */
        if (!running || demo.status != FVM_OK) /* Do not render after close/error. */
            break; /* Cleanup happens after no callback can run. */
        if (sdl_cursor(&host, &demo) != FVM_OK) break; /* Apply requests made by script/input before painting. */
        demo_frame(&demo, sdl_present, &host); /* Borrow frame bytes until synchronous upload returns. */
        if (smoke && demo.frames >= DEMO_MAX_STEPS) /* Deterministic finite software test mode. */
            running = 0; /* Interactive mode instead waits for a native close event. */
        SDL_Delay(DEMO_FRAME_MS); /* Coarse tutorial pacing, not an audio or movie clock. */
    } /* End frame/event loop before resource release. */
    result = demo.status == FVM_OK ? 0 : 1; /* Backend/VM failures stay visible. */
    if (!result) /* Distinguish completed presentation from initialization failure. */
        printf("SDL frames=%u wheels=%u last_delta=%.3f\n", demo.frames, demo.wheel_events, demo.last_wheel_delta); /* Smoke expects 120 frames. */
cleanup: /* All exits share reverse ownership cleanup. */
    if (host.cursor_original >= 0 && SDL_ShowCursor(host.cursor_original) < 0) result = 1; /* Restore global SDK state. */
    if (result) /* Print both diagnostic domains without hiding the original status. */
        fprintf(stderr, "SDL tutorial: VM=%d SDL=%s\n", (int)demo.status, SDL_GetError()); /* Borrow error text. */
    demo_close(&demo); /* No managed state survives the owner loop. */
    if (host.texture) SDL_DestroyTexture(host.texture); /* Release GPU/native copy first. */
    if (host.renderer) SDL_DestroyRenderer(host.renderer); /* Release dependent renderer second. */
    if (host.window) SDL_DestroyWindow(host.window); /* Release the native window last. */
    SDL_Quit(); /* Shut down the SDL subsystems acquired by this standalone host. */
    return result; /* A dummy-video run is software acceptance only. */
} /* This example does not claim Android Activity/lifecycle acceptance. */
