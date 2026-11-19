/* Copyright (c) 2026 bhgv
 * [GC:] EN: Explicit local implementation ownership crosses into standard Inferno linking only after the full contract matches.
 * RU: Явное владение local implementation переходит в штатный Inferno linking лишь после полного совпадения контракта.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "kernel.h"
#include "binding.h"


/* [GC:] EN/RU: See binding.h; only native snapshot allocation precedes the original linker's root/MP transaction.
 * См. binding.h; до штатной root/MP transaction выполняется лишь native snapshot allocation.
 */
Module *dvm_module_check_contract(Module *module, const DVM_LaunchContract *required)
{
    if (!module || module == H)
    {
        kwerrstr("module contract requires a loaded local implementation");
        return NULL;
    }
    DVM_LaunchContract *exports = required ? dvm_module_contract_exports(module) : NULL;
    int matched = exports && dvm_launch_contract_matches(required, exports);
    dvm_launch_contract_free(exports);
    if (!matched)
    {
        unload(module);
        kwerrstr("launch target interface changed or cannot be checked");
        return NULL;
    }
    return module;
}


/* [GC:] EN/RU: See binding.h; no native snapshot survives a possible linker exception or managed allocation.
 * См. binding.h; native snapshot не переживает possible linker exception/managed allocation.
 */
Modlink *dvm_module_link_contract(Module *module, Import *imports, int mkmp, const DVM_LaunchContract *required)
{
    if (!imports || imports == H)
    {
        if (module && module != H)
        {
            unload(module);
        }
        kwerrstr("module contract requires an import table");
        return H;
    }
    module = dvm_module_check_contract(module, required);
    return module ? linkmod(module, imports, mkmp) : H;
}


/* [GC:] EN/RU: See binding.h; the returned link is private until native attachment completes, without another managed allocation.
 * См. binding.h; link остаётся private до native attachment, без дополнительной managed allocation.
 */
Modlink *dvm_module_link_prepared(Module *module, Import *imports, int mkmp,
    const DVM_LaunchContract *required, DVM_Namespace *group)
{
    if (!group)
    {
        if (module && module != H)
        {
            unload(module);
        }
        kwerrstr("module binding requires a prepared namespace");
        return H;
    }
    Modlink *link = dvm_module_link_contract(module, imports, mkmp, required);
    if (link != H && !dvm_module_environment_bind(link, group))
    {
        destroy(link);
        kwerrstr("cannot attach module namespace");
        return H;
    }
    return link;
}
