/* FlashVM: explicitly scope the classic Darwin code-memory policy to desktop Intel macOS. */
#ifndef FVM_MAC_CODE_PLATFORM_H
#define FVM_MAC_CODE_PLATFORM_H /* The controlled SDK fixtures replace only this platform boundary. */
#if defined(__APPLE__) && defined(__MACH__)
#include <TargetConditionals.h>
#if TARGET_OS_OSX && defined(__x86_64__) && !defined(__ILP32__)
#define FVM_MAC_CODE_SUPPORTED 1 /* Ordinary desktop Intel x64 process; no Apple Silicon/iOS ABI inference. */
#else
#define FVM_MAC_CODE_SUPPORTED 0 /* MAP_JIT/Apple Silicon and other Apple platforms need separate policies. */
#endif
#else
#define FVM_MAC_CODE_SUPPORTED 0 /* This is not a Darwin process. */
#endif
#endif
