/* Copyright (c) 2026 bhgv
 * EN: Reserve executable VFS names without replacing existing namespace entries.
 * RU: Резервировать исполняемые имена VFS без замены существующих записей.
 */
#include "exec_private.h"
#include "styxrpc/named_channel.h"
#include <stdlib.h>
#include <string.h>

#define EXEC_MARKER_NAMESPACE UINT32_C(0x40000000) /* High word of an execute marker's virtual Qid. */

typedef struct ExecMarker
{
    uint32_t serial; /* Stable session-local Qid low word. */
    char name[]; /* Owned displayed basename. */
} ExecMarker;


/* EN: Describe one pinned marker. Only execute metadata is granted.
 * RU: Описать закреплённую метку. Выдать только право исполнения в метаданных. */
static FVM_Result marker_info(void *state, const char *path, FVM_FileInfo *out)
{
    if (strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    const ExecMarker *marker = state;
    memset(out, 0, sizeof(*out));
    strcpy(out->name, marker->name);
    out->mode = 0111;
    out->identity.high = EXEC_MARKER_NAMESPACE;
    out->identity.low = marker->serial;
    return FVM_OK;
}


/* EN: An executable marker has no ordinary data cursor.
 * RU: Исполняемая метка не открывает обычный дескриптор данных. */
static FVM_Result marker_open(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    (void)state;
    (void)path;
    (void)mode;
    (void)out;
    return FVM_UNSUPPORTED;
}


/* EN: Check traverse and write rights on the current parent before publication.
 * RU: Перед публикацией проверить права прохода и записи в текущий родительский каталог. */
static int parent_writable(DVM_Namespace *group, const char *path)
{
    char parent[STYXRPC_NODE_PATH_MAX + 2];
    size_t length = (size_t)(strrchr(path, '/') - path);
    if (!length)
    {
        length = 1;
    }
    memcpy(parent, path, length);
    parent[length] = 0;
    DVM_ResourcePlan plan;
    if (!dvm_resource_plan(group, parent, 0, &plan))
    {
        return 0;
    }
    unsigned access = FVM_ACCESS_WRITE | FVM_ACCESS_EXEC;
    plan.requested_access = access;
    FVM_Result status = dvm_union_select(&plan, 0);
    if (status == FVM_OK && plan.directory)
    {
        status = plan.provider ? dvm_provider_access(plan.provider, plan.relative, access) :
            plan.anchor ? fvm_port_anchor_access_at(dvm_anchor_port(plan.anchor), plan.relative, access) : FVM_UNSUPPORTED;
    }
    else
    {
        status = FVM_UNSUPPORTED;
    }
    dvm_resource_close(&plan);
    return status == FVM_OK;
}


/* EN/RU: See exec_private.h. / См. exec_private.h. */
int dvm_exec_claim(ExecOwner *owner, const char *absolute)
{
    static const DVM_ProviderOps ops = {.info = marker_info, .open = marker_open, .destroy = free};
    if (!styxrpc_named_path(absolute))
    {
        return 0;
    }
    const char *basename = strrchr(absolute, '/') + 1;
    size_t length = strlen(basename);
    if (length >= sizeof(((FVM_FileInfo *)0)->name))
    {
        return 0;
    }
    DVM_Namespace *group = owner->broker->group;
    DVM_NamespacePlan target;
    DVM_ResourcePlan plan;
    if (!dvm_namespace_plan(group, absolute, &target) || !parent_writable(group, absolute) ||
        !dvm_resource_plan(group, absolute, 0, &plan))
    {
        return 0;
    }
    FVM_FileInfo info;
    plan.absence_only = 1;
    FVM_Result status = dvm_resource_info(&plan, &info);
    dvm_resource_close(&plan);
    if (status != FVM_NOT_FOUND)
    {
        return 0;
    }
    ExecMarker *entry = malloc(sizeof(*entry) + length + 1);
    if (!entry)
    {
        return 0;
    }
    strcpy(entry->name, basename);
    pthread_mutex_lock(&owner->broker->lock);
    entry->serial = owner->broker->stopped || owner->broker->marker == UINT32_MAX ? 0 : ++owner->broker->marker;
    pthread_mutex_unlock(&owner->broker->lock);
    DVM_Provider *marker = entry->serial ? dvm_provider_create(&ops, entry) : NULL;
    if (!marker)
    {
        free(entry);
        return 0;
    }
    int mounted = dvm_namespace_mount_provider_id(group, &target, marker, "/styx-executable", &owner->layer) == 1;
    dvm_provider_release(marker);
    if (mounted)
    {
        strcpy(owner->path, absolute);
    }
    return mounted;
}


/* EN/RU: See exec_private.h. / См. exec_private.h. */
uint64_t dvm_exec_layer(DvmExecBroker *broker, const char *absolute)
{
    DVM_ResourcePlan plan;
    if (!dvm_resource_plan(broker->group, absolute, 0, &plan))
    {
        return 0;
    }
    FVM_FileInfo info;
    uint64_t id = dvm_resource_info(&plan, &info) == FVM_OK && !info.directory && (info.mode & 0111) ?
        plan.layer_id : 0;
    dvm_resource_close(&plan);
    return id;
}


/* EN/RU: See exec_private.h. / См. exec_private.h. */
void dvm_exec_unclaim(ExecOwner *owner)
{
    DVM_Namespace *group = owner->broker->group;
    DVM_NamespacePlan target;
    while (dvm_namespace_plan(group, owner->path, &target) &&
        !dvm_namespace_unmount_layer(group, &target, owner->layer))
    {
    }
}
