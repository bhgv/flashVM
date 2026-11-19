/* Copyright (c) 2026 bhgv
 * EN: Explicit owner-local Module contract checks and ordinary Inferno linking, independent of load names.
 * RU: Явные owner-local проверки контракта Module и обычный Inferno linking, независимо от load names.
 */
#ifndef DVM_MODULE_INTERFACE_BINDING_H
#define DVM_MODULE_INTERFACE_BINDING_H
#include "contract.h"
#include "environment.h"
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


/* [GC:] EN/RU: Original typed linking plus retained per-instance prepared provider-side namespace; owned link/H.
 * Consume local Module on all outcomes; borrow imports/required/group through return/exception.
 * group must be separately prepared for this instance before ACTIVE admission; linking performs no remote I/O.
 * Environment attachment uses native allocation only; failure destroys the unexposed link and keeps caller's group pin.
 * FD0/1/2 and terminal capabilities remain the caller's actual inherited objects, including shell redirections.
 * Typed linking с per-instance prepared namespace; свой link/H. Module поглотить всегда; прочие inputs borrowed.
 * Group готовится у провайдера до ACTIVE admission. Нет remote I/O; FD/terminal не подменяются.
 */
struct Modlink *dvm_module_link_prepared(struct Module *module, struct Import *imports, int mkmp,
    const DVM_LaunchContract *required, DVM_Namespace *group);


/* [GC:] EN/RU: Typed owner-local linking with per-instance prepared VFS and immutable accepted launch context; owned link/H.
 * Consume local Module on all outcomes; imports/group/context borrowed through return/exception.
 * A non-NULL typed context supplies the required contract; validate it before original linking or MP setup.
 * Bind group and metadata together before publication. No remote I/O, init invocation or automatic FD/terminal replacement.
 * Caller prepares context from the accepted handoff outside managed admission and retains inputs until return.
 * Typed owner-local linking с per-instance VFS/context; свой link/H. Module поглотить всегда, inputs borrowed.
 * Typed context задаёт required contract; проверка до original linking. Без remote I/O/init/подмены FD.
 */
struct Modlink *dvm_module_link_launch(struct Module *module, struct Import *imports, int mkmp,
    DVM_Namespace *group, struct DVM_LaunchContext *context);
#endif
