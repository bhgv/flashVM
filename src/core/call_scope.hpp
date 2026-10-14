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
    fvm_detail::vector<FVM_Value> roots_; /* [GC:] Retained old/new invocation objects. */
    FVM_GCRootFrame frame_;               /* [GC:] Frame linking invocation roots to the caller. */
    /* This scope owns one active root frame and cannot be copied. */
    FVM_CallScope(const FVM_CallScope &);
    /* Assignment would invalidate the active root frame. */
    FVM_CallScope &operator=(const FVM_CallScope &); /* Copy assignment is unavailable. */

  public:
    /* [GC:] Publish the selected constructor or method owner for one nested call. */
    FVM_CallScope(FVM_Context *c, FVM_Object *constructor, FVM_Object *function, FVM_Object *owner)
        : context_(c), old_constructor_(c->calling_constructor), old_function_(c->calling_function),
          old_owner_(c->calling_owner), roots_(FVM_CALL_SCOPE_SNAPSHOT_COUNT)
    {
        /* [GC:] Build complete snapshots before changing context-visible metadata. */
        FVM_Object *objects[FVM_CALL_SCOPE_SNAPSHOT_COUNT] = {
            old_constructor_, old_function_, old_owner_, constructor, function, owner};
        for (size_t i = 0; i < FVM_CALL_SCOPE_SNAPSHOT_COUNT; ++i)
        {
            fvm_value_undefined(&roots_[i]);
            if (objects[i])
            {
                roots_[i].type = FVM_VALUE_OBJECT;
                roots_[i].object = objects[i];
            }
        }
        /* [GC:] Link all snapshots into the active root chain. */
        frame_.previous = c->gc_frames;
        frame_.stack = &roots_;
        frame_.locals = 0;
        frame_.receiver = 0;
        /* [GC:] Attach the completed invocation snapshots to the root chain. */
        c->gc_frames = &frame_;
        c->calling_constructor = constructor;
        c->calling_function = function;
        c->calling_owner = owner;
    }
    /* [GC:] Restore invocation identity and roots even when the callee throws. */
    ~FVM_CallScope()
    {
        context_->calling_constructor = old_constructor_;
        context_->calling_function = old_function_;
        context_->calling_owner = old_owner_;
        /* [GC:] Pop this scope before destroying its snapshot vector. */
        context_->gc_frames = frame_.previous;
    }
};
#endif
