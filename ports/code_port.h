/* FlashVM: executable-memory adapter; writable preparation is separate from executable use. */
#ifndef FVM_CODE_PORT_H
#define FVM_CODE_PORT_H /* Executable memory port guard. */
#include "flashvm/flashvm.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Report support for the selected native ABI and executable-memory policy. */
    int fvm_port_code_supported(void);
    /* Copy through non-executable writable preparation, then publish executable code.
     * macOS ARM64 uses callback-controlled per-thread W^X inside one MAP_JIT arena.
     * Errors preserve outputs.
     * Windows x64 requires the fixed encoder frame and registers its unwind table.
     * The size output is an opaque release cookie: Linux/Intel macOS mapping length, ARM64 macOS slot size, Windows table offset. */
    FVM_Result fvm_port_code_create(const void *, size_t, void **, size_t *);
    /* Release an inactive executable mapping created by this adapter. */
    void fvm_port_code_release(void *, size_t);
#ifdef __cplusplus
}
#endif
#endif
