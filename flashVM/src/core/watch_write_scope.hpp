/* FlashVM: retain a thrown watcher value while completing loaded property assignment. */
#ifndef FVM_CORE_WATCH_WRITE_SCOPE_HPP
#define FVM_CORE_WATCH_WRITE_SCOPE_HPP /* Guard for deferred watcher exceptions. */
#include "runtime.hpp"

/* [GC:] Temporarily suspend a watcher exception so setters can run and collect.
 * Own writes restore it; inherited setter dispatch returns the setter result. */
class FVM_WatchWriteScope
{
    FVM_Context *context_; /* Context whose pending exception is suspended. */
    fvm_detail::vector<FVM_Value> saved_; /* [GC:] Owned slots retaining the original thrown graph. */
    FVM_GCRootFrame frame_; /* [GC:] Linked only after saving the exception succeeds. */
    bool linked_; /* Whether this scope has published its saved-value roots. */
    bool restore_; /* Whether destruction restores the original watcher exception. */
    /* A linked root scope cannot be copied. */
    FVM_WatchWriteScope(const FVM_WatchWriteScope &);
    /* Root-frame ownership cannot be assigned. */
    FVM_WatchWriteScope &operator=(const FVM_WatchWriteScope &);
  public:
    /* Construct without allocating for ordinary, non-throwing property writes. */
    explicit FVM_WatchWriteScope(FVM_Context *context)
        : context_(context), linked_(false), restore_(false) {}
    /* [GC:] Save before clearing pending state; allocation failure leaves it intact. */
    FVM_Result defer(FVM_Result status, FVM_Value &value)
    {
        if (status != FVM_EXCEPTION || !context_->swf_loaded || !context_->exception_pending)
            return status;
        saved_.push_back(context_->exception_value);
        frame_.previous = context_->gc_frames;
        frame_.stack = &saved_;
        frame_.locals = 0;
        frame_.receiver = 0;
        context_->gc_frames = &frame_;
        linked_ = restore_ = true;
        context_->exception_pending = false;
        context_->fault_pending = false;
        fvm_value_undefined(&context_->exception_value);
        fvm_value_undefined(&value);
        return FVM_OK;
    }
    /* [GC:] An inherited setter consumes the saved exception; own writes restore it. */
    FVM_Result finish(FVM_Result status, bool inherited_setter = false)
    {
        if (!linked_)
            return status;
        if (inherited_setter)
        {
            restore_ = false;
            return status;
        }
        return FVM_EXCEPTION;
    }
    /* [GC:] Restore pending ownership before releasing the saved root frame. */
    ~FVM_WatchWriteScope()
    {
        if (!linked_)
            return;
        if (restore_)
        {
            context_->exception_value = saved_[0];
            context_->exception_pending = true;
            context_->fault_pending = false;
        }
        context_->gc_frames = frame_.previous;
    }
};
#endif
