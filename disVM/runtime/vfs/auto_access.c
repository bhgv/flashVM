/* Copyright (c) 2026 bhgv
 * EN: Explicit MAUTORWX operation selection using effective backend permissions.
 * RU: Явный MAUTORWX selection по effective backend permissions операции.
 */
#include "internal.h"
#include <string.h>


/* EN/RU: Check borrowed branch-relative path/access without opening or effects.
 * Effective native identity/provider policy, never guessed low-nine mode bits.
 * Проверить path/access без open/эффектов, по effective identity/provider policy.
 * Native blocking outside managed admission and namespace locks; return port status. */
static FVM_Result permitted(DVM_ResourcePlan *choice, const char *relative, unsigned access)
{
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
static int creatable(DVM_ResourcePlan *choice)
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
    return status == FVM_OK && info.directory &&
           permitted(choice, parent, FVM_ACCESS_WRITE | FVM_ACCESS_EXEC) == FVM_OK;
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
    unsigned requested = plan->requested_access;
    if (!requested || (requested & ~FVM_ACCESS_MASK))
    {
        return FVM_INVALID_ARGUMENT;
    }
    for (choice = plan; choice; choice = choice->layers)
    {
        int exists = dvm_resource_raw_info(choice, &info) == FVM_OK;
        if (creating)
        {
            if (!creatable(choice) || (exists && info.directory))
            {
                continue;
            }
            if (!exists || permitted(choice, choice->relative, requested) == FVM_OK)
            {
                dvm_union_choose(plan, choice);
                plan->union_root = 0;
                return FVM_OK;
            }
        }
        else if (exists && !(info.directory && (requested & FVM_ACCESS_EXEC)) &&
                 permitted(choice, choice->relative, requested | (info.directory ? FVM_ACCESS_EXEC : 0)) == FVM_OK)
        {
            dvm_union_choose(plan, choice);
            if (!info.directory)
            {
                plan->union_root = 0;
            }
            return FVM_OK;
        }
    }
    return FVM_ERROR;
}
