/* Copyright (c) 2026 bhgv
 * EN: Explicit ordered-stack editing on retained namespace plans.
 * RU: Явное редактирование ordered stack по retained namespace plans.
 */
#ifndef DVM_VFS_STACK_H
#define DVM_VFS_STACK_H
#include "resource.h"
#define DVM_NS_INSERT 16U /* Explicit Vfsctl MINS, not a standard Sys.bind flag. */


/* EN/RU: Resolve signed position: zero top, -1 bottom; insertion addresses slots.
 * Разрешить signed position: zero top, -1 bottom; insertion адресует slots.
 * Return1/0; handles INT64_MIN without overflow, no allocation/I/O. */
int dvm_stack_position(int64_t position, size_t count, int insertion, size_t *output);


/* EN/RU: Commit prepared exact-path stack from retained source/target plans.
 * flags: REPL/BEFORE/AFTER/INSERT plus CREATE/AUTORWX; index used only by INSERT.
 * Native-only, may query/pin native directories outside locks. 1 success,0 stale,-1 error.
 * Применить exact-path stack: flags order/create/access, index только для INSERT.
 * Удержанные plans borrowed; native I/O вне locks, 1/0/-1 success/stale/error. */
int dvm_stack_bind(DVM_ResourcePlan *source, DVM_ResourcePlan *target, unsigned flags, int64_t index);


/* EN/RU: Move exact layer ID to final signed position at expected revision.
 * Child paths materialize an independent effective stack, leaving parent/siblings alone.
 * Native-only I/O outside locks; 1/0/-1 success/stale/error. Plans stay borrowed.
 * Переставить layer ID по revision; child path получает отдельный stack.
 * Parent/siblings не меняются; 1/0/-1 success/stale/error, plans borrowed. */
int dvm_stack_move(DVM_ResourcePlan *target, uint64_t revision, uint64_t layer, int64_t index);


/* EN/RU: Check config revision under its namespace mutex, no I/O or allocation.
 * Проверить config revision под namespace mutex, без I/O/allocation. */
int dvm_stack_current(DVM_Namespace *group, uint64_t revision);
#endif
