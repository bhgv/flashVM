/* Copyright (c) 2026 bhgv
 * EN: Directory projection overlays explicit nodes and filters every delegated child.
 * RU: Directory projection накладывает explicit nodes и фильтрует каждый delegated child.
 */
#include "view_internal.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: Return the first child component of a deeper explicit mapping.
 * Вернуть первый child component deeper explicit mapping;1/0, bounded output. */
static int child(const char *parent, const char *path, char *name)
{
    size_t length = strlen(parent);
    if (strncmp(parent, path, length) || (length > 1 && path[length] != '/'))
    {
        return 0;
    }
    const char *part = path + length;
    while (*part == '/')
    {
        part++;
    }
    if (!*part)
    {
        return 0;
    }
    const char *slash = strchr(part, '/');
    size_t count = slash ? (size_t)(slash - part) : strlen(part);
    memcpy(name, part, count);
    name[count] = 0;
    return 1;
}


/* EN/RU: Detect explicit shadowing so delegated batches never repeat overlay names.
 * Найти explicit shadowing, чтобы delegated batches не повторяли overlay names. */
static int explicit_name(DVM_View *view, const char *path, const char *name)
{
    char candidate[DVM_NS_PATH_LIMIT];
    for (DVM_ViewRule *rule = view->rules; rule; rule = rule->next)
    {
        if (child(path, rule->path, candidate) && !strcmp(candidate, name))
        {
            return 1;
        }
    }
    return 0;
}


/* EN/RU: Append visible child metadata once; unknown/denied children stay hidden.
 * Добавить visible child metadata один раз; unknown/denied children скрыты. */
static void append(DVM_View *view, const char *parent, const char *name, FVM_FileInfo *entries, size_t *count)
{
    char path[DVM_NS_PATH_LIMIT];
    if (!*name || strchr(name, '/') || !strcmp(name, ".") || !strcmp(name, "..") ||
        strlen(parent) + strlen(name) + 2 > sizeof(path))
    {
        return;
    }
    for (size_t index = 0; index < *count; index++)
    {
        if (!strcmp(entries[index].name, name))
        {
            return;
        }
    }
    strcpy(path, !strcmp(parent, "/") ? "" : parent);
    strcat(path, "/");
    strcat(path, name);
    if (dvm_view_info(view, path, &entries[*count]) == FVM_OK)
    {
        (*count)++;
    }
}


/* EN/RU: See internal header; allocation failure returns no partial batch.
 * См. internal header; allocation failure не возвращает partial batch. */
static FVM_Result batch(DVM_View *view, const char *path, DVM_VfsHandle *inner, int explicit_done,
                         FVM_FileInfo **entries, size_t *count, int *progress)
{
    FVM_FileInfo *base = NULL, *output;
    size_t base_count = 0, capacity;
    *entries = NULL;
    *count = 0;
    if (inner->ops)
    {
        FVM_Result status = dvm_vfs_list(inner, &base, &base_count);
        if (status != FVM_OK)
        {
            return status;
        }
    }
    if (base_count > (size_t)-1 / sizeof(*output) - view->count)
    {
        fvm_port_directory_entries_free(base);
        return FVM_OUT_OF_MEMORY;
    }
    *progress = base_count != 0;
    capacity = base_count + (explicit_done ? 0 : view->count);
    if (!capacity)
    {
        fvm_port_directory_entries_free(base);
        return FVM_OK;
    }
    output = calloc(capacity, sizeof(*output));
    if (!output)
    {
        fvm_port_directory_entries_free(base);
        return FVM_OUT_OF_MEMORY;
    }
    if (!explicit_done)
    {
        char name[DVM_NS_PATH_LIMIT];
        for (DVM_ViewRule *rule = view->rules; rule; rule = rule->next)
        {
            if (child(path, rule->path, name))
            {
                append(view, path, name, output, count);
            }
        }
    }
    for (size_t index = 0; index < base_count; index++)
    {
        if (!explicit_name(view, path, base[index].name))
        {
            append(view, path, base[index].name, output, count);
        }
    }
    fvm_port_directory_entries_free(base);
    if (*count)
    {
        *entries = output;
    }
    else
    {
        free(output);
    }
    return FVM_OK;
}


/* EN/RU: Skip fully hidden batches without reporting false directory EOF.
 * Не выдавать false EOF для полностью скрытого batch; bounded progress256 batches. */
FVM_Result dvm_view_list(DVM_View *view, const char *path, DVM_VfsHandle *inner, int explicit_done,
                         FVM_FileInfo **entries, size_t *count)
{
    for (unsigned attempt = 0; attempt < 256; attempt++)
    {
        int progress = 0;
        FVM_Result status = batch(view, path, inner, explicit_done, entries, count, &progress);
        if (status != FVM_OK || *count || !progress)
        {
            return status;
        }
        explicit_done = 1;
    }
    return FVM_ERROR;
}
