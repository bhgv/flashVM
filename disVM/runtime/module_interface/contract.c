/* Copyright (c) 2026 bhgv
 * [GC:] EN: Snapshot only actual name/signature metadata, never executable frames or managed storage.
 * RU: Snapshot только actual name/signature metadata, без executable frames и managed storage.
 */
#include "lib9.h"
#include "dat.h"
#include "isa.h"
#include "interp.h"
#include "contract.h"
#include <stdint.h>
#include <stdlib.h>


/* [GC:] EN/RU: See contract.h; bounded scan requires valid parsed sentinel storage, not an untrusted native pointer.
 * См. contract.h; bounded scan требует valid parsed sentinel storage, не untrusted native pointer.
 */
DVM_LaunchContract *dvm_module_contract_imports(const Import *imports)
{
    if (!imports || imports == H)
    {
        return NULL;
    }
    size_t count = 0;
    while (count < DVM_LAUNCH_SYMBOL_LIMIT && imports[count].name)
    {
        count++;
    }
    if (imports[count].name)
    {
        return NULL;
    }
    DVM_LaunchSymbol *symbols = count ? calloc(count, sizeof(*symbols)) : NULL;
    if (count && !symbols)
    {
        return NULL;
    }
    for (size_t i = 0; i < count; i++)
    {
        symbols[i].name = imports[i].name;
        symbols[i].signature = (uint32_t)imports[i].sig;
    }
    DVM_LaunchContract *contract = dvm_launch_contract_create(symbols, count);
    free(symbols);
    return contract;
}


/* [GC:] EN/RU: See contract.h; caller protects the module while native allocation copies borrowed export names.
 * См. contract.h; caller защищает module, пока native allocation копирует borrowed export names.
 */
DVM_LaunchContract *dvm_module_contract_exports(const Module *module)
{
    if (!module || module == H)
    {
        return NULL;
    }
    size_t count = 0;
    while (module->ext && count < DVM_LAUNCH_SYMBOL_LIMIT && module->ext[count].name)
    {
        count++;
    }
    if (module->ext && module->ext[count].name)
    {
        return NULL;
    }
    DVM_LaunchSymbol *symbols = count ? calloc(count, sizeof(*symbols)) : NULL;
    if (count && !symbols)
    {
        return NULL;
    }
    for (size_t i = 0; i < count; i++)
    {
        symbols[i].name = module->ext[i].name;
        symbols[i].signature = (uint32_t)module->ext[i].sig;
    }
    DVM_LaunchContract *contract = dvm_launch_contract_create(symbols, count);
    free(symbols);
    return contract;
}
