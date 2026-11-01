/* FIFO batching for captured paint gestures, independent of Flash frames. */
#include "editor_motion_batch.h"
#include <stdlib.h>


/* Compare copied identities/buttons; timestamps/positions do not change capture.
 * Return nonzero only for the same mouse or same finger on the same surface. */
static int matching(const SDL_Event *a, const SDL_Event *b)
{
    if (a->type != b->type)
    {
        return 0;
    }
    if (a->type == SDL_MOUSEMOTION)
    {
        return a->motion.windowID == b->motion.windowID && a->motion.which == b->motion.which &&
               a->motion.state == b->motion.state;
    }
    return a->type == SDL_FINGERMOTION && a->tfinger.windowID == b->tfinger.windowID &&
           a->tfinger.touchId == b->tfinger.touchId && a->tfinger.fingerId == b->tfinger.fingerId;
}


/* Keep finger coordinates exact because their pixel scale belongs to the caller.
 * Mouse jitter can merge only when collinear and close in space AND time; never
 * erase a short deliberate turn or the first segment of a returning trajectory. */
static int redundant(const SDL_Event *a, const SDL_Event *b, const SDL_Event *c)
{
    long ax;
    long ay;
    long bx;
    long by;
    if (a->type != SDL_MOUSEMOTION || (Uint32)(c->motion.timestamp - b->motion.timestamp) > 2)
    {
        return 0;
    }
    ax = (long)b->motion.x - a->motion.x;
    ay = (long)b->motion.y - a->motion.y;
    bx = (long)c->motion.x - b->motion.x;
    by = (long)c->motion.y - b->motion.y;
    return labs(ax) <= 4096 && labs(ay) <= 4096 && labs(bx) <= 1 && labs(by) <= 1 && ax * by == ay * bx &&
           ax * bx + ay * by >= 0;
}


/* All storage belongs to the caller. Consume at most256 contiguous samples per
 * call, retaining at most64; leave the next unconsumed sample in the FIFO. */
unsigned int dvm_editor_motion_batch(const SDL_Event *head, SDL_Event *points)
{
    unsigned int used = 1;
    unsigned int consumed;
    SDL_Event next;
    points[0] = *head;
    if (head->type != SDL_MOUSEMOTION && head->type != SDL_FINGERMOTION)
    {
        return used;
    }
    for (consumed = 0; consumed < 255 && used < DVM_EDITOR_MOTION_POINTS; ++consumed)
    {
        if (SDL_PeepEvents(&next, 1, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) != 1 || !matching(head, &next) ||
            SDL_PeepEvents(&next, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) != 1)
        {
            break;
        }
        if (used >= 2 && redundant(&points[used - 2], &points[used - 1], &next))
        {
            points[used - 1] = next;
        }
        else
        {
            points[used++] = next;
        }
    }
    return used;
}


/* Seal before accepted non-trajectory input; ordinary OS notifications remain
 * harmless. Release itself is handled by the capture endpoint, not this fence. */
int dvm_editor_motion_barrier(const SDL_Event *event)
{
    switch (event->type)
    {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
    case SDL_TEXTINPUT:
    case SDL_QUIT:
    case SDL_FINGERDOWN:
        return 1;
    case SDL_MOUSEBUTTONDOWN:
        return event->button.button == SDL_BUTTON_LEFT || event->button.button == SDL_BUTTON_MIDDLE;
    case SDL_MOUSEWHEEL:
        return event->wheel.y != 0;
    case SDL_WINDOWEVENT:
        return event->window.event == SDL_WINDOWEVENT_CLOSE ||
               event->window.event == SDL_WINDOWEVENT_RESIZED ||
               event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
               event->window.event == SDL_WINDOWEVENT_MAXIMIZED ||
               event->window.event == SDL_WINDOWEVENT_RESTORED ||
               event->window.event == SDL_WINDOWEVENT_FOCUS_LOST;
    default:
        return 0;
    }
}
