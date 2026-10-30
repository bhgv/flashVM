/* Isolated native SDL contact-state checks. The real editor implementation is
 * compiled here; only the header geometry query is a named test boundary.
 * Actual geometry, source isolation and rendering are checked by the Dis replay. */
#define dvm_desktop_header touch_test_header
#include "editor_view.c"

/* Deterministic borrowed canvas-header boundary for native contact tests. */
int touch_test_header(double x, double y)
{
    return x >= 84 && x < 848 && y >= 72 && y < 96;
}

/* Submit one native SDL contact value. Expected code/phase are independent
 * protocol observations, zero means the contact must produce no proposal. */
static int contact(Uint32 type,
                   Uint32 time,
                   SDL_TouchID device,
                   SDL_FingerID finger,
                   float x,
                   float y,
                   unsigned long code,
                   unsigned long phase)
{
    SDL_TouchFingerEvent event;
    DVM_UIMessage message;
    int result;
    memset(&event, 0, sizeof(event));
    memset(&message, 0, sizeof(message));
    event.type = type;
    event.timestamp = time;
    event.touchId = device;
    event.fingerId = finger;
    event.x = x;
    event.y = y;
    result = desktop_touch(&event, &message);
    if ((code && (!result || message.code != code || message.flags != phase)) || (!code && result))
    {
        fprintf(stderr,
                "contact mismatch at %u: got %d/%lu/%lu expected %lu/%lu\n",
                time,
                result,
                message.code,
                message.flags,
                code,
                phase);
        return 0;
    }
    return 1;
}

/* Check actual SDL event timestamps, contact transitions, device-local IDs and
 * zoom termination. No window, native device or document is mocked as accepted. */
int main(void)
{
    DVM_UIMessage message;
    desktop_touch(NULL, &message);
    if (!contact(SDL_FINGERDOWN, 100, 1, 1, .12f, .125f, 1, 1) ||
        !contact(SDL_FINGERUP, 110, 1, 1, .12f, .125f, 1, 3) ||
        !contact(SDL_FINGERDOWN, 150, 1, 1, .12f, .125f, 10, 1) ||
        !contact(SDL_FINGERUP, 160, 1, 1, .12f, .125f, 1, 3) || !contact(SDL_FINGERDOWN, 180, 1, 1, .12f, .125f, 1, 1))
    {
        return 1;
    }
    desktop_touch(NULL, &message);
    if (!contact(SDL_FINGERDOWN, 200, 1, 7, .3f, .4f, 1, 1) || !contact(SDL_FINGERDOWN, 210, 2, 7, .4f, .4f, 1, 3) ||
        !contact(SDL_FINGERMOTION, 220, 2, 7, .4f, .4f, 6, 1) ||
        !contact(SDL_FINGERMOTION, 230, 2, 7, .5f, .4f, 8, 1) || !contact(SDL_FINGERUP, 240, 1, 7, .3f, .4f, 6, 3))
    {
        return 1;
    }
    desktop_touch(NULL, &message);
    if (!contact(SDL_FINGERDOWN, 300, 1, 1, .3f, .4f, 1, 1) || !contact(SDL_FINGERDOWN, 310, 1, 2, .4f, .4f, 1, 3) ||
        !contact(SDL_FINGERMOTION, 320, 1, 2, .4f, .4f, 6, 1) || !contact(SDL_FINGERDOWN, 330, 1, 3, .5f, .4f, 6, 3) ||
        !contact(SDL_FINGERMOTION, 340, 1, 3, .5f, .4f, 7, 1) || !contact(SDL_FINGERUP, 350, 1, 3, .5f, .4f, 7, 3))
    {
        return 1;
    }
    desktop_touch(NULL, &message);
    if (!contact(SDL_FINGERMOTION, 400, 1, 2, .4f, .4f, 0, 0))
    {
        return 1;
    }
    /* Use the actual SDL event queue to preserve a release between motions. */
    {
        SDL_Event first;
        SDL_Event queued;
        if (SDL_Init(SDL_INIT_EVENTS) != 0)
        {
            fprintf(stderr, "SDL events: %s\n", SDL_GetError());
            return 1;
        }
        memset(&first, 0, sizeof(first));
        first.type = SDL_MOUSEMOTION;
        first.motion.state = SDL_BUTTON_LMASK;
        first.motion.x = 10;
        queued = first;
        queued.motion.x = 20;
        if (SDL_PushEvent(&queued) != 1)
        {
            return 1;
        }
        queued.motion.x = 30;
        if (SDL_PushEvent(&queued) != 1)
        {
            return 1;
        }
        queued.type = SDL_MOUSEBUTTONUP;
        if (SDL_PushEvent(&queued) != 1)
        {
            return 1;
        }
        queued = first;
        queued.motion.x = 40;
        if (SDL_PushEvent(&queued) != 1)
        {
            return 1;
        }
        coalesce_desktop_motion(&first);
        if (first.motion.x != 30 || SDL_PollEvent(&queued) != 1 || queued.type != SDL_MOUSEBUTTONUP ||
            SDL_PollEvent(&queued) != 1 || queued.type != SDL_MOUSEMOTION || queued.motion.x != 40)
        {
            fputs("motion coalescing lost/reordered the release boundary\n", stderr);
            SDL_Quit();
            return 1;
        }
        SDL_Quit();
    }
    /* A deliberately unfinished raster must not throttle queued text or keys.
     * Use the production admission gate and real SDL FIFO, with a pointer between
     * characters. Replay/capture must retain their original presentation barrier. */
    {
        SDL_Event event;
        SDL_Event taken;
        if (SDL_Init(SDL_INIT_EVENTS) != 0)
        {
            return 1;
        }
        SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
        SDL_EventState(SDL_TEXTINPUT, SDL_ENABLE);
        memset(&event, 0, sizeof(event));
        event.type = SDL_TEXTINPUT;
        strcpy(event.text.text, "Я");
        if (SDL_PushEvent(&event) != 1)
        {
            return 1;
        }
        event.type = SDL_MOUSEBUTTONDOWN;
        if (SDL_PushEvent(&event) != 1)
        {
            return 1;
        }
        event.type = SDL_KEYUP;
        if (SDL_PushEvent(&event) != 1)
        {
            return 1;
        }
        raster_pending = 1;
        desktop_dirty = 1;
        if (presentation_blocks_input())
        {
            fprintf(stderr, "typing incorrectly waits for raster completion\n");
            return 1;
        }
        capture_path = "test-only capture barrier";
        if (!presentation_blocks_input())
        {
            return 1;
        }
        capture_path = NULL;
        events = fopen(__FILE__, "rb");
        if (!events || !presentation_blocks_input())
        {
            fprintf(stderr, "replay barrier failed: stream=%p\n", (void *)events);
            return 1;
        }
        fclose(events);
        events = NULL;
        if (SDL_PollEvent(&taken) != 1 || taken.type != SDL_TEXTINPUT || strcmp(taken.text.text, "Я") ||
            !presentation_blocks_input() || SDL_PollEvent(&taken) != 1 || taken.type != SDL_MOUSEBUTTONDOWN ||
            presentation_blocks_input() || SDL_PollEvent(&taken) != 1 || taken.type != SDL_KEYUP ||
            !presentation_blocks_input())
        {
            fprintf(stderr, "asynchronous input broke FIFO or presentation barriers\n");
            return 1;
        }
        /* An already captured header continues against owner geometry, while a
         * fresh button boundary still waits for matching pixels. Keep FIFO order. */
        SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
        memset(&event, 0, sizeof(event));
        event.type = SDL_MOUSEMOTION;
        event.motion.state = SDL_BUTTON_LMASK;
        if (SDL_PushEvent(&event) != 1)
        {
            return 1;
        }
        event.type = SDL_MOUSEBUTTONUP;
        event.button.button = SDL_BUTTON_LEFT;
        if (SDL_PushEvent(&event) != 1)
        {
            return 1;
        }
        event.type = SDL_MOUSEBUTTONDOWN;
        if (SDL_PushEvent(&event) != 1)
        {
            return 1;
        }
        header_drag = 1;
        if (presentation_blocks_input())
        {
            fprintf(stderr, "captured header motion incorrectly waits for pixels\n");
            return 1;
        }
        capture_path = "captured header still respects replay capture";
        if (!presentation_blocks_input())
        {
            return 1;
        }
        capture_path = NULL;
        if (SDL_PollEvent(&taken) != 1 || taken.type != SDL_MOUSEMOTION || presentation_blocks_input() ||
            SDL_PollEvent(&taken) != 1 || taken.type != SDL_MOUSEBUTTONUP)
        {
            fprintf(stderr, "captured header lost FIFO motion/release: %u\n", taken.type);
            return 1;
        }
        header_drag = 0;
        if (!presentation_blocks_input() || SDL_PollEvent(&taken) != 1 || taken.type != SDL_MOUSEBUTTONDOWN)
        {
            fprintf(stderr, "header continuation crossed a new pointer boundary\n");
            return 1;
        }
        raster_pending = 0;
        desktop_dirty = 0;
        if (presentation_blocks_input())
        {
            return 1;
        }
        SDL_Quit();
    }
    puts("PASS native SDL contacts: double tap, pair/triple transition, device IDs, table pinch, release, focus reset");
    return 0;
}
