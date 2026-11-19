/* Copyright (c) 2026 bhgv
 * [GC:] EN: A later named-target load cannot silently replace the accepted interface; other load policies are unchanged.
 * RU: Поздний named-target load не подменяет accepted interface молча; остальные load policies не меняются.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "kernel.h"
#include "runqueue.h"
#include "../vfs/launch/context.h"
#include "contract.h"
#include "launch.h"
#include <string.h>


/* [GC:] EN/RU: See launch.h; current permanent owner pins its immutable native context through this synchronous check.
 * Owned input pins parsed exports across native-only allocation; no managed allocation or module execution.
 * См. launch.h; current permanent owner удерживает immutable native context до возврата.
 * Owned input удерживает parsed exports при native-only allocation; без managed allocation/module execution.
 */
Module *dvm_launch_module_checked(const char *path, Module *module)
{
    Prog *process = currun();
    DVM_LaunchContext *context = process ? ((DVM_RuntimeProcess *)process)->launch_context : NULL;
    if (!module || module == H || !path || !context || !context->contract || strcmp(path, context->module))
    {
        return module;
    }
    DVM_LaunchContract *exports = dvm_module_contract_exports(module);
    int matched = exports && dvm_launch_contract_matches(context->contract, exports);
    dvm_launch_contract_free(exports);
    if (!matched)
    {
        unload(module);
        kwerrstr("launch target interface changed or cannot be checked");
        return NULL;
    }
    return module;
}
