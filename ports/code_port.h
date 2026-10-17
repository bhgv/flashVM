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
    /* Copy bytes through RW storage, seal RX, then publish. Errors preserve outputs.
     * Windows x64 requires the fixed encoder frame and registers its unwind table.
     * The size output is an opaque release cookie: Linux/macOS mapping length, Windows table offset. */
    FVM_Result fvm_port_code_create(const void *, size_t, void **, size_t *);
    /* Release an inactive executable mapping created by this adapter. */
    void fvm_port_code_release(void *, size_t);
#ifdef __cplusplus
}
#endif
#endif
