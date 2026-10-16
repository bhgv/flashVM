/* [GC:] FlashVM: allocation-free temporary roots for a caller-owned mutable value vector. */
#ifndef FVM_CORE_VALUE_VECTOR_ROOT_SCOPE_HPP
#define FVM_CORE_VALUE_VECTOR_ROOT_SCOPE_HPP /* Guard the private C++98 root scope. */
#include "runtime.hpp"
/* [GC:] Borrow an initialized vector that outlives this scope; mutations remain visible to GC. */
class FVM_ValueVectorRootScope
{
    FVM_Context *context_; /* Context owning the temporary root chain. */
    FVM_GCRootFrame frame_; /* [GC:] Frame retaining current vector elements until scope exit. */
    /* [GC:] A linked frame has unique stack ownership. */
    FVM_ValueVectorRootScope(const FVM_ValueVectorRootScope &);
    /* [GC:] Assignment would break root-chain nesting. */
    FVM_ValueVectorRootScope &operator=(const FVM_ValueVectorRootScope &);
  public:
    /* [GC:] Link the vector object after its initialized storage has been prepared. */
    FVM_ValueVectorRootScope(FVM_Context *context, fvm_detail::vector<FVM_Value> &values)
        : context_(context)
    {
        frame_.previous = context->gc_frames;
        frame_.stack = &values;
        frame_.locals = 0;
        frame_.receiver = 0;
        context->gc_frames = &frame_;
    }
    /* [GC:] Unlink before the caller destroys storage, including during C++ unwinding. */
    ~FVM_ValueVectorRootScope() { context_->gc_frames = frame_.previous; }
};
#endif
