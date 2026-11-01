/* Bound native wheel work without losing legacy scroll distance or FIFO fences. */
#ifndef DVM_EDITOR_WHEEL_BATCH_H
#define DVM_EDITOR_WHEEL_BATCH_H
#include <SDL.h>
#define DVM_EDITOR_WHEEL_LIMIT 16U /* 16*120 physical pixels fits the existing +/-2048 scroll request. */
#define DVM_EDITOR_WHEEL_PHASE 16U /* Private desktop pointer phase prefix; phases0..3 keep legacy meaning. */

/* Borrow an already removed vertical wheel head. Consume at most15 following
 * same-device/window/direction/position packets, stopping before any other event
 * or opposite direction. Return1..16 legacy packet steps, never a net delta:
 * preserving direction order preserves clamping at desktop/shelf boundaries.
 * Caller serializes the SDL queue. No VM, allocation or rendering access.
 */
static unsigned int dvm_editor_wheel_batch(const SDL_Event *head)
{
    unsigned int count = 1; /* Head already belongs to the caller. */
    SDL_Event next;         /* Borrowed copied FIFO head before removal. */
    if (head->type != SDL_MOUSEWHEEL || !head->wheel.y || head->wheel.x)
    {
        return count;
    }
    while (count < DVM_EDITOR_WHEEL_LIMIT)
    {
        if (SDL_PeepEvents(&next, 1, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) != 1 ||
            next.type != SDL_MOUSEWHEEL || next.wheel.windowID != head->wheel.windowID ||
            next.wheel.which != head->wheel.which || next.wheel.direction != head->wheel.direction || next.wheel.x ||
            !next.wheel.y || (next.wheel.y > 0) != (head->wheel.y > 0))
        {
            break;
        }
#if SDL_VERSION_ATLEAST(2, 26, 0)
        if (next.wheel.mouseX != head->wheel.mouseX || next.wheel.mouseY != head->wheel.mouseY)
        {
            break;
        }
#endif
        if (SDL_PeepEvents(&next, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) != 1)
        {
            break;
        }
        count++;
    }
    return count;
}
#endif
