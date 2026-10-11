#ifndef FVM_NATIVE_CALL_GUARD_HPP
#define FVM_NATIVE_CALL_GUARD_HPP
#include "runtime.hpp"
/* C++98 scope guard: native callbacks must not reenter the collector.
   A callback may explicitly root objects and collect after it returns. */
class FVM_NativeCallGuard {
    FVM_Context *context_;
    FVM_NativeCallGuard(const FVM_NativeCallGuard &);
    FVM_NativeCallGuard &operator=(const FVM_NativeCallGuard &);
public:
    explicit FVM_NativeCallGuard(FVM_Context *context):context_(context) {
        ++context_->gc_native_depth;
    }
    ~FVM_NativeCallGuard() { --context_->gc_native_depth; }
};
#endif
