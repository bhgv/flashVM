/* Copyright (c) 2026 bhgv
 * EN: Union directory headers and lazy listing both skip cycles, preserve healthy entries and refuse false EOF.
 * RU: Header union directory и lazy listing пропускают cycles, сохраняют healthy entries и не выдают ложный EOF.
 */
#include "../../../runtime/vfs/export.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Main owns factory state until every namespace and directory cursor retires.
 * Main владеет factory state до завершения namespace/directory cursors.
 */
typedef struct Source
{
    DVM_Provider *self; /* Borrowed self edge for a real recursive root open, never an owning reference cycle. */
    int cyclic; /* Changed only between completed native directory operations. */
    unsigned loops; /* Actual recursive open callback entries. */
} Source;


/* EN/RU: Explicit fixture failure independent of compiler assertions; message borrowed.
 * Явный fixture failure независимо от compiler assertions; message borrowed.
 */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprintf(stderr, "directory walk: %s\n", message);
        exit(2);
    }
}


/* EN/RU: Native root metadata stays valid even when opening that branch would recurse.
 * Native root metadata валидна даже когда открытие ветви рекурсивно.
 */
static FVM_Result info(void *state, const char *path, FVM_FileInfo *output)
{
    (void)state;
    if (strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    memset(output, 0, sizeof(*output));
    output->directory = 1;
    output->mode = 0555;
    return FVM_OK;
}


/* EN/RU: Held header metadata describes its actual independently opened directory.
 * Metadata held header описывает actual independently opened directory.
 */
static FVM_Result held_info(void *state, FVM_FileInfo *output)
{
    return info(state, ".", output);
}


/* EN/RU: Return one owned healthy batch, then genuine EOF; state is this cursor's private consumed flag.
 * Вернуть owned healthy batch, затем настоящий EOF; state — private consumed flag cursor.
 */
static FVM_Result list(void *state, FVM_FileInfo **entries, size_t *count)
{
    int *done = state;
    *entries = NULL;
    *count = 0;
    if (*done)
    {
        return FVM_OK;
    }
    FVM_FileInfo *entry = calloc(1, sizeof(*entry));
    if (!entry)
    {
        return FVM_OUT_OF_MEMORY;
    }
    strcpy(entry->name, "healthy");
    entry->mode = 0444;
    *entries = entry;
    *count = 1;
    *done = 1;
    return FVM_OK;
}


/* EN/RU: Failed publication invalidates this private cursor instead of replaying an acknowledged batch.
 * Failed publication помечает private cursor вместо replay acknowledged batch.
 */
static void abort_list(void *state)
{
    *(int *)state = 1;
}


/* EN/RU: Consume one private native cursor flag; no provider or guest ownership in the state.
 * Поглотить private native cursor flag; state не владеет provider/guest.
 */
static void close_cursor(void *state)
{
    free(state);
}
static const DVM_VfsOps cursor_ops = {.info = held_info, .list = list, .close = close_cursor, .abort_list = abort_list}; /* Independent header/list streams. */


/* EN/RU: Recursive branch enters actual public open dispatch; healthy branch publishes one independent native cursor.
 * Recursive branch вызывает actual public open dispatch; healthy выдаёт independent native cursor.
 */
static FVM_Result open_directory(void *state, const char *path, const char *mode, DVM_VfsHandle *output)
{
    Source *source = state;
    if (source->cyclic)
    {
        source->loops++;
        return dvm_provider_open(source->self, path, mode, output);
    }
    int *done = calloc(1, sizeof(*done));
    if (!done)
    {
        return FVM_OUT_OF_MEMORY;
    }
    output->ops = &cursor_ops;
    output->state = done;
    return FVM_OK;
}


/* EN/RU: Main owns stack Source; final factory release must not free it.
 * Main владеет stack Source; final factory release его не освобождает.
 */
static void destroy(void *state)
{
    (void)state;
}
static const DVM_ProviderOps operations = {info, open_directory, destroy, DVM_PROVIDER_STABLE_BIND, NULL}; /* Stable live root, optionally cyclic open. */


/* EN/RU: Prove top cyclic open cannot hide a healthy directory, and incomplete enumeration never becomes successful EOF.
 * Проверить healthy directory под cyclic top и refusal ложного EOF для incomplete enumeration.
 */
int main(void)
{
    Source top = {.cyclic = 1};
    Source bottom = {0};
    top.self = dvm_provider_create(&operations, &top);
    bottom.self = dvm_provider_create(&operations, &bottom);
    DVM_Namespace *group = dvm_namespace_create("/", "/", NULL);
    check(group && top.self && bottom.self && dvm_stack_mount(group, top.self, "/", 0, 0) == 1 &&
        dvm_stack_mount(group, bottom.self, "/", DVM_NS_AFTER, 0) == 1, "union roots");
    DVM_ResourcePlan plan;
    DVM_VfsHandle cursor = {0};
    check(dvm_resource_plan(group, "/", 0, &plan) && dvm_union_directory(&plan, &cursor) == FVM_OK && top.loops == 1,
        "cyclic header open prevented healthy lower directory");
    dvm_resource_close(&plan);
    FVM_FileInfo *entries = NULL;
    size_t count = 0;
    check(dvm_vfs_list(&cursor, &entries, &count) == FVM_OK && count == 1 && !strcmp(entries[0].name, "healthy") &&
        top.loops == 2 && !walk_depth(), "cyclic list branch hid lower entries");
    fvm_port_directory_entries_free(entries);
    entries = NULL;
    check(dvm_vfs_list(&cursor, &entries, &count) == DVM_VFS_CYCLE && !entries && !count && !walk_depth(),
        "incomplete directory enumeration was relabeled EOF");
    dvm_vfs_close(&cursor);
    top.cyclic = 0;
    check(dvm_resource_plan(group, "/", 0, &plan) && dvm_union_directory(&plan, &cursor) == FVM_OK,
        "later healthy directory open retained old cycle state");
    dvm_resource_close(&plan);
    for (unsigned i = 0; i < 2; i++)
    {
        check(dvm_vfs_list(&cursor, &entries, &count) == FVM_OK && count == 1, "later healthy union listing");
        fvm_port_directory_entries_free(entries);
        entries = NULL;
    }
    check(dvm_vfs_list(&cursor, &entries, &count) == FVM_OK && !count && !entries && !walk_depth(), "genuine healthy EOF");
    dvm_vfs_close(&cursor);
    dvm_namespace_release(group);
    dvm_provider_release(top.self);
    dvm_provider_release(bottom.self);
    puts("PASS cyclic directory header/list fallback, incomplete-enumeration error and subsequent genuine EOF");
    return 0;
}
