/* FlashVM: scoped native drag update reentry protection. */
#ifndef FVM_DRAG_SUPPORT_HPP
#define FVM_DRAG_SUPPORT_HPP /* Drag update scope guard. */
#include "support.hpp"


/* Restore the reentry flag on every callback error, allocation failure and early return. */
class FVM_DragUpdateGuard
{
    FVM_Context *context_;                                       /* Context whose update flag this scope owns. */
    FVM_DragUpdateGuard(const FVM_DragUpdateGuard &);            /* Reentry ownership cannot be copied. */
    FVM_DragUpdateGuard &operator=(const FVM_DragUpdateGuard &); /* Assignment is disabled. */

  public:


    /* Enter only after the caller has checked for reentry. */
    explicit FVM_DragUpdateGuard(FVM_Context *c) : context_(c)
    {
        c->drag_updating = true;
    }


    /* Release precisely this update's flag. */
    ~FVM_DragUpdateGuard()
    {
        context_->drag_updating = false;
    }
};
#endif
