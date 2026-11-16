/* Copyright (c) 2026 bhgv
 * EN: Union branch selection and independent merged directory cursors.
 * RU: Выбор union ветви и независимые объединённые directory cursors.
 */
#include "internal.h"
#include <stdlib.h>
#include <string.h>



/* EN/RU: Exchange one selected resource with root, keeping cleanup/list ownership.
 * Обменять selected resource с root, сохранив cleanup/list ownership. */
void dvm_union_choose(DVM_ResourcePlan *root, DVM_ResourcePlan *choice)
{
    if (root != choice)
    {
        DVM_ResourcePlan old = *root;
        root->names = choice->names;
        memcpy(root->relative, choice->relative, sizeof(root->relative));
        root->anchor = choice->anchor;
        root->provider = choice->provider;
        root->layer_id = choice->layer_id;
        root->mount_flags = choice->mount_flags;
        choice->names = old.names;
        memcpy(choice->relative, old.relative, sizeof(choice->relative));
        choice->anchor = old.anchor;
        choice->provider = old.provider;
        choice->layer_id = old.layer_id;
        choice->mount_flags = old.mount_flags;
    }
    root->selected = 1;
}


/* EN/RU: See resource.h; metadata lookup precedes open, never retry failed writes.
 * См. resource.h; metadata lookup до open; failed write не повторяется ниже. */
FVM_Result dvm_union_select(DVM_ResourcePlan *plan, int creating)
{
    DVM_ResourcePlan *choice, *create = NULL;
    FVM_FileInfo info;
    if (plan->selected)
    {
        return FVM_OK;
    }
    if ((plan->policy & DVM_NS_AUTORWX) && plan->requested_access)
    {
        return dvm_union_auto_select(plan, creating);
    }
    for (choice = plan; choice; choice = choice->layers)
    {
        if (!create && (!choice->layer_id || (choice->mount_flags & DVM_NS_CREATE)))
        {
            create = choice;
        }
        if (dvm_resource_raw_info(choice, &info) == FVM_OK)
        {
            dvm_union_choose(plan, choice);
            plan->directory = info.directory;
            if (!info.directory)
            {
                plan->union_root = 0;
            }
            return FVM_OK;
        }
    }
    if (creating && create)
    {
        dvm_union_choose(plan, create);
        return FVM_OK;
    }
    return FVM_ERROR;
}


/* EN/RU: Preserve output on lookup refusal; selected metadata belongs to one branch.
 * Сохранить output при отказе; metadata относится к одной selected ветви. */
FVM_Result dvm_union_info(DVM_ResourcePlan *plan, FVM_FileInfo *output)
{
    FVM_Result status = dvm_union_select(plan, 0);
    return status == FVM_OK ? dvm_resource_raw_info(plan, output) : status;
}


/* EN/RU: Retain the mount head's namespace identity and one opened directory.
 * The current branch cursor follows Inferno unionread's index/umc behavior.
 * Удержать namespace identity mount head и открытый directory; current cursor
 * следует index/umc семантике Inferno unionread. */
typedef struct UnionCursor
{
    DVM_Namespace *group; /* Retained original namespace, even after caller FORKNS. */
    char on[DVM_NS_PATH_LIMIT]; /* Original mount point spelling within this group. */
    uint64_t head_id; /* Old head must not attach to a newly created mount. */
    size_t index; /* Current unionread branch number. */
    DVM_VfsHandle header; /* Opened first directory supplies stable fstat identity. */
    DVM_VfsHandle current; /* Lazily opened branch stream, independently serialized. */
    int failed; /* Publication failure forbids subsequent batches. */
} UnionCursor;


/* EN/RU: Open one retained directory selection; no managed or namespace access.
 * Открыть один retained directory selection; без managed/namespace access. */
static FVM_Result open_directory(DVM_ResourcePlan *choice, DVM_VfsHandle *output)
{
    FVM_PortDirectory *directory = NULL;
    FVM_Result status;
    if (choice->provider)
    {
        status = dvm_provider_open(choice->provider, choice->relative, "r", output);
    }
    else
    {
        status = choice->anchor ? fvm_port_directory_open_at(dvm_anchor_port(choice->anchor), choice->relative, &directory)
                                : fvm_port_directory_open(choice->names.native, &directory);
        if (status == FVM_OK)
        {
            dvm_vfs_adopt(output, NULL, directory);
        }
    }
    if (status == FVM_OK && !output->ops->list)
    {
        dvm_vfs_close(output);
        return FVM_UNSUPPORTED;
    }
    return status;
}


/* EN/RU: Borrow cursor/output; fstat remains tied to the originally opened node.
 * Заимствовать cursor/output; fstat относится к первоначально открытому node. */
static FVM_Result union_info(void *state, FVM_FileInfo *output)
{
    UnionCursor *cursor = state;
    return dvm_vfs_info(&cursor->header, output);
}


/* EN/RU: Follow current mount-head order by branch index, skipping failed/EOF
 * streams and preserving an already opened current stream as Inferno unionread.
 * Следовать current mount-head order, пропускать failed/EOF streams, сохраняя
 * уже открытый current stream как Inferno unionread. Duplicate names are kept. */
static FVM_Result union_list(void *state, FVM_FileInfo **entries, size_t *count)
{
    UnionCursor *cursor = state;
    size_t limit = dvm_namespace_layer_count(cursor->group, cursor->on, cursor->head_id);
    if (cursor->failed || !entries || !count)
    {
        return FVM_ERROR;
    }
    *entries = NULL;
    *count = 0;
    while (cursor->index < limit)
    {
        DVM_ResourcePlan choice = {0};
        FVM_Result status;
        if (!dvm_namespace_layer_at(cursor->group, cursor->on, cursor->head_id, cursor->index, &choice))
        {
            return FVM_OK;
        }
        status = cursor->current.ops ? FVM_OK : open_directory(&choice, &cursor->current);
        dvm_resource_close(&choice);
        if (status == FVM_OK)
        {
            status = dvm_vfs_list(&cursor->current, entries, count);
            if (status == FVM_OK && *count)
            {
                return FVM_OK;
            }
        }
        fvm_port_directory_entries_free(*entries);
        *entries = NULL;
        *count = 0;
        dvm_vfs_close(&cursor->current);
        cursor->index++;
    }
    return FVM_OK;
}


/* EN/RU: Rewind directory enumeration only; other seeks remain unsupported.
 * Сбросить directory enumeration; другие seeks остаются unsupported. */
static FVM_Result union_seek(void *state, long offset, int origin, long *position)
{
    UnionCursor *cursor = state;
    if (offset || origin != FVM_FILE_SEEK_START || !position || cursor->failed)
    {
        return FVM_UNSUPPORTED;
    }
    dvm_vfs_close(&cursor->current);
    cursor->index = 0;
    *position = 0;
    return FVM_OK;
}


/* EN/RU: Abort only native cursor metadata after failed managed publication.
 * Пометить только native cursor metadata failed после ошибки publication. */
static void union_abort(void *state)
{
    UnionCursor *cursor = state;
    cursor->failed = 1;
    dvm_vfs_abort_list(&cursor->current);
}


/* EN/RU: Close actual streams before dropping the namespace/mount-head lifetime.
 * Закрыть streams до освобождения namespace/mount-head lifetime. */
static void union_close(void *state)
{
    UnionCursor *cursor = state;
    dvm_vfs_close(&cursor->current);
    dvm_vfs_close(&cursor->header);
    dvm_namespace_release(cursor->group);
    free(cursor);
}
static const DVM_VfsOps union_ops = {union_info, NULL, NULL, union_seek, union_list, union_close, union_abort}; /* Native union directory adapter. */


/* EN/RU: See resource.h; first directory open must succeed, lower opens are lazy.
 * См. resource.h; первый directory open должен пройти, нижние открываются лениво. */
FVM_Result dvm_union_directory(DVM_ResourcePlan *plan, DVM_VfsHandle *output)
{
    UnionCursor *cursor = calloc(1, sizeof(*cursor));
    FVM_Result status;
    if (!cursor)
    {
        return FVM_OUT_OF_MEMORY;
    }
    if (!plan->group || !dvm_namespace_retain(plan->group))
    {
        free(cursor);
        return FVM_ERROR;
    }
    cursor->group = plan->group;
    cursor->head_id = plan->head_id;
    strcpy(cursor->on, plan->names.path);
    status = open_directory(plan, &cursor->header);
    if (status != FVM_OK)
    {
        union_close(cursor);
        return status;
    }
    output->ops = &union_ops;
    output->state = cursor;
    return FVM_OK;
}
