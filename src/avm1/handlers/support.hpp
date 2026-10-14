/* FlashVM: shared validation and temporary roots for value-based execution services. */
#ifndef FVM_EXEC_SUPPORT_HPP
#define FVM_EXEC_SUPPORT_HPP /* Private service support guard. */
#include "flashvm/execution.h"
#include "../coercion.hpp"
#include "../../core/string_internal.hpp"
#include "../../core/native_call_guard.hpp"
#include "../../standard/movieclip_state.hpp"
#include "flashvm/metamethod.h"
#include "flashvm/table.h"
#include "flashvm/property.h"
#include <cmath>
#include <limits>
#include <new>
/* [GC:] Own a mutable value array without changing this, targets or interpreter state. */
class FVM_ExecutionRoots
{
    FVM_Context *context_;  /* Context owning the root chain. */
    FVM_GCRootFrame frame_; /* [GC:] Linked temporary roots. */
    /* [GC:] A linked frame cannot be copied. */
    FVM_ExecutionRoots(const FVM_ExecutionRoots &);
    /* [GC:] A linked frame cannot be assigned. */
    FVM_ExecutionRoots &operator=(const FVM_ExecutionRoots &);

  public:
    fvm_detail::vector<FVM_Value> values; /* [GC:] Mutable operands and intermediate values. */
    /* [GC:] Initialize all slots before publishing the frame. */
    FVM_ExecutionRoots(FVM_Context *c, size_t count) : context_(c), values(count)
    {
        for (size_t i = 0; i < count; ++i)
            fvm_value_undefined(&values[i]);
        frame_.previous = c->gc_frames;
        frame_.stack = &values;
        frame_.locals = 0;
        frame_.receiver = 0;
        c->gc_frames = &frame_;
    }
    /* [GC:] Restore the caller's chain on success, failure and allocation exceptions. */
    ~FVM_ExecutionRoots()
    {
        context_->gc_frames = frame_.previous;
    }
};
#endif
