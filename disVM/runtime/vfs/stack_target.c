/* Copyright (c) 2026 bhgv
 * EN: Validate an absent explicit file mount point beneath an existing visible directory.
 * RU: Проверка отсутствующей явной file mount point внутри существующего видимого каталога.
 */
#include "stack.h"
#include <string.h>


/* EN/RU: See stack.h; distinguish genuine absent leaves from permission/provider failures.
 * См. stack.h; отличать отсутствующий leaf от permission/provider failures. */
int dvm_stack_file_target(DVM_ResourcePlan *target)
{
    FVM_FileInfo info;
    for (DVM_ResourcePlan *choice = target; choice; choice = choice->layers)
    {
        if (dvm_resource_raw_info(choice, &info) != FVM_NOT_FOUND)
        {
            return -1;
        }
    }
    char parent[DVM_NS_PATH_LIMIT];
    strcpy(parent, target->names.path);
    char *leaf = strrchr(parent, '/');
    if (!leaf || !leaf[1])
    {
        return -1;
    }
    if (leaf == parent)
    {
        leaf[1] = 0;
    }
    else
    {
        *leaf = 0;
    }
    DVM_ResourcePlan plan;
    if (!dvm_resource_plan(target->group, parent, 0, &plan))
    {
        return -1;
    }
    int result = plan.names.version != target->names.version ? 0 :
        dvm_resource_info(&plan, &info) == FVM_OK && info.directory ? 1 : -1;
    dvm_resource_close(&plan);
    return result;
}
