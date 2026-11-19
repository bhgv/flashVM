/* Copyright (c) 2026 bhgv
 * EN: Owned native snapshots of Inferno names/signatures; no frames, code pointers or wire representation.
 * RU: Native owned snapshots имён/сигнатур Inferno; без frames, code pointers и wire representation.
 */
#ifndef DVM_LAUNCH_CONTRACT_H
#define DVM_LAUNCH_CONTRACT_H
#include <stddef.h>
#define DVM_LAUNCH_SYMBOL_LIMIT 1024U /* Bounded number of interface members in a native request. */
#define DVM_LAUNCH_SYMBOL_NAME 256U /* UTF-8 bytes including terminating NUL per member. */
typedef struct DVM_LaunchContract DVM_LaunchContract; /* Exclusive immutable native owner, free after joining readers. */


/* EN/RU: Borrowed constructor/result view; signature is the exact unsigned32 Inferno link value, not authority.
 * Borrowed constructor/result view; signature — точное unsigned32 значение Inferno link, не authority.
 */
typedef struct DVM_LaunchSymbol
{
    const char *name; /* Nonempty terminated name; output borrows immutable contract storage. */
    unsigned long signature; /* 0..0xffffffffUL, independent of native WORD/pointer width. */
} DVM_LaunchSymbol;


/* EN/RU: Copy count borrowed symbols into sorted owned snapshot/NULL; NULL symbols allowed only for count0.
 * Reject oversized/empty names, duplicate names, wide signatures or OOM; never adopt inputs or execute code.
 * Скопировать count borrowed symbols в sorted owned snapshot/NULL; NULL symbols только при count0.
 * Отказать oversized/empty names, duplicate names, wide signatures/OOM; inputs не принимать, код не выполнять.
 */
DVM_LaunchContract *dvm_launch_contract_create(const DVM_LaunchSymbol *symbols, size_t count);


/* EN/RU: Copy borrowed live snapshot; owned result/NULL on NULL/OOM, no callbacks or managed access.
 * Скопировать borrowed live snapshot; свой result/NULL при NULL/OOM, без callbacks/managed access.
 */
DVM_LaunchContract *dvm_launch_contract_clone(const DVM_LaunchContract *contract);


/* EN/RU: Return member count of borrowed snapshot;0 for NULL, no allocation or side effects.
 * Вернуть member count borrowed snapshot;0 для NULL, без allocation/side effects.
 */
size_t dvm_launch_contract_count(const DVM_LaunchContract *contract);


/* EN/RU: Copy sorted member view to output;1 success/0 invalid index/input, output unchanged on failure.
 * Returned name borrows contract through reader completion; no ownership transfer or allocation.
 * Скопировать sorted member view в output;1/0 при invalid input/index, output неизменен при отказе.
 * Имя заимствует contract до завершения reader; без передачи ownership/allocation.
 */
int dvm_launch_contract_symbol(const DVM_LaunchContract *contract, size_t index, DVM_LaunchSymbol *output);


/* EN/RU: Require every requested name/signature in available; extra exports/order do not matter,1/0.
 * Both immutable snapshots borrowed through return; NULL refuses, empty non-NULL request matches any snapshot.
 * No code/ABI conversion, execution permission, authentication, allocation or I/O follows from a match.
 * Требовать все requested names/signatures в available; лишние exports/order не влияют,1/0.
 * Snapshots borrowed до return; NULL — отказ, пустой non-NULL request совместим с любым snapshot.
 * Совпадение не даёт code/ABI conversion, execute permission, authentication, allocation либо I/O.
 */
int dvm_launch_contract_matches(const DVM_LaunchContract *required, const DVM_LaunchContract *available);


/* EN/RU: Consume exclusive snapshot after readers join; NULL allowed, no callbacks/I/O/managed owners.
 * Поглотить exclusive snapshot после join readers; NULL допустим, без callbacks/I/O/managed owners.
 */
void dvm_launch_contract_free(DVM_LaunchContract *contract);
#endif
