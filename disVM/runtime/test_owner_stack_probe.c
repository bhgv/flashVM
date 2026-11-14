/* Copyright (c) 2026 bhgv
 * EN: Verify active owner stack capture, bounded traversal and actual module release.
 * RU: Проверить active owner stack capture, bounded traversal и actual module release.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "managed_workers.h"
#include "disvm/native_context.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int observed;  /* Atomic once-only owner/geometry evidence. */
static int grew;      /* Atomic once-only real provider stack growth evidence. */
static int finalized; /* Atomic once-only final provider release evidence. */
extern void
__real_dvm_diagnostic_stack_capture(DVM_DiagnosticStack *, const DVM_DiagnosticLocation *, Modlink *, const void *);
extern void __real_dvm_resident_stack_publish(const DVM_DiagnosticStack *);
extern int __real_dvm_module_registry_release(Module *);
extern void __real_extend(void);


/* EN/RU: Refuse an invalid native unit/owner premise explicitly, without assertions.
 * Явно отказать при неверной native unit/owner premise, без assertions. */
static void require_stack(int valid)
{
    if (!valid)
    {
        fprintf(stderr, "FAIL owner stack active owner or native geometry\n");
        exit(14);
    }
}


/* EN: Own synthetic metadata through each call; check local/imported root, copied
 * unaligned header, cycles, malformed return and eight-frame truncation. No GC roots.
 * RU: Владеть synthetic metadata до возврата; проверить local/imported root,
 * unaligned header copy, cycles, malformed return и truncation восьми frames. */
static void geometry_unit(void)
{
    Module module;
    Modlink link;
    Inst code[16];
    Frame frames[10];
    unsigned char unaligned[sizeof(Frame) + 1]; /* Deliberately unaligned trusted header copy. */
    DVM_DiagnosticLocation top;
    DVM_DiagnosticStack result;
    int index;
    memset(&module, 0, sizeof(module));
    memset(&link, 0, sizeof(link));
    memset(frames, 0, sizeof(frames));
    module.prog = code;
    module.nprog = 16;
    module.name = "SyntheticCaller";
    module.path = "synthetic.dis";
    link.m = &module;
    link.prog = code;
    dvm_diagnostic_location_copy(&top, &link, code);
    for (index = 0; index < 9; index++)
    {
        frames[index].fp = (uchar *)&frames[index + 1];
        frames[index].lr = code + index + 1;
    }
    frames[0].mr = &link;
    __real_dvm_diagnostic_stack_capture(&result, &top, &link, frames);
    require_stack(result.flags == DVM_STACK_TRUNCATED && result.count == 8 && result.frames[7].next_pc == 7);
    frames[2].fp = nil;
    __real_dvm_diagnostic_stack_capture(&result, &top, &link, frames);
    require_stack(result.flags == DVM_STACK_COMPLETE && result.count == 3 && result.frames[2].next_pc == 2);
    frames[1].fp = (uchar *)frames;
    __real_dvm_diagnostic_stack_capture(&result, &top, &link, frames);
    require_stack(result.flags == DVM_STACK_UNAVAILABLE && result.count == 3);
    frames[0].lr = H;
    __real_dvm_diagnostic_stack_capture(&result, &top, &link, frames);
    require_stack(result.flags == DVM_STACK_UNAVAILABLE && result.count == 1);
    memset(frames, 0, sizeof(frames));
    memcpy(unaligned + 1, frames, sizeof(Frame));
    __real_dvm_diagnostic_stack_capture(&result, &top, &link, unaligned + 1);
    require_stack(result.flags == DVM_STACK_COMPLETE && result.count == 1);
}


/* [GC:] EN: Only the owner with a claimed turn supplies live TLS module/FP.
 * Borrow through real capture; CONTROL=2 corrupts only a copied caller label.
 * RU: Только owner с claimed turn передаёт live TLS module/FP. Заимствовать до
 * real capture; CONTROL=2 искажает только copied caller label. */
void __wrap_dvm_diagnostic_stack_capture(DVM_DiagnosticStack *output,
                                         const DVM_DiagnosticLocation *top,
                                         Modlink *module,
                                         const void *fp)
{
    DVM_NativeContext *context = dvm_native_context_current();
    require_stack(context && dvm_native_context_turn(context) && up && up->prog && module == DVM_R.M && fp == DVM_R.FP);
    if (!__atomic_exchange_n(&observed, 1, __ATOMIC_RELAXED))
    {
        geometry_unit();
        fprintf(stderr, "NATIVEDEBUGSTACK owner=1 tls=1 geometry=1 bounded=1\n");
    }
    __real_dvm_diagnostic_stack_capture(output, top, module, fp);
    if (CONTROL == 2 && output->count > 1 && output->frames[1].name_bytes)
    {
        output->frames[1].name[0] = '!';
    }
}


/* EN/RU: CONTROL=1 omits only stack publication, preserving actual guest work.
 * CONTROL=1 убирает только stack publication, сохраняя actual guest work. */
void __wrap_dvm_resident_stack_publish(const DVM_DiagnosticStack *stack)
{
    if (CONTROL != 1)
    {
        __real_dvm_resident_stack_publish(stack);
    }
}


/* [GC:] EN/RU: Observe actual final registry release before caller destroys metadata; retain nothing.
 * Наблюдать final registry release до уничтожения metadata caller; ничего не удерживать. */
int __wrap_dvm_module_registry_release(Module *module)
{
    int last = __real_dvm_module_registry_release(module);
    if (last && module->name && !strcmp(module->name, "StackProvider") &&
        !__atomic_exchange_n(&finalized, 1, __ATOMIC_RELAXED))
    {
        fprintf(stderr, "NATIVEDEBUGSTACK module_finalized=1\n");
    }
    return last;
}


/* [GC:] EN: Observe a successful production extent allocation while this owner
 * executes provider recursion. Borrow module only before the allocating call.
 * RU: Наблюдать успешное production extent allocation при provider recursion
 * своего owner. Заимствовать module только до allocating call. */
void __wrap_extend(void)
{
    int provider = DVM_R.M && DVM_R.M != H && DVM_R.M->m && DVM_R.M->m != H && DVM_R.M->m->name &&
                   !strcmp(DVM_R.M->m->name, "StackProvider");
    __real_extend();
    if (provider && !__atomic_exchange_n(&grew, 1, __ATOMIC_RELAXED))
    {
        fprintf(stderr, "NATIVEDEBUGSTACK stack_growth=1\n");
    }
}
