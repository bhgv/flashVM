/* Copyright (c) 2026 bhgv
 * EN: Explicit resource purpose admission for existing and future code-loading adapters.
 * RU: Explicit назначение ресурса для существующих и будущих code-loading adapters.
 */
#include "intent.h"
#include <string.h>


/* EN/RU: See intent.h; intent is trusted adapter metadata, not a permission grant.
 * См. intent.h; intent задаёт adapter, но это не permission grant. */
int dvm_resource_plan_intent(DVM_Namespace *group, const char *path, DVM_ResourceIntent intent, DVM_ResourcePlan *plan)
{
    if (!plan)
    {
        return 0;
    }
    if (!group || !path || intent < DVM_RESOURCE_DATA || intent > DVM_RESOURCE_SWF)
    {
        memset(plan, 0, sizeof(*plan));
        return 0;
    }
    if (!dvm_resource_plan(group, path, intent != DVM_RESOURCE_DATA, plan))
    {
        memset(plan, 0, sizeof(*plan));
        return 0;
    }
    return 1;
}
