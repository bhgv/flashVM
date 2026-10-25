/* FlashVM: allocation-free temporary root for a partially built native result object. */
#ifndef FVM_CORE_OBJECT_ROOT_SCOPE_HPP
#define FVM_CORE_OBJECT_ROOT_SCOPE_HPP /* Guard for the private C++98 root scope. */
#include "runtime.hpp"


/* [GC:] Retain one object's complete graph without changing invocation identity. */
class FVM_ObjectRootScope
{
    FVM_Context *context_;  /* Context owning the active root chain. */
    FVM_GCRootFrame frame_; /* [GC:] Receiver root linked until scope exit. */
    /* One active frame cannot be copied. */
    FVM_ObjectRootScope(const FVM_ObjectRootScope &);
    /* Assignment would detach a live root frame. */
    FVM_ObjectRootScope &operator=(const FVM_ObjectRootScope &);

  public:


    /* [GC:] Link the same-context object after successful result creation. */
    FVM_ObjectRootScope(FVM_Context *c, FVM_Object *object) : context_(c)
    {
        frame_.previous = c->gc_frames;
        frame_.stack = 0;
        frame_.locals = 0;
        frame_.receiver = object;
        c->gc_frames = &frame_;
    }


    /* [GC:] Restore the caller's roots on success, errors and allocation exceptions. */
    ~FVM_ObjectRootScope()
    {
        context_->gc_frames = frame_.previous;
    }
};
#endif
