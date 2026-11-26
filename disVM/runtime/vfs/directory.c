/* Copyright (c) 2026 bhgv
 * EN: Ordinary directory cursor augmented by exact child namespace bindings, preserving siblings.
 * RU: Обычный directory cursor с точными child namespace bindings, сохраняющий соседние файлы.
 */
#include "directory_internal.h"
#include "cancel.h"
#include "path_cancel.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: Native cursor pins its original directory and namespace; no guest pointers.
 * Native cursor удерживает исходный directory и namespace; без guest pointers. */
typedef struct Directory
{
    DVM_VfsHandle inner; /* Owned original native/provider/union directory cursor. */
    DVM_Namespace *group; /* Owned namespace reference through close. */
    char parent[DVM_NS_PATH_LIMIT]; /* Original virtual directory spelling, not a host path. */
    DirectoryName *names; /* Owned unique child binding spellings captured on first enumeration read. */
    int (*visible)(const char *name); /* Optional pure basename predicate for the lifetime of the cursor. */
    size_t count; /* Captured immediate child names. */
    int initialized; /* Child names captured at first read, refreshed after successful rewind. */
    int emitted; /* Child bindings already emitted once for this enumeration. */
    int failed; /* Failed allocation/publication cannot silently skip a consumed batch. */
} Directory;


/* EN/RU: Query retained original directory metadata; never reopen its mutable pathname.
 * Получить metadata удержанного исходного directory без повторного открытия pathname. */
static FVM_Result info(void *state, FVM_FileInfo *out)
{
    return dvm_vfs_info(&((Directory *)state)->inner, out);
}


/* EN/RU: Check whether an underlying name is replaced/hidden by a captured child binding.
 * Проверить замену/скрытие underlying name захваченным child binding. */
static int shadow(Directory *cursor, const char *name)
{
    for (size_t i = 0; i < cursor->count; i++)
    {
        if (!strcmp(strrchr(cursor->names[i].path, '/')+1, name))
        {
            return 1;
        }
    }
    return 0;
}


/* EN/RU: Produce one owned batch; children use current metadata/visibility, other names pass unchanged.
 * Вернуть свой batch; children используют current metadata/visibility, остальные имена неизменны. */
static FVM_Result batch(Directory *cursor, FVM_FileInfo **entries, size_t *count, int *progress)
{
    FVM_FileInfo *base = NULL;
    size_t base_count = 0;
    FVM_Result status = dvm_vfs_list(&cursor->inner, &base, &base_count);
    if (status != FVM_OK)
    {
        return status;
    }
    *progress = base_count != 0;
    size_t extra = cursor->emitted ? 0 : cursor->count;
    if (extra > (size_t)-1/sizeof(**entries) || base_count > (size_t)-1/sizeof(**entries)-extra)
    {
        fvm_port_directory_entries_free(base);
        return FVM_OUT_OF_MEMORY;
    }
    size_t capacity = base_count+extra;
    FVM_FileInfo *out = capacity ? calloc(capacity, sizeof(*out)) : NULL;
    if (capacity && !out)
    {
        fvm_port_directory_entries_free(base);
        return FVM_OUT_OF_MEMORY;
    }
    size_t used = 0;
    if (!cursor->emitted)
    {
        for (size_t i = 0; i < cursor->count; i++)
        {
            if (dvm_cancel_requested(dvm_cancel_current()))
            {
                break;
            }
            const char *name = strrchr(cursor->names[i].path, '/')+1;
            if (cursor->visible && !cursor->visible(name))
            {
                continue;
            }
            DVM_ResourcePlan child;
            if (dvm_resource_plan(cursor->group, cursor->names[i].path, 0, &child))
            {
                if (dvm_cancel_current() && !dvm_path_cancellable(&child))
                {
                    dvm_resource_close(&child);
                    free(out);
                    fvm_port_directory_entries_free(base);
                    return FVM_UNSUPPORTED;
                }
                if (dvm_resource_info(&child, &out[used]) == FVM_OK)
                {
                    strcpy(out[used++].name, name);
                }
                dvm_resource_close(&child);
            }
        }
    }
    for (size_t i = 0; i < base_count; i++)
    {
        if ((!cursor->visible || cursor->visible(base[i].name)) && !shadow(cursor, base[i].name))
        {
            out[used++] = base[i];
        }
    }
    fvm_port_directory_entries_free(base);
    cursor->emitted = 1;
    if (used)
    {
        *entries = out;
        *count = used;
    }
    else
    {
        free(out);
    }
    return !used && dvm_cancel_requested(dvm_cancel_current()) ? FVM_ERROR : FVM_OK;
}


/* EN/RU: Skip fully shadowed batches with bounded work; real EOF only after underlying EOF.
 * Пропустить полностью shadowed batches с bounded work; EOF только после underlying EOF. */
static FVM_Result list(void *state, FVM_FileInfo **entries, size_t *count)
{
    Directory *cursor = state;
    *entries = NULL;
    *count = 0;
    if (cursor->failed)
    {
        return FVM_ERROR;
    }
    if (!cursor->initialized)
    {
        if (!dvm_directory_names(cursor->group, cursor->parent, &cursor->names, &cursor->count))
        {
            return FVM_ERROR;
        }
        cursor->initialized = 1;
    }
    for (unsigned i = 0; i < 256; i++)
    {
        if (dvm_cancel_requested(dvm_cancel_current()))
        {
            cursor->failed = 1;
            return FVM_ERROR;
        }
        int progress = 0;
        FVM_Result status = batch(cursor, entries, count, &progress);
        if (status != FVM_OK)
        {
            cursor->failed = 1;
            return status;
        }
        if (dvm_cancel_requested(dvm_cancel_current()))
        {
            cursor->failed = 1;
            dvm_vfs_abort_list(&cursor->inner);
        }
        if (*count || !progress)
        {
            return FVM_OK;
        }
    }
    cursor->failed = 1;
    return FVM_ERROR;
}


/* EN/RU: Preserve underlying rewind support; reset projected names only after successful seek.
 * Сохранить underlying rewind support; reset projected names только после успешного seek. */
static FVM_Result seek(void *state, long offset, int origin, long *position)
{
    Directory *cursor = state;
    if (offset || origin != FVM_FILE_SEEK_START || !position || cursor->failed)
    {
        return FVM_UNSUPPORTED;
    }
    FVM_Result status = dvm_vfs_seek(&cursor->inner, offset, origin, position);
    if (status == FVM_OK)
    {
        free(cursor->names);
        cursor->names = NULL;
        cursor->count = 0;
        cursor->emitted = cursor->initialized = 0;
    }
    return status;
}


/* EN/RU: Mark consumed stream unusable after failed managed result publication.
 * Пометить consumed stream unusable после failed managed result publication. */
static void abort_list(void *state)
{
    Directory *cursor = state;
    cursor->failed = 1;
    dvm_vfs_abort_list(&cursor->inner);
}


/* EN/RU: Consume original stream before namespace/name pins; no guest or provider pointer escape.
 * Поглотить original stream до namespace/name pins; без guest/provider pointer escape. */
static void close_directory(void *state)
{
    Directory *cursor = state;
    dvm_vfs_close(&cursor->inner);
    dvm_namespace_release(cursor->group);
    free(cursor->names);
    free(cursor);
}


/* EN/RU: Query owned substreams only after every borrower retires; never inspect live routes or do I/O.
 * Проверить owned substreams после ухода borrowers; без live route lookup и I/O. */
static int close_ready(void *state)
{
    return dvm_vfs_close_cancellable(&((Directory *)state)->inner);
}
static const DVM_VfsOps operations = {info, NULL, NULL, seek, list, close_directory, abort_list}; /* Ordinary directory projection preserves supported base rewind. */


/* EN/RU: See resource.h. / См. resource.h. */
FVM_Result dvm_namespace_directory_filtered(DVM_ResourcePlan *plan,
    int (*visible)(const char *name), DVM_VfsHandle *out)
{
    if (!plan || !plan->group || dvm_union_select(plan, 0) != FVM_OK || !plan->directory)
    {
        return FVM_ERROR;
    }
    Directory *cursor = calloc(1, sizeof(*cursor));
    if (!cursor)
    {
        return FVM_OUT_OF_MEMORY;
    }
    FVM_Result status = FVM_ERROR;
    if (!dvm_namespace_retain(plan->group))
    {
        free(cursor);
        return status;
    }
    cursor->group = plan->group;
    cursor->visible = visible;
    strcpy(cursor->parent, plan->names.path);
    if (plan->union_root)
    {
        status = dvm_union_directory(plan, &cursor->inner);
    }
    else if (plan->provider)
    {
        status = dvm_provider_open(plan->provider, plan->relative, "r", &cursor->inner);
    }
    else
    {
        FVM_PortDirectory *directory = NULL;
        status = plan->anchor ? fvm_port_directory_open_at(dvm_anchor_port(plan->anchor), plan->relative, &directory) :
            fvm_port_directory_open(plan->names.native, &directory);
        if (status == FVM_OK)
        {
            dvm_vfs_adopt(&cursor->inner, NULL, directory);
        }
    }
    if (status == FVM_OK && !cursor->inner.ops->list)
    {
        status = FVM_ERROR;
    }
    if (status != FVM_OK)
    {
        close_directory(cursor);
        return status;
    }
    out->native_flags = cursor->inner.native_flags & DVM_VFS_CANCEL_INFO;
    if ((cursor->inner.native_flags & DVM_VFS_CANCEL_LIST) &&
        dvm_directory_cancel_ready(cursor->group, cursor->parent))
    {
        out->native_flags |= DVM_VFS_CANCEL_LIST;
    }
    out->close_ready = close_ready;
    out->ops = &operations;
    out->state = cursor;
    return FVM_OK;
}


/* EN: Preserve the ordinary unfiltered directory on local Sys and existing routes.
 * RU: Сохранить обычный список каталога для локального Sys и прежних маршрутов. */
FVM_Result dvm_namespace_directory(DVM_ResourcePlan *plan, DVM_VfsHandle *out)
{
    return dvm_namespace_directory_filtered(plan, NULL, out);
}


/* EN/RU: See directory_internal.h; expose no underlying provider or union authority.
 * См. directory_internal.h; underlying provider/union authority не выдаётся. */
FVM_PortDirectory *dvm_directory_native(DVM_VfsHandle *handle)
{
    if (!handle || handle->ops != &operations || !handle->state)
    {
        return NULL;
    }
    Directory *cursor = handle->state;
    return dvm_vfs_directory(&cursor->inner);
}
