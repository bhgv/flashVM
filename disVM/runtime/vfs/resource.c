/* Copyright (c) 2026 bhgv
 * EN: Owner-local resource cleanup and native operations on frozen selections.
 * RU: Owner-local resource cleanup и native operations по frozen selections.
 */
#include "resource.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: Release registered plan before abandoning its C frame; no VM calls.
 * Освободить plan до ухода из C frame; без вызовов VM. */
static int unwind(void *argument)
{
    dvm_resource_close(argument);
    return 1;
}


/* EN/RU: Register native cleanup after resolution; called by resolution.c only.
 * Зарегистрировать native cleanup после resolution; вызов только из resolution.c. */
int dvm_resource_register(DVM_ResourcePlan *plan)
{
    if (dvm_native_context_current() && !dvm_native_cleanup_link(&plan->cleanup, unwind, plan))
    {
        dvm_anchor_release(plan->anchor);
        plan->anchor = NULL;
        return 0;
    }
    return 1;
}


/* EN/RU: See resource.h; native unwind and explicit return use the same release.
 * См. resource.h; native unwind и обычный return используют один release. */
void dvm_resource_close(DVM_ResourcePlan *plan)
{
    if (!plan)
    {
        return;
    }
    if (plan->cleanup.context && !dvm_native_cleanup_unlink(&plan->cleanup))
    {
        abort();
    }
    dvm_anchor_release(plan->anchor);
    dvm_anchor_release(plan->created);
    plan->anchor = NULL;
    plan->created = NULL;
}


/* EN/RU: See resource.h. Directory identity is held; child changes remain native.
 * См. resource.h. Identity каталога удержана; изменения children — native. */
FVM_Result dvm_resource_info(DVM_ResourcePlan *plan, FVM_FileInfo *output)
{
    return plan->anchor ? fvm_port_anchor_info_at(dvm_anchor_port(plan->anchor), plan->relative, output)
                        : fvm_port_path_info(plan->names.native, output);
}


/* EN/RU: See resource.h; return owned independent cursor/file, not duplicated offsets.
 * См. resource.h; вернуть независимый cursor/file, не общий duplicated offset. */
FVM_Result
dvm_resource_open(DVM_ResourcePlan *plan, const char *mode, FVM_PortFile **file, FVM_PortDirectory **directory)
{
    FVM_FileInfo metadata;
    *file = NULL;
    *directory = NULL;
    if (!strcmp(mode, "r") && dvm_resource_info(plan, &metadata) == FVM_OK && metadata.directory)
    {
        return plan->anchor ? fvm_port_directory_open_at(dvm_anchor_port(plan->anchor), plan->relative, directory)
                            : fvm_port_directory_open(plan->names.native, directory);
    }
    return plan->anchor ? fvm_port_file_open_at(dvm_anchor_port(plan->anchor), plan->relative, mode, file)
                        : fvm_port_file_open(plan->names.native, mode, file);
}


/* EN/RU: See resource.h; production groups always have retained anchors.
 * См. resource.h; production groups всегда имеют retained anchors. */
FVM_Result dvm_resource_remove(DVM_ResourcePlan *plan)
{
    return plan->anchor ? fvm_port_anchor_remove_at(dvm_anchor_port(plan->anchor), plan->relative) : FVM_UNSUPPORTED;
}
