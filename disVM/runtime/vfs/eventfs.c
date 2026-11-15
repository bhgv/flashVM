/* Copyright (c) 2026 bhgv
 * EN: Explicit event projection factory, root directory and source read policy.
 * RU: Явная event projection factory, root directory и source read policy.
 */
#include "eventfs_internal.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: Factory and each root cursor own an independent source reference.
 * Factory и каждый root cursor владеют отдельной source reference. */
typedef struct EventRoot
{
    DVM_Provider *source; /* Retained source factory, never a guest pointer. */
    int listed; /* This root cursor already emitted its sole directory entry. */
} EventRoot;


/* EN/RU: See eventfs_internal.h; no fallback to mode-bit guesses or native paths.
 * См. eventfs_internal.h; без fallback к mode bits либо native paths. */
FVM_Result dvm_eventfs_allowed(DVM_Provider *source)
{
    return dvm_provider_access(source, ".", FVM_ACCESS_READ);
}


/* EN/RU: See eventfs_internal.h; synthetic names never disclose source paths.
 * См. eventfs_internal.h; synthetic names не раскрывают source paths. */
FVM_Result dvm_eventfs_info(DVM_Provider *source, const char *path, FVM_FileInfo *output)
{
    int directory = !strcmp(path, ".");
    FVM_Result status;
    if (!output || (!directory && strcmp(path, "events")))
    {
        return FVM_NOT_FOUND;
    }
    status = dvm_eventfs_allowed(source);
    if (status != FVM_OK)
    {
        return status;
    }
    memset(output, 0, sizeof(*output));
    strcpy(output->name, directory ? "." : "events");
    output->directory = directory;
    output->mode = directory ? 0555 : 0666;
    output->identity.low = directory ? 0 : 1;
    return FVM_OK;
}


/* EN/RU: Forward factory metadata with its retained source capability.
 * Передать factory metadata с удержанным source capability. */
static FVM_Result factory_info(void *state, const char *path, FVM_FileInfo *output)
{
    EventRoot *root = state;
    return dvm_eventfs_info(root->source, path, output);
}


/* EN/RU: Check projection operation; write controls observers, not source files.
 * Проверить projection operation; write меняет observer, не source files. */
static FVM_Result access_check(void *state, const char *path, unsigned access)
{
    EventRoot *root = state;
    if (!strcmp(path, "."))
    {
        if (access & FVM_ACCESS_WRITE)
        {
            return FVM_ERROR;
        }
    }
    else if (strcmp(path, "events") || (access & FVM_ACCESS_EXEC))
    {
        return FVM_ERROR;
    }
    return dvm_eventfs_allowed(root->source);
}


/* EN/RU: Return opened root metadata, independently from namespace spelling.
 * Вернуть opened root metadata, независимо от namespace spelling. */
static FVM_Result root_info(void *state, FVM_FileInfo *output)
{
    EventRoot *root = state;
    return dvm_eventfs_info(root->source, ".", output);
}


/* EN/RU: List the only synthetic entry once; allocation/refusal never consumes it.
 * Выдать единственный entry один раз; allocation/refusal не поглощает его. */
static FVM_Result root_list(void *state, FVM_FileInfo **entries, size_t *count)
{
    EventRoot *root = state;
    FVM_FileInfo *entry;
    FVM_Result status = dvm_eventfs_allowed(root->source);
    if (status != FVM_OK)
    {
        return status;
    }
    *entries = NULL;
    *count = 0;
    if (root->listed)
    {
        return FVM_OK;
    }
    entry = calloc(1, sizeof(*entry));
    if (!entry)
    {
        return FVM_OUT_OF_MEMORY;
    }
    status = dvm_eventfs_info(root->source, "events", entry);
    if (status != FVM_OK)
    {
        free(entry);
        return status;
    }
    *entries = entry;
    *count = 1;
    root->listed = 1;
    return FVM_OK;
}


/* EN/RU: Invalidate this listing only after failed managed batch publication.
 * Инвалидировать этот listing после failed managed publication. */
static void root_abort(void *state)
{
    EventRoot *root = state;
    root->listed = 1;
}


/* EN/RU: Rewind only root enumeration to zero, checking current access.
 * Сбросить только enumeration в ноль с проверкой current access. */
static FVM_Result root_seek(void *state, long offset, int origin, long *position)
{
    EventRoot *root = state;
    FVM_Result status = dvm_eventfs_allowed(root->source);
    if (status != FVM_OK)
    {
        return status;
    }
    if (offset || origin != FVM_FILE_SEEK_START || !position)
    {
        return FVM_UNSUPPORTED;
    }
    root->listed = 0;
    *position = 0;
    return FVM_OK;
}


/* EN/RU: Consume root/factory storage after its borrowers retire.
 * Поглотить root/factory storage после завершения borrowers. */
static void root_close(void *state)
{
    EventRoot *root = state;
    dvm_provider_release(root->source);
    free(root);
}
static const DVM_VfsOps root_ops = {root_info, NULL, NULL, root_seek, root_list, root_close, root_abort}; /* Read-only root directory. */


/* EN/RU: Open a root cursor or independent event observer, retaining source.
 * Открыть root cursor либо independent event observer, удерживая source. */
static FVM_Result factory_open(void *state, const char *path, const char *mode, DVM_VfsHandle *output)
{
    EventRoot *factory = state, *root;
    FVM_Result status = dvm_eventfs_allowed(factory->source);
    if (status != FVM_OK)
    {
        return status;
    }
    if (!strcmp(path, "events"))
    {
        return dvm_eventfile_open(factory->source, mode, output);
    }
    if (strcmp(path, ".") || strcmp(mode, "r"))
    {
        return FVM_UNSUPPORTED;
    }
    root = calloc(1, sizeof(*root));
    if (!root)
    {
        return FVM_OUT_OF_MEMORY;
    }
    if (!dvm_provider_retain(factory->source))
    {
        free(root);
        return FVM_ERROR;
    }
    root->source = factory->source;
    output->ops = &root_ops;
    output->state = root;
    return FVM_OK;
}
static const DVM_ProviderOps factory_ops = {factory_info, factory_open, root_close, DVM_PROVIDER_STABLE_BIND, access_check}; /* Fixed projection namespace. */


/* EN/RU: See eventfs.h; construction grants nothing until trusted host installation.
 * См. eventfs.h; constructor ничего не открывает до trusted host installation. */
DVM_Provider *dvm_provider_eventfs(DVM_Provider *source)
{
    EventRoot *root;
    DVM_Provider *provider;
    if (!source)
    {
        return NULL;
    }
    root = calloc(1, sizeof(*root));
    if (!root)
    {
        return NULL;
    }
    if (!dvm_provider_retain(source))
    {
        free(root);
        return NULL;
    }
    root->source = source;
    provider = dvm_provider_create(&factory_ops, root);
    if (!provider)
    {
        root_close(root);
    }
    return provider;
}
