/* Copyright (c) 2026 bhgv
 * EN: Built-in synchronous RX leaf fault capture; never replay a failed instruction.
 * RU: Встроенный захват synchronous RX leaf faults; без повторения сбойной инструкции.
 */
#ifndef DVM_JIT_SIGNALS_H
#define DVM_JIT_SIGNALS_H
#include <stddef.h>
#include <stdint.h>
struct REG;


/* EN/RU: Copied native CPU context; numeric values grant no memory-read authority.
 * Копия native CPU context; числовые значения не разрешают чтение памяти. */
typedef struct DVM_JitFault
{
    int signal_number;      /* Actual synchronous OS signal, zero when absent. */
    int signal_code;        /* Kernel si_code, never inferred from a managed exception. */
    uintptr_t address;      /* Kernel fault address. */
    uintptr_t pc;           /* Actual native instruction address. */
    uintptr_t sp;           /* Actual native stack pointer. */
    uintptr_t rx_base;      /* Admitted immutable JIT mapping base. */
    size_t rx_size;         /* Admitted mapping extent. */
    unsigned int count;     /* Architecture register count, bounded by32. */
    uint64_t registers[32]; /* x64 gregset, ARM64 x0..x30/pstate, RV64 gregset. */
} DVM_JitFault;


/* EN/RU: Install handlers before threads; preserve defaults/restore on shutdown.
 * Установить handlers до threads; сохранить dispositions и вернуть при shutdown. */
int dvm_jit_signals_start(void);
void dvm_jit_signals_stop(void);


/* [GC:] EN/RU: Publish scalar mapping provenance before invoking selected code.
 * Публиковать scalar mapping provenance до вызова selected code. */
void dvm_jit_signal_select(const void *base, size_t size, int opcode);


/* [GC:] EN: Invoke fast once using original live REG. Guard only admitted RX scalar
 * leaves with no held object guards; other callbacks retain ordinary semantics.
 * A caught fault raises a new native-fault error after capture, never retries.
 * RU: Вызвать fast один раз с live REG. Защищать только RX scalar leaves без
 * object guards; остальные callbacks сохраняют обычную семантику. Пойманная
 * авария вызывает native-fault error после capture, без повторения операции. */
int dvm_jit_signal_invoke(int (*fast)(struct REG *), struct REG *registers, int opcode);


/* EN/RU: Copy this owner's retained CPU context, or return0; no foreign TLS reads.
 * Копировать CPU context своего owner либо вернуть0; без foreign TLS reads. */
int dvm_jit_fault_copy(DVM_JitFault *output);


/* EN/RU: Return immutable build-architecture/CPU register labels for copied context.
 * Вернуть immutable build-architecture/CPU register labels для copied context. */
const char *dvm_jit_fault_architecture(void);
const char *dvm_jit_fault_register_name(unsigned int index);


/* EN/RU: Clear previous diagnostic at the next instruction boundary, not on fault release.
 * Снять предыдущую диагностику на следующей instruction boundary, не при fault release. */
void dvm_jit_fault_clear(void);


/* EN/RU: Disarm stale landing state when VM error unwinds past its invocation.
 * Снять stale landing state при VM error unwind за пределы invocation. */
void dvm_jit_signal_disarm(void);


/* EN/RU: Release this owner's optional alternate signal stack only outside native invocation.
 * Освободить optional alternate signal stack своего owner только вне native invocation. */
void dvm_jit_signal_thread_stop(void);
#endif
