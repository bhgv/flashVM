/* Copyright (c) 2026 bhgv
 * EN: Native anchored file/directory capabilities and combined provider mount/bind.
 * RU: Native anchored file/directory capabilities и combined provider mount/bind.
 */
#include "export.h"
#include "view_internal.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: Captured root identity, never reopen a stale diagnostic host pathname.
 * Captured root identity; без открытия stale diagnostic host pathname. */
typedef struct NativeExport
{
    DVM_Anchor *anchor; /* Retained native directory identity or parent of a file. */
    char component[DVM_NS_PATH_LIMIT]; /* File component for exact-file exports. */
    int directory; /* Descendant delegation is permitted only for a pinned directory. */
} NativeExport;


/* EN/RU: Build a safe anchor-relative path without traversal or file descendants.
 * Построить safe anchor-relative path без traversal/file descendants;1/0. */
static int path(NativeExport *root, const char *relative, char *output)
{
    if (!dvm_view_path(relative, 0) || (!root->directory && strcmp(relative, ".")))
    {
        return 0;
    }
    strcpy(output, root->directory ? relative : root->component);
    return 1;
}


/* EN/RU: Native metadata through the captured capability only.
 * Native metadata только через captured capability. */
static FVM_Result info(void *state, const char *relative, FVM_FileInfo *output)
{
    NativeExport *root = state;
    char name[DVM_NS_PATH_LIMIT];
    return path(root, relative, name) ? fvm_port_anchor_info_at(dvm_anchor_port(root->anchor), name, output) : FVM_NOT_FOUND;
}


/* EN/RU: Delegate effective native rwx checks, never infer from mode alone.
 * Передать effective native rwx checks; не выводить их только из mode. */
static FVM_Result access_check(void *state, const char *relative, unsigned access)
{
    NativeExport *root = state;
    char name[DVM_NS_PATH_LIMIT];
    return path(root, relative, name) ? fvm_port_anchor_access_at(dvm_anchor_port(root->anchor), name, access) : FVM_ERROR;
}


/* EN/RU: Open ordinary native resources, adapting provider rw to existing-file rwo.
 * Открыть ordinary native resource; provider rw преобразовать в existing-file rwo. */
static FVM_Result open_file(void *state, const char *relative, const char *mode, DVM_VfsHandle *output)
{
    NativeExport *root = state;
    FVM_FileInfo metadata;
    FVM_PortFile *file = NULL;
    FVM_PortDirectory *directory = NULL;
    char name[DVM_NS_PATH_LIMIT];
    FVM_Result status;
    if (!path(root, relative, name))
    {
        return FVM_NOT_FOUND;
    }
    if (!strcmp(mode, "r") && fvm_port_anchor_info_at(dvm_anchor_port(root->anchor), name, &metadata) == FVM_OK && metadata.directory)
    {
        status = fvm_port_directory_open_at(dvm_anchor_port(root->anchor), name, &directory);
    }
    else
    {
        status = fvm_port_file_open_at(dvm_anchor_port(root->anchor), name, !strcmp(mode, "rw") ? "rwo" : mode, &file);
    }
    if (status == FVM_OK && !dvm_vfs_adopt(output, file, directory))
    {
        fvm_port_file_close(file);
        fvm_port_directory_close(directory);
        status = FVM_ERROR;
    }
    return status;
}


/* EN/RU: Release retained native identity after all bindings/FDs retire.
 * Освободить native identity после retirement всех bindings/FDs. */
static void destroy(void *state)
{
    NativeExport *root = state;
    dvm_anchor_release(root->anchor);
    free(root);
}
static const DVM_ProviderOps ops = {info, open_file, destroy, DVM_PROVIDER_STABLE_BIND, access_check}; /* Immutable root mapping. */


/* EN/RU: See export.h; freeze directory identity or exact parent/file mapping.
 * См. export.h; закрепить directory identity либо parent/file mapping. */
DVM_Provider *dvm_export_native(DVM_ResourcePlan *plan, int directory)
{
    NativeExport *root;
    if (!plan || plan->provider || !plan->anchor)
    {
        return NULL;
    }
    root = calloc(1, sizeof(*root));
    if (!root)
    {
        return NULL;
    }
    root->directory = directory != 0;
    if (directory)
    {
        DVM_Anchor *directory = dvm_anchor_open(plan->anchor, plan->relative);
        root->anchor = directory ? dvm_anchor_confine(directory) : NULL;
        dvm_anchor_release(directory);
    }
    else
    {
        root->anchor = dvm_anchor_confine(plan->anchor);
        strcpy(root->component, plan->relative);
    }
    if (!root->anchor)
    {
        free(root);
        return NULL;
    }
    DVM_Provider *provider = dvm_provider_create(&ops, root);
    if (!provider)
    {
        destroy(root);
    }
    return provider;
}


/* EN/RU: See export.h; synthetic source is a retained capability, not a global ID.
 * См. export.h; synthetic source — retained capability, не global ID. */
int dvm_stack_mount(DVM_Namespace *group, DVM_Provider *provider, const char *on, unsigned flags, int64_t index)
{
    DVM_ResourcePlan target, source;
    if (!group || !provider || !dvm_resource_plan(group, on, 0, &target))
    {
        return -1;
    }
    memset(&source, 0, sizeof(source));
    source.group = group;
    source.names.version = target.names.version;
    strcpy(source.names.path, "/");
    strcpy(source.names.native, "/export-capability");
    strcpy(source.relative, ".");
    source.provider = provider;
    int result = dvm_stack_bind(&source, &target, flags, index);
    dvm_resource_close(&target);
    return result;
}
