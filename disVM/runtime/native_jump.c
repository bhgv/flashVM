/* Keep sanitizer stack tracking on the actual nonlocal-transfer boundary. */
#if defined(__has_feature)
#if __has_feature(thread_sanitizer)
#define DVM_TSAN_JUMP 1 /* Clang's sanitizer feature spelling. */
#endif
#endif
#if defined(__SANITIZE_THREAD__)
#define DVM_TSAN_JUMP 1 /* GCC's sanitizer feature spelling. */
#endif
#if defined(DVM_TSAN_JUMP)
/* glibc redirects siglongjmp to __longjmp_chk under fortification. Some TSan
 * runtimes intercept only siglongjmp; bypassing that interceptor leaks their
 * shadow call stack on repeated errors. Disable fortification in this one small
 * translation unit before any system header; retain it in the ordinary build
 * and all other translation units. Do not disable sanitizer instrumentation. */
#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
#define _FORTIFY_SOURCE 0 /* Only the sanitizer's jump boundary is unfortified. */
#endif
#include "native_jump.h"


/* buffer/value are a live same-thread sigsetjmp continuation and nonzero status.
 * Transfer control, preserving its saved signal mask; never return. Caller has
 * already unwound native scopes and retained Proc error depth. No allocation,
 * locks, VM callback or managed ownership change. Invalid/stale buffers are UB,
 * as for POSIX siglongjmp: this boundary does not authorize cross-thread jumps. */
void dvm_native_error_jump(sigjmp_buf buffer, int value)
{
    siglongjmp(buffer, value);
}
