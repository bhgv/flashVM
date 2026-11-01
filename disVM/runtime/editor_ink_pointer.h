/* Native live ink belongs to one copied SDL pointer, never a VM object. */
#ifndef DVM_EDITOR_INK_POINTER_H
#define DVM_EDITOR_INK_POINTER_H
#include <SDL.h>


/* Classify motion/release against an admitted left-button/finger Down value.
 * Return1 for the
 * captured pointer,0 for another device/contact/window, and-1 when the owning
 * mouse reports that its left button is no longer held. The caller seals at the
 * last admitted point on-1; the hover coordinate must never become paint.
 * Both events are non-null borrowed immutable copies. No queue, allocation or VM access.
 */
static int dvm_editor_ink_pointer(const SDL_Event *begin, const SDL_Event *event)
{
    if (begin->type == SDL_MOUSEBUTTONDOWN)
    {
        if (event->type == SDL_MOUSEMOTION)
        {
            if (event->motion.windowID != begin->button.windowID || event->motion.which != begin->button.which)
            {
                return 0;
            }
            return (event->motion.state & SDL_BUTTON_LMASK) ? 1 : -1;
        }
        return event->type == SDL_MOUSEBUTTONUP && event->button.button == SDL_BUTTON_LEFT &&
               event->button.windowID == begin->button.windowID && event->button.which == begin->button.which;
    }
    return begin->type == SDL_FINGERDOWN && (event->type == SDL_FINGERMOTION || event->type == SDL_FINGERUP) &&
           event->tfinger.windowID == begin->tfinger.windowID && event->tfinger.touchId == begin->tfinger.touchId &&
           event->tfinger.fingerId == begin->tfinger.fingerId;
}
#endif
