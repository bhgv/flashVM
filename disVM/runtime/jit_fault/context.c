/* Copyright (c) 2026 bhgv
 * EN: Linux64 kernel CPU-context projection for x64, ARM64 and RV64.
 * RU: Проекция Linux64 kernel CPU context для x64, ARM64 и RV64.
 */
#define _GNU_SOURCE
#include "internal.h"
#include <ucontext.h>


/* EN/RU: Read kernel PC from borrowed signal context; unsupported builds refuse.
 * Прочитать kernel PC из borrowed signal context; unsupported builds отказывают. */
uintptr_t dvm_jit_signal_pc(void *context)
{
    ucontext_t *native = context;
#if defined(__x86_64__)
    return (uintptr_t)native->uc_mcontext.gregs[REG_RIP];
#elif defined(__aarch64__)
    return (uintptr_t)native->uc_mcontext.pc;
#elif defined(__riscv) && __riscv_xlen == 64
    return (uintptr_t)native->uc_mcontext.__gregs[REG_PC];
#else
#error "Define audited kernel context mapping for this architecture"
#endif
}


/* EN/RU: Copy integer CPU registers using only lock-free atomic stores; no libc calls.
 * Копировать integer CPU registers только lock-free atomic stores; без libc calls. */
void dvm_jit_signal_context(void *context, DVM_JitFault *output)
{
    ucontext_t *native = context;
    unsigned int i;
    __atomic_store_n(&output->pc, dvm_jit_signal_pc(context), __ATOMIC_RELAXED);
#if defined(__x86_64__)
    __atomic_store_n(&output->sp, (uintptr_t)native->uc_mcontext.gregs[REG_RSP], __ATOMIC_RELAXED);
    __atomic_store_n(&output->count, NGREG, __ATOMIC_RELAXED);
    for (i = 0; i < NGREG; i++)
    {
        __atomic_store_n(&output->registers[i], (uint64_t)native->uc_mcontext.gregs[i], __ATOMIC_RELAXED);
    }
#elif defined(__aarch64__)
    __atomic_store_n(&output->sp, (uintptr_t)native->uc_mcontext.sp, __ATOMIC_RELAXED);
    __atomic_store_n(&output->count, 32, __ATOMIC_RELAXED);
    for (i = 0; i < 31; i++)
    {
        __atomic_store_n(&output->registers[i], (uint64_t)native->uc_mcontext.regs[i], __ATOMIC_RELAXED);
    }
    __atomic_store_n(&output->registers[31], (uint64_t)native->uc_mcontext.pstate, __ATOMIC_RELAXED);
#elif defined(__riscv) && __riscv_xlen == 64
    __atomic_store_n(&output->sp, (uintptr_t)native->uc_mcontext.__gregs[REG_SP], __ATOMIC_RELAXED);
    __atomic_store_n(&output->count, 32, __ATOMIC_RELAXED);
    for (i = 0; i < 32; i++)
    {
        __atomic_store_n(&output->registers[i], (uint64_t)native->uc_mcontext.__gregs[i], __ATOMIC_RELAXED);
    }
#endif
}


/* EN/RU: Return the compile-target name without allocation or runtime probing.
 * Вернуть имя compile target без allocation либо runtime probing. */
const char *dvm_jit_fault_architecture(void)
{
#if defined(__x86_64__)
    return "x64";
#elif defined(__aarch64__)
    return "arm64";
#else
    return "riscv64";
#endif
}


/* EN/RU: Label kernel integer register index, including explicit status/PC slots.
 * Подписать kernel integer register index, включая status/PC slots. */
const char *dvm_jit_fault_register_name(unsigned int index)
{
#if defined(__x86_64__)
    static const char
        *const names[] = {"r8",  "r9",     "r10",    "r11", "r12",    "r13",     "r14", "r15",
                          "rdi", "rsi",    "rbp",    "rbx", "rdx",    "rax",     "rcx", "rsp",
                          "rip", "eflags", "csgsfs", "err", "trapno", "oldmask", "cr2"}; /* Linux64 gregset order. */
#elif defined(__aarch64__)
    static const char *const names[] = {
        "x0",  "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7",  "x8",  "x9",    "x10",
        "x11", "x12", "x13", "x14", "x15", "x16", "x17", "x18", "x19", "x20",   "x21",
        "x22", "x23", "x24", "x25", "x26", "x27", "x28", "x29", "x30", "pstate"}; /* SP/PC have separate fields. */
#else
    static const char *const
        names[] = {"pc", "ra", "sp", "gp", "tp",  "t0",  "t1", "t2", "s0", "s1", "a0",
                   "a1", "a2", "a3", "a4", "a5",  "a6",  "a7", "s2", "s3", "s4", "s5",
                   "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"}; /* Linux64 gregset includes PC at0. */
#endif
    return index < sizeof(names) / sizeof(names[0]) ? names[index] : "unavailable";
}
