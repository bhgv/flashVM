/* FlashVM: desktop Darwin JIT policy; ARM64 callback publication is an explicit modern-SDK opt-in. */
#ifndef FVM_MAC_CODE_PLATFORM_H
#define FVM_MAC_CODE_PLATFORM_H /* The controlled SDK fixtures replace only this platform boundary. */
#if defined(__APPLE__) && defined(__MACH__)
#include <TargetConditionals.h>

#if TARGET_OS_OSX && defined(FVM_ENABLE_MACOS_ARM64_JIT) && FVM_ENABLE_MACOS_ARM64_JIT &&                              \
    (defined(__aarch64__) || defined(__arm64__)) && !defined(__arm64e__) && !defined(__ILP32__) &&                     \
    defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__) && __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ >= 110400
#define FVM_MAC_CODE_ARM64 1 /* Callback API requires macOS 11.4+; arm64e is a separate ABI. */
#else
#define FVM_MAC_CODE_ARM64 0 /* Older deployment targets keep explicit interpreter fallback. */
#endif
#if FVM_MAC_CODE_ARM64
#define FVM_MAC_CODE_SUPPORTED 1 /* Native instruction support still requires host signing permission. */
#elif TARGET_OS_OSX && defined(__x86_64__) && !defined(__ILP32__)
#define FVM_MAC_CODE_SUPPORTED 1 /* Ordinary desktop Intel x64 process; no Apple Silicon/iOS ABI inference. */
#else
#define FVM_MAC_CODE_SUPPORTED 0 /* MAP_JIT/Apple Silicon and other Apple platforms need separate policies. */
#endif
#else
#define FVM_MAC_CODE_SUPPORTED 0 /* This is not a Darwin process. */
#define FVM_MAC_CODE_ARM64 0     /* No Apple SDK is needed on this build. */
#endif
#endif
