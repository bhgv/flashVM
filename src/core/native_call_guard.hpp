/* [GC:] FlashVM: collector barriers for native callbacks and trusted rooted bridges. */
#ifndef FVM_NATIVE_CALL_GUARD_HPP
#define FVM_NATIVE_CALL_GUARD_HPP /* Native-entry guard. */
#include "runtime.hpp"
#include "finalizer_entry.hpp"
/* [GC:] Native callbacks block collector reentry. A trusted runtime bridge may
 * omit its own barrier only after retaining every value it uses across script.
 * Enclosing native barriers are never suspended or decremented by this option. */
class FVM_NativeCallGuard
{
    FVM_Context *context_; /* [GC:] Barrier owner, or NULL for a trusted bridge entry. */
    /* [GC:] An active collector barrier cannot be copied. */
    FVM_NativeCallGuard(const FVM_NativeCallGuard &);
    /* [GC:] Assignment would break the barrier nesting count. */
    FVM_NativeCallGuard &operator=(const FVM_NativeCallGuard &);

  public:
    /* [GC:] Enter a native callback after preparation and commit any armed finalizer attempt. */
    explicit FVM_NativeCallGuard(FVM_Context *context, bool barrier = true)
        : context_(barrier ? context : 0)
    {
        if (context_)
            ++context_->gc_native_depth;
        fvm_finalizer_enter(context);
    }
    /* [GC:] Restore only the barrier introduced by this entry on every exit path. */
    ~FVM_NativeCallGuard()
    {
        if (context_)
            --context_->gc_native_depth;
    }
};
#endif
