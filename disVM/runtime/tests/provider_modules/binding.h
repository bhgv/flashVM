/* Copyright (c) 2026 bhgv
 * [GC:] EN: Explicit name-independent contract binding of actual parsed modules through the original Inferno linker.
 * RU: Явный name-independent contract binding actual parsed modules через исходный Inferno linker.
 */
#ifndef PROVIDER_MODULE_BINDING_H
#define PROVIDER_MODULE_BINDING_H
#include "../../module_interface/binding.h"
static __thread unsigned binding_allocation; /* Exact snapshot calloc refusal, active only around a selected public binding call. */
static __thread int binding_watching; /* Startup-controller-only observation interval, never active after guest publication. */
static __thread unsigned binding_links; /* Actual ordinary Inferno linkmod calls within this interval. */
static __thread unsigned binding_unloads; /* Direct consumed-input unloads before any linker is entered. */
static __thread Module *binding_input; /* Borrowed comparison identity only; never dereferenced after consumption. */


/* [GC:] EN/RU: Original Inferno linker consumes module and returns owned Modlink/H; imports borrowed, mkmp unchanged.
 * Исходный linker поглощает Module и возвращает свой Modlink/H; imports borrowed, mkmp не меняется.
 */
extern Modlink *__real_linkmod(Module *module, Import *imports, int mkmp);


/* [GC:] EN/RU: Original unload consumes one Module owner; no test recursion or extra reference is introduced.
 * Исходный unload поглощает один Module owner; без test recursion/дополнительного reference.
 */
extern void __real_unload(Module *module);


/* [GC:] EN/RU: Count actual linker entry without replacing its result, allocation, MP construction or exceptions.
 * Учесть actual linker entry без подмены result/allocation/MP construction/exceptions.
 */
Modlink *__wrap_linkmod(Module *module, Import *imports, int mkmp)
{
    if (binding_watching)
    {
        binding_links++;
    }
    return __real_linkmod(module, imports, mkmp);
}


/* [GC:] EN/RU: Count only direct input retirement under the controller's public-call observation; preserve production unload.
 * Учесть лишь direct input retirement в public-call interval controller; сохранить production unload.
 */
void __wrap_unload(Module *module)
{
    if (binding_watching && module == binding_input)
    {
        binding_unloads++;
    }
    __real_unload(module);
}


/* [GC:] EN/RU: Parse an actual image under an unrelated label and free its bytes; return owned local Module.
 * No request/path lookup or code execution; fixture caller consumes the result exactly once.
 * Разобрать actual image под unrelated label, освободить bytes; свой local Module, без lookup/execution.
 */
static Module *binding_image(const char *file)
{
    Image image = image_read(file);
    Dir metadata = {0};
    metadata.length = image.size;
    metadata.mode = DVM_MODULE_NO_CACHE;
    metadata.qid.type = DVM_MODULE_UNCACHEABLE_QID;
    Module *module = parsemod("/unrelated-alias.dis", image.bytes, (ulong)image.size, &metadata);
    free(image.bytes);
    launch_check(module && module != H, "parse explicit binding image");
    return module;
}


/* [GC:] EN/RU: Bind actual caller imports to the exact parsed local implementation; validate full contract before linker entry.
 * required/imports borrowed under caller's Module pin. Refusal consumes input once without entering linkmod.
 * Snapshot failures target each real calloc; success returns same Module and original entry frames/PCs.
 * Связать actual caller imports с точной parsed local implementation; полный contract проверяется до linkmod.
 * required/imports borrowed при Module pin caller. Refusal поглощает input один раз без linkmod.
 */
static void binding_attempt(const char *file, Import *imports, const DVM_LaunchContract *required,
    unsigned failure, int expected)
{
    Module *module = binding_image(file);
    binding_input = module;
    binding_links = 0;
    binding_unloads = 0;
    binding_allocation = failure;
    binding_watching = 1;
    Modlink *link = dvm_module_link_contract(module, imports, 1, required);
    binding_watching = 0;
    launch_check(!binding_allocation, "explicit binding snapshot refusal was not reached");
    if (expected)
    {
        launch_check(link != H && link->m == module && binding_links == 1 && !binding_unloads,
            "explicit binding replaced selected implementation or skipped original linker");
        unsigned count = 0;
        for (; imports[count].name; count++)
        {
            Link *entry = module->ext;
            while (entry && entry->name && strcmp(entry->name, imports[count].name))
            {
                entry++;
            }
            launch_check(entry && entry->name && entry->sig == imports[count].sig &&
                link->links[count].u.pc == entry->u.pc && link->links[count].frame == entry->frame,
                "explicit binding changed original Inferno entry frame or PC");
        }
        launch_check(link->nlinks == (int)count, "explicit binding changed import count");
        destroy(link); /* No managed allocation occurs between unrooted native result and this immediate consumption. */
    }
    else
    {
        launch_check(link == H && !binding_links && binding_unloads == 1,
            "refused explicit contract reached MP linking or lost input ownership");
    }
    binding_input = NULL;
}


/* [GC:] EN/RU: Exercise aliases, unused required exports, mismatch, invalid inputs, both snapshot allocations and recovery.
 * Caller module/required borrowed during startup, no Module/Import/Modlink pointer reaches native handler or wire.
 * Проверить aliases, unused required exports, mismatch, invalid inputs, оба snapshot allocations и recovery.
 */
static void interface_binding_checks(Module *caller, const DVM_LaunchContract *required)
{
    Import *imports = required_imports(caller);
    binding_attempt("app.dis", imports, required, 0, 1);
    if (dvm_launch_contract_count(required))
    {
        binding_attempt("wrong.dis", imports, required, 0, 0);
    }
    size_t count = dvm_launch_contract_count(required);
    DVM_LaunchSymbol *symbols = calloc(count + 1, sizeof(*symbols));
    launch_check(symbols != NULL, "full unused-member contract fixture");
    for (size_t i = 0; i < count; i++)
    {
        launch_check(dvm_launch_contract_symbol(required, i, &symbols[i]), "copy actual required symbol");
    }
    symbols[count].name = "required-but-not-imported";
    symbols[count].signature = 1;
    DVM_LaunchContract *full = dvm_launch_contract_create(symbols, count + 1);
    free(symbols);
    launch_check(full != NULL, "full required interface fixture");
    binding_attempt("app.dis", imports, full, 0, 0);
    dvm_launch_contract_free(full);
    binding_attempt("app.dis", imports, NULL, 0, 0);
    binding_attempt("app.dis", NULL, required, 0, 0);
    binding_attempt("app.dis", H, required, 0, 0);
    binding_attempt("app.dis", imports, required, 1, 0);
    binding_attempt("app.dis", imports, required, 2, 0);
    binding_attempt("app.dis", imports, required, 0, 1);
    launch_check(dvm_module_link_contract(NULL, imports, 1, required) == H &&
        dvm_module_link_contract(H, imports, 1, required) == H, "invalid module accepted by explicit binding");
}
#endif
