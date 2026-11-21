/* Copyright (c) 2026 bhgv
 * EN: Provider projection identity publication and request-local traversal errors.
 * RU: Публикация provider projection identity и request-local traversal errors.
 */
#include "provider_walk.h"
#include <stdlib.h>


/* EN/RU: See provider.h; caller exclusively owns an unpublished factory and retains that ownership through return.
 * См. provider.h; caller эксклюзивно владеет unpublished factory до return.
 */
int dvm_provider_walk_identity(DVM_Provider *provider, const WalkEntity *entity, unsigned long high, unsigned long low)
{
    if (!provider || !entity || high > 0xffffffffUL || low > 0xffffffffUL)
    {
        return 0;
    }
    WalkScope scope = {0};
    if (walk_enter(&scope, entity, high, low, WALK_INFO, ".", "") != WALK_OK)
    {
        return 0;
    }
    dvm_provider_walk_leave(&scope);
    fvm_port_sync_lock(provider->lock);
    int valid = provider->references == 1 && !provider->walk_used;
    if (valid)
    {
        provider->walk_entity = *entity;
        provider->walk_high = high;
        provider->walk_low = low;
    }
    fvm_port_sync_unlock(provider->lock);
    return valid;
}


/* EN/RU: See provider.h; immutable once a source callback can run, output belongs to native caller.
 * См. provider.h; immutable после допуска source callback, output принадлежит native caller.
 */
int dvm_provider_walk_get(DVM_Provider *provider, WalkEntity *entity)
{
    if (!provider || !entity)
    {
        return 0;
    }
    fvm_port_sync_lock(provider->lock);
    provider->walk_used = 1;
    *entity = provider->walk_entity;
    fvm_port_sync_unlock(provider->lock);
    return 1;
}


/* EN/RU: See provider_walk.h; immutable identity is read after sealing under the factory lifetime lock.
 * См. provider_walk.h; immutable identity читается после sealing под lifetime lock factory.
 */
FVM_Result dvm_provider_walk_enter(DVM_Provider *provider, const char *path, unsigned operation,
    const char *detail, WalkScope *scope)
{
    WalkEntity entity;
    if (!dvm_provider_walk_get(provider, &entity))
    {
        return FVM_INVALID_ARGUMENT;
    }
    int status = walk_enter(scope, &entity, provider->walk_high, provider->walk_low, operation, path, detail);
    return status == WALK_OK ? FVM_OK : status == WALK_CYCLE ? DVM_VFS_CYCLE :
        status == WALK_NOMEM ? FVM_OUT_OF_MEMORY : status == WALK_INVALID ? FVM_INVALID_ARGUMENT : FVM_ERROR;
}


/* EN/RU: See provider_walk.h; native callback contracts return normally before any managed reentry/unwind.
 * См. provider_walk.h; native callbacks возвращаются нормально до managed reentry/unwind.
 */
void dvm_provider_walk_leave(WalkScope *scope)
{
    if (!walk_leave(scope))
    {
        abort();
    }
}


/* EN/RU: See provider_walk.h; held identity distinguishes replacement objects opened at the same pathname.
 * См. provider_walk.h; held identity отличает replacement objects, открытые по одному pathname.
 */
FVM_Result dvm_vfs_walk_enter(const WalkEntity *entity, const char *path, unsigned operation,
    const char *detail, WalkScope *scope)
{
    int status = walk_enter(scope, entity, 0, 0, operation, path, detail);
    return status == WALK_OK ? FVM_OK : status == WALK_CYCLE ? DVM_VFS_CYCLE :
        status == WALK_NOMEM ? FVM_OUT_OF_MEMORY : status == WALK_INVALID ? FVM_INVALID_ARGUMENT : FVM_ERROR;
}


/* EN/RU: See provider.h. / См. provider.h. */
int dvm_provider_set_styx_caller(DVM_Provider *context, DVM_Provider *session, uint32_t pid)
{
    DVM_StyxCallerKey key;
    if (!context || context == session || !pid || !dvm_provider_walk_get(session, &key.session))
    {
        return 0;
    }
    key.pid = pid;
    fvm_port_sync_lock(context->lock);
    int installed = context->references == 1 && !context->styx_caller.pid;
    if (installed)
    {
        context->styx_caller = key;
    }
    fvm_port_sync_unlock(context->lock);
    return installed;
}


/* EN/RU: See provider.h. / См. provider.h. */
int dvm_provider_styx_caller(DVM_Provider *context, DVM_StyxCallerKey *key)
{
    if (!context || !key)
    {
        return 0;
    }
    fvm_port_sync_lock(context->lock);
    int present = context->styx_caller.pid != 0;
    if (present)
    {
        *key = context->styx_caller;
    }
    fvm_port_sync_unlock(context->lock);
    return present;
}
