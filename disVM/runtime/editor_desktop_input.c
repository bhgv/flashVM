/* Preserve authored trajectories while retaining cheap window/table dragging. */
#include "editor_desktop_input.h"


/* event is borrowed owner state and captured is a scalar gesture flag.
 * Write only the latest eligible uncaptured motion; painting consumes nothing
 * beyond the original head. No allocation, callbacks or VM access occurs. */
void dvm_editor_coalesce_motion(SDL_Event *event, int captured)
{
    SDL_Event next;
    unsigned int count;
    if (captured || (event->type != SDL_MOUSEMOTION && event->type != SDL_FINGERMOTION))
    {
        return;
    }
    for (count = 0; count < 256; ++count)
    {
        int same;
        if (SDL_PeepEvents(&next, 1, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) != 1)
        {
            break;
        }
        same = next.type == event->type;
        if (same && next.type == SDL_MOUSEMOTION)
        {
            same = next.motion.windowID == event->motion.windowID && next.motion.which == event->motion.which &&
                   next.motion.state == event->motion.state;
        }
        if (same && next.type == SDL_FINGERMOTION)
        {
            same = next.tfinger.touchId == event->tfinger.touchId && next.tfinger.fingerId == event->tfinger.fingerId;
        }
        if (!same || SDL_PeepEvents(&next, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) != 1)
        {
            break;
        }
        *event = next;
    }
}
