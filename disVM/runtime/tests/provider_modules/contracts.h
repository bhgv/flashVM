/* Copyright (c) 2026 bhgv
 * [GC:] EN: Actual compiler import tables and parsed owner exports feed native launch admission; no VM pointer reaches handler.
 * RU: Actual compiler import tables и parsed owner exports для native launch admission; handler не получает VM pointers.
 */
#ifndef PROVIDER_MODULE_CONTRACTS_H
#define PROVIDER_MODULE_CONTRACTS_H
#include "../../module_interface/contract.h"


/* [GC:] EN/RU: Exercise native metadata bounds and signed Import.sig bit preservation before real-image acceptance.
 * No inputs or escaped heap owners; explicit failure on invalid adapter result.
 * Проверить metadata bounds и сохранение signed Import.sig bits до real-image acceptance; без входов/escaped heap owners.
 */
static void interface_adapter_checks(void)
{
    Import imports[2] = {{0}};
    imports[0].name = "highbits";
    imports[0].sig = (-2147483647 - 1);
    DVM_LaunchContract *contract = dvm_module_contract_imports(imports);
    DVM_LaunchSymbol symbol;
    launch_check(contract && dvm_launch_contract_symbol(contract, 0, &symbol) &&
        symbol.signature == 0x80000000UL && !dvm_module_contract_imports(NULL) &&
        !dvm_module_contract_imports(H) && !dvm_module_contract_exports(NULL) &&
        !dvm_module_contract_exports(H), "actual Import.sig unsigned32 adapter contract");
    dvm_launch_contract_free(contract);
    Import *oversized = calloc(DVM_LAUNCH_SYMBOL_LIMIT + 1, sizeof(*oversized));
    launch_check(oversized != NULL, "bounded native import fixture");
    for (unsigned i = 0; i <= DVM_LAUNCH_SYMBOL_LIMIT; i++)
    {
        oversized[i].name = "unterminated";
    }
    launch_check(!dvm_module_contract_imports(oversized), "oversized native import table accepted");
    free(oversized);
}


/* [GC:] EN/RU: Borrow the real Value import table under the startup caller's Module pin; no allocation or retention.
 * Unused-interface variants return one static empty table; no pointer leaves the startup controller.
 * Заимствовать real Value imports при Module pin caller; без allocation/retention.
 * Неиспользуемый interface даёт static empty table; pointer не выходит из startup controller.
 */
static Import *required_imports(Module *module)
{
    static Import empty; /* Immutable zero sentinel for variants that never use a Value member. */
    if (module->ldt)
    {
        for (unsigned table = 0; module->ldt[table]; table++)
        {
            Import *imports = module->ldt[table];
            for (unsigned i = 0; imports[i].name; i++)
            {
                if (!strcmp(imports[i].name, "value"))
                {
                    return imports;
                }
            }
        }
    }
    launch_check(CONTROL == 10 || CONTROL == 13, "actual typed caller omitted Value imports");
    return &empty;
}


/* [GC:] EN/RU: Copy actual borrowed caller import metadata into an owned native contract; failure is explicit.
 * Скопировать actual borrowed caller imports в свой native contract; отказ явный.
 */
static DVM_LaunchContract *required_interface(Module *module)
{
    DVM_LaunchContract *contract = dvm_module_contract_imports(required_imports(module));
    launch_check(contract != NULL, "actual caller import contract");
    return contract;
}

/* [GC:] EN/RU: Parse actual source-built image on startup controller, snapshot exports, unload Module before native handler.
 * Image/label borrowed through return; no returned Module/Type/frame/code pointer, no guest bytecode executes.
 * Разобрать actual image на startup controller, снять exports, unload Module до native handler.
 * Image/label borrowed до return; Module/Type/frame/code pointer не выходит, guest bytecode не исполняется.
 */
static DVM_LaunchContract *image_interface(Image *image, char *label)
{
    Dir metadata = {0};
    metadata.length = image->size;
    metadata.mode = DVM_MODULE_NO_CACHE;
    metadata.qid.type = DVM_MODULE_UNCACHEABLE_QID;
    Module *parsed = parsemod(label, image->bytes, (ulong)image->size, &metadata);
    launch_check(parsed != NULL && parsed != H, "parse actual owner interface image");
    DVM_LaunchContract *contract = dvm_module_contract_exports(parsed);
    unload(parsed);
    launch_check(contract != NULL, "actual owner export contract");
    return contract;
}
#endif
