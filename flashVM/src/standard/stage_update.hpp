/* FlashVM: dynamic redraw eligibility for supported input and interval callback routes. */
#ifndef FVM_STANDARD_STAGE_UPDATE_HPP
#define FVM_STANDARD_STAGE_UPDATE_HPP /* Guard private redraw callback/scope declarations. */
#include "../core/runtime.hpp"
/* Mark a redraw only inside an eligible event/interval call chain; arguments are ignored. */
FVM_Result fvm_builtin_update_after_event(FVM_Context *, FVM_Object *, unsigned int,
                                          const FVM_Value *, FVM_Value *, void *);
/* Restore the enclosing eligibility on every return, exception and native allocation failure. */
class FVM_StageUpdateEventScope
{
    FVM_Context *context_; /* Owning context, valid throughout the enclosing rooted delivery. */
    bool previous_; /* Dynamic eligibility inherited from the enclosing event call chain. */
    FVM_StageUpdateEventScope(const FVM_StageUpdateEventScope &); /* Keep restoration ownership unique. */
    FVM_StageUpdateEventScope &operator=(const FVM_StageUpdateEventScope &); /* Prevent double restoration. */
  public:
    /* Grant eligibility for this callback without disabling an already eligible outer call. */
    FVM_StageUpdateEventScope(FVM_Context *c, bool eligible)
        : context_(c), previous_(c->stage_update_allowed)
    {
        if (eligible)
            c->stage_update_allowed = true;
    }
    /* Restore only eligibility; a committed pending redraw survives callback errors. */
    ~FVM_StageUpdateEventScope() { context_->stage_update_allowed = previous_; }
};
#endif
