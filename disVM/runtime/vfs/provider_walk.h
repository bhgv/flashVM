/* Copyright (c) 2026 bhgv
 * EN: Shared native factory traversal scopes and VFS-specific error mapping.
 * RU: Общие native factory traversal scopes и VFS-specific error mapping.
 */
#ifndef DVM_PROVIDER_WALK_H
#define DVM_PROVIDER_WALK_H
#include "provider_internal.h"


/* EN/RU: Enter immutable factory projection/path state; return FVM_OK/error without invoking source callbacks.
 * provider/path/detail borrowed through matching leave; scope zeroed. No lifetime lock or managed access.
 * Войти в immutable factory projection/path state; FVM_OK/error без source callbacks; inputs borrowed до leave.
 */
FVM_Result dvm_provider_walk_enter(DVM_Provider *provider, const char *path, unsigned operation,
    const char *detail, WalkScope *scope);


/* EN/RU: Enter one immutable held-object state, mapping WALK errors to VFS status; inputs borrowed through leave.
 * Войти в immutable held-object state, преобразовать WALK errors в VFS status; inputs borrowed до leave.
 */
FVM_Result dvm_vfs_walk_enter(const WalkEntity *entity, const char *path, unsigned operation,
    const char *detail, WalkScope *scope);


/* EN/RU: Leave a successfully entered scope; abort on internal unbalanced ownership, never silently corrupt another search.
 * Выйти из successful scope; internal unbalanced ownership завершает работу, не портит другой search молча.
 */
void dvm_provider_walk_leave(WalkScope *scope);
#endif
