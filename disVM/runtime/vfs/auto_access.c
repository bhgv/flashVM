/* Copyright (c) 2026 bhgv
 * EN: Explicit MAUTORWX operation selection using effective backend permissions.
 * RU: Явный MAUTORWX selection по effective backend permissions операции.
 */
#include "internal.h"
#include "cancel.h"
#include <string.h>


/* EN/RU: Check borrowed branch-relative path/access without opening or effects.
 * Effective native identity/provider policy, never guessed low-nine mode bits.
 * Проверить path/access без open/эффектов, по effective identity/provider policy.
 * Native blocking outside managed admission and namespace locks; return port status. */
static FVM_Result permitted(DVM_ResourcePlan *choice, const char *relative, unsigned access)
{
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        return FVM_ERROR;
    }
    if (choice->provider)
    {
        return dvm_provider_access(choice->provider, relative, access);
    }
    return choice->anchor ? fvm_port_anchor_access_at(dvm_anchor_port(choice->anchor), relative, access)
                          : FVM_UNSUPPORTED;
}


/* EN/RU: Check the actual parent of branch-relative entry for write AND search.
 * No creation/open occurs; missing/non-directory parents refuse this branch.
 * Проверить actual parent на write И search; без create/open. Отсутствующий
 * parent либо не-каталог отвергает ветвь. Borrow choice through native callbacks. */
static FVM_Result creatable(DVM_ResourcePlan *choice)
{
    char parent[DVM_NS_PATH_LIMIT], *slash;
    FVM_FileInfo info;
    FVM_Result status;
    strcpy(parent, choice->relative);
    slash = strrchr(parent, '/');
    if (slash)
    {
        *slash = 0;
    }
    else
    {
        strcpy(parent, ".");
    }
    if (choice->provider)
    {
        status = dvm_provider_info(choice->provider, parent, &info);
    }
    else
    {
        status = choice->anchor ? fvm_port_anchor_info_at(dvm_anchor_port(choice->anchor), parent, &info)
                                : FVM_UNSUPPORTED;
    }
    return status != FVM_OK ? status : !info.directory ? FVM_ERROR :
        permitted(choice, parent, FVM_ACCESS_WRITE | FVM_ACCESS_EXEC);
}


/* EN/RU: See resource.h. Select only; actual I/O remains one ordinary attempt.
 * For create walk directories in stack order, including a missing top entry;
 * existing entries must also allow the requested access and be regular files.
 * См. resource.h. Только selection; I/O остаётся одной ordinary попыткой.
 * Create идёт по порядку каталогов, включая missing top; existing entry должен
 * разрешать access и быть не-каталогом. No MCREATE requirement in this opt-in mode. */
FVM_Result dvm_union_auto_select(DVM_ResourcePlan *plan, int creating)
{
    DVM_ResourcePlan *choice;
    FVM_FileInfo info;
    int cycle = 0; /* Preserve proven cycles at exhaustion, even in explicit access-based selection. */
    unsigned requested = plan->requested_access;
    if (!requested || (requested & ~FVM_ACCESS_MASK))
    {
        return FVM_INVALID_ARGUMENT;
    }
    for (choice = plan; choice; choice = choice->layers)
    {
        if (dvm_cancel_requested(dvm_cancel_current()))
        {
            return FVM_ERROR;
        }
        FVM_Result status = dvm_resource_raw_info(choice, &info);
        if (status == DVM_VFS_CYCLE)
        {
            cycle = 1;
            continue;
        }
        int exists = status == FVM_OK;
        if (dvm_cancel_requested(dvm_cancel_current()))
        {
            return FVM_ERROR;
        }
        if (creating)
        {
            status = creatable(choice);
            if (status == DVM_VFS_CYCLE)
            {
                cycle = 1;
            }
            if (status != FVM_OK || (exists && info.directory))
            {
                continue;
            }
        }
        if (exists)
        {
            if (!creating && info.directory && (requested & FVM_ACCESS_EXEC))
            {
                continue;
            }
            status = permitted(choice, choice->relative, requested | (info.directory ? FVM_ACCESS_EXEC : 0));
            if (status == DVM_VFS_CYCLE)
            {
                cycle = 1;
            }
            if (status != FVM_OK)
            {
                continue;
            }
        }
        else if (!creating)
        {
            continue;
        }
        dvm_union_choose(plan, choice);
        plan->directory = exists && info.directory;
        if (!plan->directory)
        {
            plan->union_root = 0;
        }
        return FVM_OK;
    }
    return cycle ? DVM_VFS_CYCLE : FVM_ERROR;
}
