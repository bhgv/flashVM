/* Copyright (c) 2026 bhgv
 * EN: Test-only immutable RX traps, preserving real production signal capture and GC.
 * RU: Только тестовые immutable RX traps; настоящий production signal capture и GC сохранены.
 */
#define _GNU_SOURCE
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "isa.h"
#include "jit_fault/signals.h"
#include "runtime_gc.h"
#include "managed_owner.h"
#include "disvm/native_cleanup.h"
#include "disvm/native_context.h"
#include <sys/mman.h>
#include <signal.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static __thread int eligible;     /* Only the native owner that printed its source identity. */
static int injected;              /* Atomic once-only trap mapping publication. */
static int fault_pid;             /* Atomic exact retained source identity. */
static int fault_held;            /* Atomic source is inside retained debug fault gate. */
static int gc_observed;           /* Atomic completed Fast work during retention. */
static unsigned long sideeffects; /* Test-only scalar write before actual illegal instruction. */
static void *backing;             /* Truncated-file mapping for actual SIGBUS, never guest storage. */
static void *mapping;             /* Owned native test-only executable page, not guest code mutation. */
extern void Sys_print(void *);
int __real_dvm_runtime_fast(int, int (*)(REG *), REG *);
int __real_dvm_bigtable_call_ready(void (*)(void *), void *);
void __real_dvm_debug_fault(const char *);
void __real_dvm_debug_native_gate(Prog *);
int __real_dvm_jit_fault_copy(DVM_JitFault *);
unsigned int __real_dvm_gc_fast_shared(DVM_NativeContext *, size_t, void *);
void __real_dvm_close_worker_stop(void);


/* EN/RU: Source's first print marks only this thread eligible; forward actual host gate.
 * Первый print source отмечает только свой thread; вызвать настоящий host gate. */
int __wrap_dvm_bigtable_call_ready(void (*function)(void *), void *frame)
{
    if (function == Sys_print)
    {
        eligible = 1;
    }
    return __real_dvm_bigtable_call_ready(function, frame);
}


/* EN/RU: Test-only persistent native scope, deliberately left open before RX trap.
 * Только тестовый persistent native scope, намеренно оставленный открытым до RX trap. */
static DVM_NativeCleanup scope;


/* EN/RU: Normal callback can unlink its own native record; must not run after unsafe landing.
 * Normal callback снимает свою native record; не вызывается после unsafe landing. */
static int release_scope(void *argument)
{
    return dvm_native_cleanup_unlink(argument);
}


/* EN/RU: Leave a real cleanup registration alive; the RX landing must refuse recovery.
 * Оставить настоящую cleanup registration живой; RX landing должен отказать recovery. */
static void dirty_scope(void)
{
    if (!dvm_native_cleanup_link(&scope, release_scope, &scope))
    {
        fprintf(stderr, "test native cleanup registration failed\n");
        exit(22);
    }
    fprintf(stderr, "JITSIGNAL_DIRTY cleanup_linked=1\n");
}


/* EN: Build an immutable real trap page outside handlers. CONTROL=2 selects a
 * null read,4 truncated backing,5 partial write,7 repeat,8 pending cleanup. Production mappings and
 * instruction bytes are untouched. Exit on host refusal, never count it as proof.
 * RU: Создать immutable real trap page вне handlers. CONTROL=2 — null read;
 * остальные варианты — illegal instruction. Production mappings и bytes целы.
 * Host refusal — ошибка теста, не proof. */
static int (*trap_page(void))(REG *)
{
#if defined(__x86_64__)
    static const unsigned char illegal[] = {0x0f, 0x0b};
    static const unsigned char nullread[] = {0x48, 0x8b, 0x04, 0x25, 0, 0, 0, 0};
#elif defined(__aarch64__)
    static const unsigned char illegal[] = {0, 0, 0, 0};
    static const unsigned char nullread[] = {0xf0, 0x03, 0x1f, 0xaa, 0x00, 0x02, 0x40, 0xf9};
#elif defined(__riscv) && __riscv_xlen == 64
    static const unsigned char illegal[] = {0, 0, 0, 0};
    static const unsigned char nullread[] = {0x03, 0x35, 0x00, 0x00};
#endif
    unsigned char readcode[32] = {0};
    unsigned char partial[40] = {0};
    uintptr address;
    int nullfault = CONTROL == 2 || (CONTROL == 7 && __atomic_load_n(&injected, __ATOMIC_RELAXED) == 2);
    if (CONTROL == 4)
    {
        int descriptor = memfd_create("jit-signal-test", MFD_CLOEXEC);
        if (descriptor < 0 || ftruncate(descriptor, 4096))
        {
            perror("test SIGBUS backing");
            exit(22);
        }
        backing = mmap(NULL, 4096, PROT_READ, MAP_PRIVATE, descriptor, 0);
        if (backing == MAP_FAILED || ftruncate(descriptor, 0))
        {
            perror("test SIGBUS mapping");
            exit(22);
        }
        close(descriptor);
    }
    address = CONTROL == 4 ? (uintptr)backing : (uintptr)&sideeffects;
#if defined(__x86_64__)
    {
        const unsigned char reader[] = {0x48, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0x48, 0x8b, 0x00};
        const unsigned char writer[] = {0x48, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0x48, 0xff, 0x00, 0x0f, 0x0b};
        memcpy(readcode, reader, sizeof(reader));
        memcpy(partial, writer, sizeof(writer));
        memcpy(readcode + 2, &address, 8);
        memcpy(partial + 2, &address, 8);
    }
#elif defined(__aarch64__)
    {
        const unsigned int reader[] = {0x58000050, 0xf9400200};
        const unsigned int writer[] = {0x580000d0, 0xf9400211, 0x91000631, 0xf9000211, 0, 0xd503201f};
        memcpy(readcode, reader, sizeof(reader));
        memcpy(partial, writer, sizeof(writer));
        memcpy(readcode + 8, &address, 8);
        memcpy(partial + 24, &address, 8);
    }
#elif defined(__riscv) && __riscv_xlen == 64
    {
        const unsigned int reader[] = {0x00000297, 0x0102b283, 0x0002b503, 0x00000013};
        const unsigned int writer[] = {0x00000297, 0x0182b283, 0x0002b303, 0x00130313, 0x0062b023, 0};
        memcpy(readcode, reader, sizeof(reader));
        memcpy(partial, writer, sizeof(writer));
        memcpy(readcode + 16, &address, 8);
        memcpy(partial + 24, &address, 8);
    }
#endif
    if (CONTROL == 8)
    {
        uintptr helper = (uintptr)dirty_scope;
#if defined(__x86_64__)
        const unsigned char call[] = {0x48, 0x83, 0xec, 0x08, 0x48, 0xb8, 0,    0,    0,    0,    0,
                                      0,    0,    0,    0xff, 0xd0, 0x48, 0x83, 0xc4, 0x08, 0x0f, 0x0b};
        memcpy(partial, call, sizeof(call));
        memcpy(partial + 6, &helper, 8);
#elif defined(__aarch64__)
        const unsigned int call[] = {0x58000090, 0xd63f0200, 0, 0xd503201f};
        memcpy(partial, call, sizeof(call));
        memcpy(partial + 16, &helper, 8);
#else
        const unsigned int call[] = {0x00000297, 0x0102b283, 0x000280e7, 0};
        memcpy(partial, call, sizeof(call));
        memcpy(partial + 16, &helper, 8);
#endif
    }
    if (CONTROL == 9)
    {
        unsigned int offset = (unsigned int)offsetof(REG, M);
#if defined(__x86_64__)
        const unsigned char corrupt[] = {0x48, 0xc7, 0x87, 0, 0, 0, 0, 0, 0, 0, 0, 0x0f, 0x0b};
        memcpy(partial, corrupt, sizeof(corrupt));
        memcpy(partial + 3, &offset, 4);
#elif defined(__aarch64__)
        unsigned int corrupt[] = {0xf900001fU | ((offset / 8) << 10), 0};
        memcpy(partial, corrupt, sizeof(corrupt));
#else
        unsigned int corrupt[] = {0x00053023U | ((offset & 31) << 7) | ((offset >> 5) << 25), 0};
        memcpy(partial, corrupt, sizeof(corrupt));
#endif
        fprintf(stderr, "JITSIGNAL_ROOTS mutation_armed=1\n");
    }
    mapping = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mapping == MAP_FAILED)
    {
        perror("test RX allocation");
        exit(22);
    }
    if (CONTROL == 4)
    {
        memcpy(mapping, readcode, sizeof(readcode));
    }
    else if (CONTROL == 5 || CONTROL == 8 || CONTROL == 9)
    {
        memcpy(mapping, partial, sizeof(partial));
    }
    else
    {
        memcpy(mapping, nullfault ? nullread : illegal, nullfault ? sizeof(nullread) : sizeof(illegal));
    }
    __builtin___clear_cache(mapping, (char *)mapping + 4096);
    if (mprotect(mapping, 4096, PROT_READ | PROT_EXEC))
    {
        perror("test RX publication");
        exit(22);
    }
    return (int (*)(REG *))mapping;
}


/* EN/RU: An unsafe C/helper fault must never jump through the RX-only landing.
 * Unsafe C/helper fault не должен переходить через RX-only landing. */
static int foreign_fault(REG *registers)
{
    (void)registers;
    fprintf(stderr, "JITSIGNAL_FOREIGN_C entering=1\n");
#if defined(__x86_64__)
    __asm__ volatile("ud2");
#elif defined(__aarch64__)
    __asm__ volatile(".inst 0");
#else
    __asm__ volatile(".word 0");
#endif
    return 0;
}


/* EN/RU: Inject exactly one real RX fault into a selected addition before any effects.
 * Ввести ровно один real RX fault в selected addition до любых effects. */
int __wrap_dvm_runtime_fast(int operation, int (*fast)(REG *), REG *registers)
{
    int expected = __atomic_load_n(&injected, __ATOMIC_RELAXED);
    DVM_RangeAccess guard = {0};
    if (eligible && operation == IADDW && expected < (CONTROL == 7 ? 2 : 1) &&
        __atomic_compare_exchange_n(&injected, &expected, expected + 1, 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED))
    {
        if (CONTROL == 3)
        {
            fast = foreign_fault;
        }
        else
        {
            if (CONTROL == 6)
            {
                if (dvm_managed_ranges_begin(registers->d, sizeof(WORD), NULL, 0, &guard) != DVM_CELL_ACQUIRED)
                {
                    fprintf(stderr, "test native guard acquisition failed\n");
                    exit(22);
                }
                fprintf(stderr, "JITSIGNAL_GUARDED entering=1\n");
            }
            if (mapping && mapping != MAP_FAILED)
            {
                munmap(mapping, 4096);
                mapping = NULL;
            }
            fast = trap_page();
            dvm_jit_signal_select(mapping, 4096, operation);
        }
    }
    if (eligible && operation == IADDW && fast == (int (*)(REG *))mapping)
    {
        fprintf(stderr, "JITSIGNALINPUT register_image=%llx\n", (unsigned long long)(uintptr)registers);
    }
    return __real_dvm_runtime_fast(operation, fast, registers);
}


/* EN/RU: Observe the genuine retained-debug gate without changing exception semantics.
 * Наблюдать настоящий retained-debug gate без изменения exception semantics. */
void __wrap_dvm_debug_fault(const char *message)
{
    if (!strcmp(message, "sys: JIT native signal"))
    {
        __atomic_store_n(&fault_pid, currun()->pid, __ATOMIC_RELAXED);
        __atomic_store_n(&fault_held, 1, __ATOMIC_RELEASE);
    }
    __real_dvm_debug_fault(message);
}


/* EN/RU: CONTROL=1 omits only copied CPU publication; capture/handler/app remain genuine.
 * CONTROL=1 убирает только copied CPU publication; capture/handler/app настоящие. */
int __wrap_dvm_jit_fault_copy(DVM_JitFault *output)
{
    return CONTROL == 1 ? 0 : __real_dvm_jit_fault_copy(output);
}


/* [GC:] EN/RU: Require actual completed Fast work while native fault roots are retained.
 * Наблюдать actual completed Fast work при удержании native fault roots. */
unsigned int __wrap_dvm_gc_fast_shared(DVM_NativeContext *context, size_t budget, void *user)
{
    unsigned int result = __real_dvm_gc_fast_shared(context, budget, user);
    if (result && __atomic_load_n(&fault_held, __ATOMIC_ACQUIRE) &&
        !__atomic_exchange_n(&gc_observed, 1, __ATOMIC_RELAXED))
    {
        fprintf(stderr, "JITSIGNALGC completed_while_held=1\n");
    }
    return result;
}


/* EN/RU: Release test mapping only after genuine source/worker shutdown, then report evidence.
 * Освободить test mapping только после настоящего source/worker shutdown, вывести evidence. */
void __wrap_dvm_close_worker_stop(void)
{
    __real_dvm_close_worker_stop();
    if (mapping && mapping != MAP_FAILED)
    {
        munmap(mapping, 4096);
    }
    if (backing && backing != MAP_FAILED)
    {
        munmap(backing, 4096);
    }
    fprintf(stderr, "JITSIGNALPROBE sideeffects=%lu\n", sideeffects);
    fprintf(stderr,
            "JITSIGNALPROBE injected=%d gc=%d\n",
            __atomic_load_n(&injected, __ATOMIC_RELAXED),
            __atomic_load_n(&gc_observed, __ATOMIC_RELAXED));
}


/* [GC:] EN/RU: End proof window only when the genuine retained source gate returns.
 * Завершить proof window только после возврата настоящего retained source gate. */
void __wrap_dvm_debug_native_gate(Prog *process)
{
    int pid = process ? process->pid : 0;
    __real_dvm_debug_native_gate(process);
    if (pid == __atomic_load_n(&fault_pid, __ATOMIC_RELAXED))
    {
        __atomic_store_n(&fault_held, 0, __ATOMIC_RELEASE);
    }
}
