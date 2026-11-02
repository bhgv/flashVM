/* Private C99-compatible acquire/release stop request; never a heap barrier. */
#ifndef DVM_WORLD_REQUEST_H
#define DVM_WORLD_REQUEST_H
#if defined(_MSC_VER)
#include <intrin.h>
#endif


/* Read an aligned request word concurrently with collector publication. word
 * lives through all owner joins. Unknown compilers conservatively take the
 * locked poll path; they never perform an unsynchronized plain shared read. */
static int dvm_world_request_load(long *word)
{
#if defined(DVM_WORLD_NO_ATOMICS)
    (void)word;
    return 1;
#elif defined(_MSC_VER)
    return _InterlockedCompareExchange(word,0,0) != 0;
#elif defined(__GNUC__) || defined(__clang__)
    return __atomic_load_n(word,__ATOMIC_ACQUIRE) != 0;
#else
    (void)word;
    return 1;
#endif
}


/* Publish or clear the request under the world mutex. value is zero or one;
 * all concurrent accesses use these helpers. Fallback readers always lock and
 * use the mutex-protected collector predicate instead of this optional word. */
static void dvm_world_request_store(long *word, long value)
{
#if defined(DVM_WORLD_NO_ATOMICS)
    (void)word;
    (void)value;
#elif defined(_MSC_VER)
    _InterlockedExchange(word,value);
#elif defined(__GNUC__) || defined(__clang__)
    __atomic_store_n(word,value,__ATOMIC_RELEASE);
#else
    (void)word;
    (void)value;
#endif
}
#endif
