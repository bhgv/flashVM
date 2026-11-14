/* Copyright (c) 2026 bhgv
 * EN: Verify location captures on active owner TLS and bounded metadata geometry.
 * RU: Проверить location captures на active owner TLS и bounded metadata geometry.
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

static int finalized; /* Atomic once-only evidence of actual provider final release. */
static int observed;  /* Atomic once-only ownership and native geometry evidence. */
extern void __real_dvm_diagnostic_location_capture(DVM_DiagnosticLocation *, Modlink *, const void *);
extern void __real_dvm_resident_location_publish(const DVM_DiagnosticLocation *);
extern int __real_dvm_module_registry_release(Module *);


/* EN/RU: Fail explicitly on semantic ownership/geometry errors; no assertions.
 * Явно отказать при semantic ownership/geometry errors; без assertions. */
static void require_location(int valid)
{
    if (!valid)
    {
        fprintf(stderr, "FAIL owner location geometry or TLS boundary\n");
        exit(14);
    }
}


/* EN: Own native stack metadata through callback. Prove end-PC, misalignment,
 * native-PC refusal, invalid UTF-8 and complete truncated prefixes without GC.
 * RU: Владеть stack metadata до возврата callback. Проверить end-PC, misalignment,
 * native-PC refusal, invalid UTF-8 и целые truncated prefixes без GC. */
static void geometry_unit(void)
{
    Module module;
    Modlink link;
    Inst code[4];
    DVM_DiagnosticLocation result;
    char label[600]; /* Valid repeated two-byte lambda, longer than both native capacities. */
    unsigned int index;
    memset(&module, 0, sizeof(module));
    memset(&link, 0, sizeof(link));
    for (index = 0; index < 598; index += 2)
    {
        label[index] = (char)0xce;
        label[index + 1] = (char)0xbb;
    }
    label[598] = 0;
    module.prog = code;
    module.nprog = 4;
    module.name = label;
    module.path = label;
    link.m = &module;
    link.prog = code;
    __real_dvm_diagnostic_location_capture(&result, &link, code + 4);
    require_location(result.flags == 7 && result.next_pc == 4 && result.nprog == 4 && result.name_bytes == 62 &&
                     result.path_bytes == 254 && !result.name[62] && !result.path[254]);
    __real_dvm_diagnostic_location_capture(&result, &link, (uchar *)code + 1);
    require_location(!result.flags && !result.name_bytes && !result.path_bytes);
    module.compiled = 1;
    __real_dvm_diagnostic_location_capture(&result, &link, code);
    require_location(!result.flags);
    module.compiled = 0;
    module.name = "\xc0\x80";
    __real_dvm_diagnostic_location_capture(&result, &link, code);
    require_location(!result.flags && !result.name_bytes && !result.path_bytes);
}


/* EN: Capture arguments must be this thread's live TLS registers while it owns
 * a claimed turn; no target execution or foreign register read is introduced.
 * RU: Capture arguments — живые TLS registers своего потока с claimed turn;
 * не добавлять target execution либо чтение чужих registers. */
void __wrap_dvm_diagnostic_location_capture(DVM_DiagnosticLocation *output, Modlink *module, const void *pc)
{
    DVM_NativeContext *context = dvm_native_context_current();
    require_location(context && dvm_native_context_turn(context) && up && up->prog && module == DVM_R.M &&
                     pc == DVM_R.PC);
    if (!__atomic_exchange_n(&observed, 1, __ATOMIC_RELAXED))
    {
        geometry_unit();
        fprintf(stderr, "NATIVEDEBUGLOCATION owner=1 tls=1 geometry=1 utf8=1\n");
    }
    __real_dvm_diagnostic_location_capture(output, module, pc);
    if (CONTROL == 2 && (output->flags & DVM_LOCATION_VALID) && output->path_bytes)
    {
        output->path[0] = '!';
    }
}


/* EN/RU: CONTROL=1 omits only copied-boundary publication; execution remains unchanged.
 * CONTROL=1 убирает только copied-boundary publication; execution не меняется. */
void __wrap_dvm_resident_location_publish(const DVM_DiagnosticLocation *location)
{
    if (CONTROL != 1)
    {
        __real_dvm_resident_location_publish(location);
    }
}


/* [GC:] EN: After final registry release, caller owns immutable native metadata
 * until its destructor continues. Record real provider finalization, retain nothing.
 * RU: После final registry release caller владеет immutable native metadata до
 * продолжения destructor. Зафиксировать provider finalization без retain. */
int __wrap_dvm_module_registry_release(Module *module)
{
    int last = __real_dvm_module_registry_release(module);
    if (last && module->name && !strcmp(module->name, "LocationProvider") &&
        !__atomic_exchange_n(&finalized, 1, __ATOMIC_RELAXED))
    {
        fprintf(stderr, "NATIVEDEBUGLOCATION module_finalized=1\n");
    }
    return last;
}
