/* FlashVM: modern macOS SDK boundary; callback allowlisting is supplied by Apple's C++ macro. */
#ifndef FVM_MAC_CODE_ARM64_SDK_HPP
#define FVM_MAC_CODE_ARM64_SDK_HPP /* Controlled tests replace this complete SDK boundary. */
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include <libkern/OSCacheControl.h>
/* C++98 expands the SDK's callback-list macro; no hand-written Mach-O section layout is inferred. */
#define FVM_MAC_JIT_ALLOW_CALLBACK(callback) PTHREAD_JIT_WRITE_ALLOW_CALLBACKS_NP(callback)
/* Metadata allocation stays outside the per-thread writable interval. */
#define FVM_MAC_JIT_ALLOCATE(size) std::malloc(size)
#define FVM_MAC_JIT_FREE(memory) std::free(memory)
#endif
