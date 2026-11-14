/* Copyright (c) 2026 bhgv
 * EN: Inject a side-effect-free backend exception before one actual native addition.
 * RU: Ввести backend exception без side effects до одного actual native сложения.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "isa.h"
#include "raise.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int injected;        /* Source executes the first bad call before all later calls. */
static Inst *failed_pc;     /* Borrowed comparison only; module remains rooted in fixture. */
static unsigned long other; /* Native additions outside the quarantined function. */
int __real_dvm_jit_native_error(const char *);
int __real_dvm_runtime_fast(int, int (*)(REG *), REG *);
void __real_dvm_close_worker_stop(void);


/* EN/RU: Throw explicit backend exception before writes; runtime must not replay it.
 * Выбросить explicit backend exception до writes; runtime не должен повторять её. */
static int fault(REG *registers)
{
    (void)registers;
    error(exCompile);
    return 0;
}


/* EN: Replace first demo addition only; later calls of the failed opcode must
 * bypass native dispatch entirely. Leave unrelated functions' genuine JIT intact.
 * RU: Заменить только первое demo сложение; последующие вызовы сбойной instruction
 * обходят native dispatch полностью. JIT остальных функций оставить настоящим. */
int __wrap_dvm_runtime_fast(int operation, int (*fast)(REG *), REG *registers)
{
    if (registers->M && registers->M->m && registers->M->m->name && !strcmp(registers->M->m->name, "QuarantineDemo"))
    {
        if (!injected && operation == IADDW)
        {
            injected = 1;
            failed_pc = registers->PC;
            return __real_dvm_runtime_fast(operation, fault, registers);
        }
        if (injected)
        {
            fprintf(
                stderr, "QUARANTINEENTRY pc=%ld op=%d\n", (long)(registers->PC - registers->M->m->prog - 1), operation);
        }
        if (CONTROL != 1 && injected && registers->PC == failed_pc)
        {
            fprintf(stderr, "FAIL quarantined function executed native callback again\n");
            exit(21);
        }
        if (injected && operation == IADDW)
        {
            __atomic_fetch_add(&other, 1, __ATOMIC_RELAXED);
        }
    }
    return __real_dvm_runtime_fast(operation, fast, registers);
}


/* EN/RU: Print independent evidence only after the real worker shutdown.
 * Вывести независимые evidence только после настоящего worker shutdown. */
void __wrap_dvm_close_worker_stop(void)
{
    __real_dvm_close_worker_stop();
    fprintf(stderr, "QUARANTINEPROBE injected=%d other=%lu\n", injected, __atomic_load_n(&other, __ATOMIC_RELAXED));
}


/* EN/RU: CONTROL=1 omits classification only; original error handling stays intact.
 * CONTROL=1 убирает только classification; original error handling сохранён. */
int __wrap_dvm_jit_native_error(const char *message)
{
    if (CONTROL == 1)
    {
        return 0;
    }
    return __real_dvm_jit_native_error(message);
}
