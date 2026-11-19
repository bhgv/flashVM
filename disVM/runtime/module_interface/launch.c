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
#include "binding.h"
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
    DVM_LaunchContext *context = dvm_module_launch_current(process);
    if (!module || module == H || !path || !context || !context->contract || strcmp(path, context->module))
    {
        return module;
    }
    return dvm_module_check_contract(module, context->contract);
}
