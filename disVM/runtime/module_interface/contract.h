/* Copyright (c) 2026 bhgv
 * EN: Copy actual Inferno import/export signatures into VM-independent native launch contracts.
 * RU: Копировать реальные Inferno import/export signatures в VM-independent native launch contracts.
 */
#ifndef DVM_MODULE_INTERFACE_CONTRACT_H
#define DVM_MODULE_INTERFACE_CONTRACT_H
#include "../vfs/launch/contract.h"
struct Import;
struct Module;


/* [GC:] EN/RU: Borrow a valid sentinel-terminated Import table under the caller's owner/module pin.
 * Return owned native contract/NULL on invalid/oversized/OOM; no heap pointer escapes, no guest code or I/O.
 * Native symbols include exact unsigned32 bits even when Import.sig is negative.
 * Заимствовать valid sentinel Import table при owner/module pin caller; свой contract/NULL при invalid/oversized/OOM.
 * Heap pointer не выходит, без guest/I/O; negative Import.sig сохраняет точные unsigned32 bits.
 */
DVM_LaunchContract *dvm_module_contract_imports(const struct Import *imports);


/* [GC:] EN/RU: Borrow a valid loaded Module under caller's native module pin; copy sentinel exports to owned contract/NULL.
 * No Module/Type/frame/code pointer survives; NULL ext means a valid empty export table. No guest code/I/O.
 * Заимствовать valid loaded Module при native module pin caller; sentinel exports в свой contract/NULL.
 * Module/Type/frame/code pointer не сохраняется; NULL ext — valid empty exports, без guest/I/O.
 */
DVM_LaunchContract *dvm_module_contract_exports(const struct Module *module);
#endif
