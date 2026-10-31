/* Real SDL FIFO regression: captured paint must retain the entire trajectory. */
#include "editor_desktop_input.h"
#include <stdio.h>
#include <stdlib.h>


/* Fail before reading invalid queue state. No predicates disappear under NDEBUG. */
static void require(int value, const char *message)
{
    if (!value)
    {
        fprintf(stderr, "FAIL paint input: %s (%s)\n", message, SDL_GetError());
        exit(1);
    }
}


/* Push a copied same-device mouse sample; x/y form a deliberately closed turn. */
static void motion(int x, int y)
{
    SDL_Event event;
    SDL_zero(event);
    event.type = SDL_MOUSEMOTION;
    event.motion.windowID = 1;
    event.motion.which = 2;
    event.motion.state = SDL_BUTTON_LMASK;
    event.motion.x = x;
    event.motion.y = y;
    require(SDL_PushEvent(&event) == 1, "push motion");
}


/* Test actual queue consumption, not a second implementation of its policy. */
int main(void)
{
    SDL_Event first;
    SDL_Event next;
    require(SDL_Init(SDL_INIT_EVENTS) == 0, "initialize events without a display");
    SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
    motion(10, 10);
    motion(30, 0);
    motion(10, 10);
    require(SDL_PeepEvents(&first, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) == 1, "remove first paint point");
    dvm_editor_coalesce_motion(&first, 1);
    require(first.motion.x == 10 && first.motion.y == 10, "head preserved");
    require(SDL_PeepEvents(&next, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) == 1 && next.motion.x == 30,
            "turn not lost");
    require(SDL_PeepEvents(&next, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) == 1 && next.motion.x == 10,
            "return point not lost");
    motion(1, 1);
    motion(2, 2);
    motion(3, 3);
    require(SDL_PeepEvents(&first, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) == 1, "remove drag head");
    dvm_editor_coalesce_motion(&first, 0);
    require(first.motion.x == 3 && first.motion.y == 3, "uncaptured drag stays coalesced");
    require(SDL_PeepEvents(&next, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) == 0,
            "drag consumed only matching motion");
    motion(4, 4);
    SDL_zero(next);
    next.type = SDL_MOUSEBUTTONUP;
    next.button.button = SDL_BUTTON_LEFT;
    require(SDL_PushEvent(&next) == 1, "push release barrier");
    motion(5, 5);
    require(SDL_PeepEvents(&first, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) == 1, "remove barrier head");
    dvm_editor_coalesce_motion(&first, 0);
    require(first.motion.x == 4, "release prevents crossing FIFO boundary");
    require(SDL_PeepEvents(&next, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) == 1 &&
                next.type == SDL_MOUSEBUTTONUP,
            "release retained");
    require(SDL_PeepEvents(&next, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) == 1 && next.motion.x == 5,
            "following motion retained");
    SDL_Quit();
    puts("PASS paint input FIFO: captured turn, return, drag coalescing and release ordering");
    return 0;
}
