/* FlashVM: rooted invocation metadata for constructor and superclass dispatch. */
#ifndef FVM_CORE_CALL_SCOPE_HPP
#define FVM_CORE_CALL_SCOPE_HPP /* Include guard for invocation scope metadata. */
#include "runtime.hpp"
#include "call_scope_constants.h"

/* [GC:] Save invocation identity and retain both old and new owner objects. */
class FVM_CallScope
{
    FVM_Context *context_;        /* Context whose invocation state is changed. */
    FVM_Object *old_constructor_; /* [GC:] Constructor restored when the nested call ends. */
    FVM_Object *old_function_;    /* [GC:] Function restored when the nested call ends. */
    FVM_Object *old_owner_;       /* [GC:] Prototype owner restored when the nested call ends. */
    unsigned int old_super_depth_; /* Explicit/inferred superclass depth restored after nesting. */
    /* [GC:] Inline frames retain saved/active identities without allocating a temporary vector. */
    FVM_GCRootFrame frames_[FVM_CALL_SCOPE_SNAPSHOT_COUNT];
    /* This scope owns one active root frame and cannot be copied. */
    FVM_CallScope(const FVM_CallScope &);
    /* Assignment would invalidate the active root frame. */
    FVM_CallScope &operator=(const FVM_CallScope &); /* Copy assignment is unavailable. */

  public:
    /* [GC:] Publish invocation identities and optional explicit superclass depth for one call. */
    FVM_CallScope(FVM_Context *c, FVM_Object *constructor, FVM_Object *function, FVM_Object *owner,
                  unsigned int super_depth = FVM_CALL_SCOPE_INFER_SUPER_DEPTH)
        : context_(c), old_constructor_(c->calling_constructor), old_function_(c->calling_function),
          old_owner_(c->calling_owner), old_super_depth_(c->calling_super_depth)
    {
        /* [GC:] Build complete snapshots before changing context-visible metadata. */
        FVM_Object *objects[FVM_CALL_SCOPE_SNAPSHOT_COUNT] = {
            old_constructor_, old_function_, old_owner_, constructor, function, owner};
        for (size_t i = 0; i < FVM_CALL_SCOPE_SNAPSHOT_COUNT; ++i)
        {
            frames_[i].previous = i ? &frames_[i - 1] : c->gc_frames;
            frames_[i].stack = 0;
            frames_[i].locals = 0;
            frames_[i].receiver = objects[i];
        }
        /* [GC:] Publish only the complete, allocation-free snapshot chain. */
        c->gc_frames = &frames_[FVM_CALL_SCOPE_SNAPSHOT_COUNT - 1];
        c->calling_constructor = constructor;
        c->calling_function = function;
        c->calling_owner = owner;
        c->calling_super_depth = super_depth;
    }
    /* [GC:] Restore invocation identity and roots even when the callee throws. */
    ~FVM_CallScope()
    {
        context_->calling_constructor = old_constructor_;
        context_->calling_function = old_function_;
        context_->calling_owner = old_owner_;
        context_->calling_super_depth = old_super_depth_;
        /* [GC:] Detach the inline chain after restoring the previous invocation identity. */
        context_->gc_frames = frames_[0].previous;
    }
};
#endif
