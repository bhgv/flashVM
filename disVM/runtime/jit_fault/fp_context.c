/* Copyright (c) 2026 bhgv
 * EN: Copy base Linux64 FP/SIMD signal state without executing floating arithmetic.
 * RU: Копировать base Linux64 FP/SIMD signal state без floating arithmetic.
 */
#define _GNU_SOURCE
#include "internal.h"
#include <ucontext.h>


/* EN: Borrow only the kernel's current signal frame. Copy FXSAVE x87/XMM,
 * ARM64 bounded FPSIMD or RV64 D state; never chase XSAVE/SVE/vector extensions.
 * Format0 explicitly denotes unavailable state. No allocation, libc or locks.
 * RU: Заимствовать только kernel signal frame. Копировать FXSAVE x87/XMM,
 * bounded ARM64 FPSIMD либо RV64 D; без XSAVE/SVE/vector extensions. Format0
 * явно означает unavailable state. Без allocation, libc либо locks. */
void dvm_jit_signal_fp_context(void *context, DVM_JitFault *output)
{
    ucontext_t *native = context;
    unsigned int index;
    __atomic_store_n(&output->fp_format, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&output->fp_count, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&output->fp_bits, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&output->x87_count, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&output->fp_tag, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&output->fp_extra, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&output->fp_control, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&output->fp_status, 0, __ATOMIC_RELAXED);
#if defined(__x86_64__)
    struct _libc_fpstate *fp = native->uc_mcontext.fpregs;
    if (!fp)
    {
        return;
    }
    __atomic_store_n(&output->fp_tag, fp->ftw & 255U, __ATOMIC_RELAXED);
    __atomic_store_n(&output->fp_control, fp->cwd, __ATOMIC_RELAXED);
    __atomic_store_n(&output->fp_status, fp->swd, __ATOMIC_RELAXED);
    __atomic_store_n(&output->fp_extra, fp->mxcsr, __ATOMIC_RELAXED);
    for (index = 0; index < 16; index++)
    {
        __atomic_store_n(&output->fp_registers[index][0],
                         (uint64_t)fp->_xmm[index].element[0] | (uint64_t)fp->_xmm[index].element[1] << 32,
                         __ATOMIC_RELAXED);
        __atomic_store_n(&output->fp_registers[index][1],
                         (uint64_t)fp->_xmm[index].element[2] | (uint64_t)fp->_xmm[index].element[3] << 32,
                         __ATOMIC_RELAXED);
    }
    for (index = 0; index < 8; index++)
    {
        __atomic_store_n(&output->x87_registers[index][0],
                         (uint64_t)fp->_st[index].significand[0] | (uint64_t)fp->_st[index].significand[1] << 16 |
                             (uint64_t)fp->_st[index].significand[2] << 32 |
                             (uint64_t)fp->_st[index].significand[3] << 48,
                         __ATOMIC_RELAXED);
        __atomic_store_n(&output->x87_registers[index][1], fp->_st[index].exponent, __ATOMIC_RELAXED);
    }
    __atomic_store_n(&output->fp_bits, 128, __ATOMIC_RELAXED);
    __atomic_store_n(&output->fp_count, 16, __ATOMIC_RELAXED);
    __atomic_store_n(&output->x87_count, 8, __ATOMIC_RELAXED);
    __atomic_store_n(&output->fp_format, 1, __ATOMIC_RELAXED);
#elif defined(__aarch64__)
    (void)index;
    dvm_jit_signal_fpsimd(native->uc_mcontext.__reserved, sizeof(native->uc_mcontext.__reserved), output);
#elif defined(__riscv) && __riscv_flen == 64
    __atomic_store_n(&output->fp_control, native->uc_mcontext.__fpregs.__d.__fcsr, __ATOMIC_RELAXED);
    __atomic_store_n(&output->fp_status, native->uc_mcontext.__fpregs.__d.__fcsr, __ATOMIC_RELAXED);
    for (index = 0; index < 32; index++)
    {
        __atomic_store_n(
            &output->fp_registers[index][0], native->uc_mcontext.__fpregs.__d.__f[index], __ATOMIC_RELAXED);
        __atomic_store_n(&output->fp_registers[index][1], 0, __ATOMIC_RELAXED);
    }
    __atomic_store_n(&output->fp_bits, 64, __ATOMIC_RELAXED);
    __atomic_store_n(&output->fp_count, 32, __ATOMIC_RELAXED);
    __atomic_store_n(&output->fp_format, 3, __ATOMIC_RELAXED);
#else
    (void)index;
    (void)native;
#endif
}


/* EN/RU: Name copied base FP format without inspecting live CPU registers.
 * Подписать copied base FP format без чтения live CPU registers. */
const char *dvm_jit_fault_fp_prefix(unsigned int format)
{
    return format == 1 ? "xmm" : format == 2 ? "v" : format == 3 ? "f" : "unavailable";
}
