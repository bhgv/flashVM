/* Copyright (c) 2026 bhgv
 * EN: Explicit owner-local Module contract checks and ordinary Inferno linking, independent of load names.
 * RU: Явные owner-local проверки контракта Module и обычный Inferno linking, независимо от load names.
 */
#ifndef DVM_MODULE_INTERFACE_BINDING_H
#define DVM_MODULE_INTERFACE_BINDING_H
#include "contract.h"
struct Modlink;


/* [GC:] EN/RU: Consume owned local module, return same owner/NULL after checking all required exports; no lookup or execution.
 * Borrow required through return, NULL requirements refuse without downgrading to untyped. On mismatch/OOM/invalid,
 * unload valid input and set native error. Actual Module owns the implementation; names are not object identity.
 * ACTIVE owner/controller only, no foreign Module/managed pointers or provider calls; caller owns non-NULL result.
 * Поглотить owned local Module; вернуть тот же owner/NULL после проверки всех required exports, без lookup/execution.
 * required borrowed до return; NULL requirements отказывают, не выбирают untyped. При mismatch/OOM/invalid
 * выполнить unload valid input и задать error. ACTIVE owner/controller; без foreign pointers/provider calls.
 */
struct Module *dvm_module_check_contract(struct Module *module, const DVM_LaunchContract *required);


/* [GC:] EN/RU: Consume owned local module on every outcome; check complete requirements, then call ordinary Inferno linkmod.
 * Borrow required and valid sentinel imports through return/exception; imports must stay owner/root protected across GC.
 * mkmp has the original linkmod meaning (create independent module data when nonzero). Return owned Modlink/H;
 * mismatch/OOM/invalid inputs refuse before MP initialization; linker allocation/errors retain original cleanup semantics.
 * ACTIVE owner/controller only. No lookup, code transfer, guest invocation, authority inference or cross-heap pointers.
 * Поглотить owned local Module при любом исходе; проверить полный required contract, затем обычный linkmod.
 * required/imports borrowed до return/exception; imports защищены owner/root при GC. mkmp сохраняет смысл linkmod.
 * Свой Modlink/H; refusal до MP initialization, дальнейшие cleanup/errors — штатные Inferno. Без lookup/transfer/execution.
 */
struct Modlink *dvm_module_link_contract(struct Module *module, struct Import *imports, int mkmp,
    const DVM_LaunchContract *required);
#endif
