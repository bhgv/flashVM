/* FlashVM: private C89 boundary to the macOS ARM64 callback-protected shared code arena. */
#ifndef FVM_MAC_CODE_ARM64_H
#define FVM_MAC_CODE_ARM64_H /* Protect private port declarations. */
#include "../code_port.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Query the enabled build policy and per-thread write-protection facility. */
    int fvm_mac_arm64_code_supported(void);
    /* Reserve a reusable aligned slot, copy through the allowed callback and publish on success. */
    FVM_Result fvm_mac_arm64_code_create(const void *, size_t, void **, size_t *);
    /* Return an inactive slot to the pool; the single MAP_JIT region stays process-owned. */
    void fvm_mac_arm64_code_release(void *, size_t);
#ifdef __cplusplus
}
#endif
#endif
