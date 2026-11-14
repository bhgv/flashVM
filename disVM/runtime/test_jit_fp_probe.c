/* Copyright (c) 2026 bhgv
 * EN: Real FP RX canaries and source-local FP-environment restoration evidence.
 * RU: Настоящие FP RX canaries и проверка восстановления FP environment своего source.
 */
#define _GNU_SOURCE
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "isa.h"
#include "jit_fault/signals.h"
#include "runtime_gc.h"
#include "disvm/native_context.h"
#include <sys/mman.h>
#include <fenv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static __thread int eligible; /* Source identity is printed before its REAL computation. */
static __thread int invoking; /* Native error bypasses callback return on this owner. */
static int injected;          /* Atomic once-only actual FP opcode injection. */
static int fault_pid;         /* Exact retained source, atomic. */
static int held;              /* Source is inside the genuine debug fault gate, atomic. */
static int gc_seen;           /* Completed Fast work during retention, atomic. */
static void *mapping;         /* Test-only immutable RX allocation, released after all joins. */
extern void Sys_print(void *);
int __real_dvm_runtime_fast(int, int (*)(REG *), REG *);
int __real_dvm_bigtable_call_ready(void (*)(void *), void *);
int __real_dvm_jit_native_error(const char *);
int __real_dvm_jit_fault_copy(DVM_JitFault *);
int __real_fesetenv(const fenv_t *);
void __real_dvm_debug_fault(const char *);
void __real_dvm_debug_native_gate(Prog *);
unsigned int __real_dvm_gc_fast_shared(DVM_NativeContext *, size_t, void *);
void __real_dvm_close_worker_stop(void);


/* EN/RU: Mark only the source that prints before REAL execution; preserve actual gate.
 * Отметить только source, печатающий до REAL execution; сохранить настоящий gate. */
int __wrap_dvm_bigtable_call_ready(void (*function)(void *), void *frame)
{
    if (function == Sys_print)
    {
        eligible = 1;
    }
    return __real_dvm_bigtable_call_ready(function, frame);
}


/* EN: Publish separate RX FP-bit canaries followed by a real illegal instruction.
 * No production JIT byte is patched; canary values are raw, never FP-converted.
 * RU: Опубликовать отдельные RX FP-bit canaries с настоящей illegal instruction.
 * Production JIT bytes целы; canary values — raw bits, без FP conversion. */
static int (*trap_page(void))(REG *)
{
    unsigned char code[128] = {0};
    uint64_t low = UINT64_C(0x1122334455667788);
    uint64_t high = UINT64_C(0x8877665544332211);
#if defined(__x86_64__)
    const unsigned char instructions[] = {0x48, 0xb8, 0,    0,    0,    0,    0,    0,    0,    0,    0x66, 0x48,
                                          0x0f, 0x6e, 0xc0, 0x48, 0xb8, 0,    0,    0,    0,    0,    0,    0,
                                          0,    0x66, 0x48, 0x0f, 0x6e, 0xc8, 0x66, 0x0f, 0x6c, 0xc1, 0x66, 0x4c,
                                          0x0f, 0x6e, 0xf8, 0xd9, 0xe8, 0xdb, 0xe2, 0x0f, 0xae, 0x15, 0,    0,
                                          0,    0,    0xd9, 0x2d, 0,    0,    0,    0,    0x0f, 0x0b};
    memcpy(code, instructions, sizeof(instructions));
    memcpy(code + 2, &low, 8);
    memcpy(code + 17, &high, 8);
    {
        uint32_t mxcsr = CONTROL == 3 ? 0x7d80 : 0x7f80;
        uint16_t control = 0x0f7f;
        uint32_t mxcsr_offset = 46;
        uint32_t control_offset = 44;
        memcpy(code + 46, &mxcsr_offset, 4);
        memcpy(code + 52, &control_offset, 4);
        memcpy(code + 96, &mxcsr, 4);
        memcpy(code + 100, &control, 2);
        if (CONTROL == 3)
        {
            const unsigned char divide[] = {
                0x66, 0x0f, 0x57, 0xd2, 0xf2, 0x0f, 0x10, 0x1d, 0, 0, 0, 0, 0xf2, 0x0f, 0x5e, 0xda, 0x0f, 0x0b};
            uint32_t displacement = 36;
            uint64_t one = UINT64_C(0x3ff0000000000000);
            memcpy(code + 56, divide, sizeof(divide));
            memcpy(code + 64, &displacement, 4);
            memcpy(code + 104, &one, 8);
        }
    }
#elif defined(__aarch64__)
    const uint32_t instructions[] = {
        0x9c000100U, 0x9c00017fU, 0xd2a01810U, 0xd51b4410U, 0xd51b443fU, 0, 0xd503201fU, 0xd503201fU};
    memcpy(code, instructions, sizeof(instructions));
    memcpy(code + 32, &low, 8);
    memcpy(code + 40, &high, 8);
    memcpy(code + 48, &high, 8);
    memcpy(code + 56, &low, 8);
#elif defined(__riscv) && __riscv_xlen == 64
    const uint32_t instructions[] = {
        0x00000297U, 0x0202b007U, 0x0282bf87U, 0x02000313U, 0x00331073U, 0, 0x00000013U, 0x00000013U};
    memcpy(code, instructions, sizeof(instructions));
    memcpy(code + 32, &low, 8);
    memcpy(code + 40, &high, 8);
#endif
    mapping = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mapping == MAP_FAILED)
    {
        perror("test FP RX allocation");
        exit(22);
    }
    memcpy(mapping, code, sizeof(code));
    __builtin___clear_cache(mapping, (char *)mapping + 4096);
    if (mprotect(mapping, 4096, PROT_READ | PROT_EXEC))
    {
        perror("test FP RX publication");
        exit(22);
    }
    return (int (*)(REG *))mapping;
}


/* EN: Intercept one genuine REAL addition, establish source-local rounding/flags,
 * then invoke the real runtime policy on actual RX. No scalar integer substitution.
 * RU: Перехватить настоящий REAL addition, установить source-local rounding/flags,
 * вызвать настоящую runtime policy с actual RX. Без integer подмены операции. */
int __wrap_dvm_runtime_fast(int operation, int (*fast)(REG *), REG *registers)
{
    int expected = 0;
    if (eligible && operation == IADDF &&
        __atomic_compare_exchange_n(&injected, &expected, 1, 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED))
    {
        if (__real_fesetenv(FE_DFL_ENV) || fesetround(FE_DOWNWARD) || feraiseexcept(FE_INEXACT))
        {
            fprintf(stderr, "test FP policy setup failed\n");
            exit(22);
        }
        fprintf(stderr, "JITFPOPCODE selected=%d\n", operation);
        fast = trap_page();
        dvm_jit_signal_select(mapping, 4096, operation);
        fprintf(stderr, "JITSIGNALINPUT register_image=%llx\n", (unsigned long long)(uintptr)registers);
        invoking = 1;
    }
    return __real_dvm_runtime_fast(operation, fast, registers);
}


/* EN/RU: CONTROL=2 omits only the production restoration attempt after RX landing.
 * CONTROL=2 убирает только production restoration attempt после RX landing. */
int __wrap_fesetenv(const fenv_t *environment)
{
    return CONTROL == 2 && invoking ? 0 : __real_fesetenv(environment);
}


/* EN: Observe source-local FP policy before guest error handling. Restore default
 * test policy afterward; never repair the missing restoration before observation.
 * RU: Проверить source-local FP policy до guest error handling. После проверки
 * вернуть default test policy; не исправлять пропуск до наблюдения. */
int __wrap_dvm_jit_native_error(const char *message)
{
    if (invoking && !strcmp(message, "sys: JIT native signal"))
    {
        int restored = fegetround() == FE_DOWNWARD && fetestexcept(FE_ALL_EXCEPT) == FE_INEXACT;
        fprintf(stderr, "JITFPENV restored=%d\n", restored);
        invoking = 0;
        if (__real_fesetenv(FE_DFL_ENV))
        {
            exit(22);
        }
    }
    return __real_dvm_jit_native_error(message);
}


/* EN/RU: CONTROL=1 removes only the copied FP publication, preserving native capture/GPR.
 * CONTROL=1 удаляет только copied FP publication, сохраняя native capture/GPR. */
int __wrap_dvm_jit_fault_copy(DVM_JitFault *output)
{
    int result = __real_dvm_jit_fault_copy(output);
    if (CONTROL == 1 && result)
    {
        output->fp_format = 0;
        output->fp_count = 0;
    }
    return result;
}


/* EN/RU: Mark the genuine original-error retention window before normal debug gate.
 * Отметить genuine original-error retention window до normal debug gate. */
void __wrap_dvm_debug_fault(const char *message)
{
    if (!strcmp(message, "sys: JIT native signal"))
    {
        __atomic_store_n(&fault_pid, currun()->pid, __ATOMIC_RELAXED);
        __atomic_store_n(&held, 1, __ATOMIC_RELEASE);
    }
    __real_dvm_debug_fault(message);
}


/* EN/RU: End the retention proof window only after the actual source gate returns.
 * Завершить retention proof window только после actual source gate. */
void __wrap_dvm_debug_native_gate(Prog *process)
{
    int pid = process ? process->pid : 0;
    __real_dvm_debug_native_gate(process);
    if (pid == __atomic_load_n(&fault_pid, __ATOMIC_RELAXED))
    {
        __atomic_store_n(&held, 0, __ATOMIC_RELEASE);
    }
}


/* [GC:] EN/RU: Observe real completed Fast work while the faulted source remains retained.
 * Наблюдать real completed Fast work при удержании faulted source. */
unsigned int __wrap_dvm_gc_fast_shared(DVM_NativeContext *context, size_t budget, void *user)
{
    unsigned int result = __real_dvm_gc_fast_shared(context, budget, user);
    if (result && __atomic_load_n(&held, __ATOMIC_ACQUIRE) && !__atomic_exchange_n(&gc_seen, 1, __ATOMIC_RELAXED))
    {
        fprintf(stderr, "JITSIGNALGC completed_while_held=1\n");
    }
    return result;
}


/* EN/RU: Release only test RX storage after all joins; print independent injection evidence.
 * Освободить только test RX storage после всех joins; вывести independent evidence. */
void __wrap_dvm_close_worker_stop(void)
{
    __real_dvm_close_worker_stop();
    if (mapping && mapping != MAP_FAILED)
    {
        munmap(mapping, 4096);
    }
    fprintf(stderr, "JITSIGNALPROBE sideeffects=0\n");
}
