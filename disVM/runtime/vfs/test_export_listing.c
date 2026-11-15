/* Copyright (c) 2026 bhgv
 * EN: Fully hidden source batches must not become a false visible-directory EOF.
 * RU: Полностью скрытые source batches не должны становиться false EOF каталога.
 */
#include "export.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Fail an observed directory contract explicitly.
 * Явно завершить test при нарушении directory contract. */
static void require(int ok, const char *label)
{
    if (!ok)
    {
        fprintf(stderr, "export listing: %s\n", label);
        abort();
    }
}


/* EN/RU: Serve fixed directory/file metadata with distinct names.
 * Вернуть fixed metadata каталога/файлов с разными names. */
static FVM_Result info(void *state, const char *path, FVM_FileInfo *output)
{
    (void)state;
    if (strcmp(path, ".") && strcmp(path, "secret") && strcmp(path, "safe"))
    {
        return FVM_NOT_FOUND;
    }
    memset(output, 0, sizeof(*output));
    output->directory = !strcmp(path, ".");
    output->mode = output->directory ? 0555 : 0444;
    return FVM_OK;
}


/* EN/RU: Emit hidden first batch, visible second batch, then real EOF.
 * Выдать hidden first batch, visible second batch, затем real EOF. */
static FVM_Result list(void *state, FVM_FileInfo **entries, size_t *count)
{
    unsigned *page = state;
    *entries = NULL;
    *count = 0;
    if (*page >= 2)
    {
        return FVM_OK;
    }
    *entries = calloc(1, sizeof(**entries));
    if (!*entries)
    {
        return FVM_OUT_OF_MEMORY;
    }
    strcpy((*entries)[0].name, (*page)++ ? "safe" : "secret");
    *count = 1;
    return FVM_OK;
}


/* EN/RU: Consume private directory cursor.
 * Освободить private directory cursor. */
static void close_cursor(void *state)
{
    free(state);
}
/* EN/RU: Retire failed cursor; stop enumeration after publication failure.
 * Завершить cursor после ошибки публикации. */
static void abort_cursor(void *state)
{
    *(unsigned *)state = 2;
}
static const DVM_VfsOps directory_ops = {NULL, NULL, NULL, NULL, list, close_cursor, abort_cursor}; /* Two-batch source directory. */


/* EN/RU: Open independent directory cursor, no data files needed by this test.
 * Открыть independent cursor, data files этому test не нужны. */
static FVM_Result open_directory(void *state, const char *path, const char *mode, DVM_VfsHandle *output)
{
    (void)state;
    if (strcmp(path, ".") || strcmp(mode, "r"))
    {
        return FVM_UNSUPPORTED;
    }
    output->state = calloc(1, sizeof(unsigned));
    output->ops = output->state ? &directory_ops : NULL;
    return output->state ? FVM_OK : FVM_OUT_OF_MEMORY;
}


/* EN/RU: Count final factory retirement in borrowed test storage.
 * Посчитать final factory retirement в borrowed test storage. */
static void destroy(void *state)
{
    (*(unsigned *)state)++;
}
static const DVM_ProviderOps source_ops = {info, open_directory, destroy, DVM_PROVIDER_STABLE_BIND, NULL}; /* Fixed node identities. */


/* EN/RU: Deny search while leaving metadata/read visible for synthetic parents.
 * Запретить search, сохранив metadata/read видимыми для synthetic parents. */
static int readonly_policy(void *context, const char *path, unsigned access)
{
    (void)context;
    (void)path;
    return !(access & FVM_ACCESS_EXEC);
}


/* EN/RU: Actual provider/list adapter, not a copied filtering algorithm.
 * Проверить actual provider/list adapter, не копию filtering algorithm. */
void test_export_hidden_batches(void)
{
    unsigned destroyed = 0;
    DVM_Provider *source = dvm_provider_create(&source_ops, &destroyed);
    DVM_View *builder = dvm_view_create(NULL, NULL, NULL);
    require(source && builder && dvm_view_map(builder, "/", source, ".", 5, DVM_VIEW_TREE) &&
            dvm_view_map(builder, "/secret", NULL, ".", 0, DVM_VIEW_TREE), "mapping");
    DVM_Provider *view = dvm_view_freeze(builder);
    DVM_VfsHandle directory = {0};
    FVM_FileInfo *entries = NULL;
    size_t count = 0;
    require(view && dvm_provider_open(view, ".", "r", &directory) == FVM_OK, "open");
    require(
            dvm_vfs_list(&directory, &entries, &count) == FVM_OK && count == 1 &&
            !strcmp(entries[0].name, "safe"), "hidden batch became EOF or leaked secret");
    fvm_port_directory_entries_free(entries);
    require(dvm_vfs_list(&directory, &entries, &count) == FVM_OK && !count, "missing real EOF");
    dvm_vfs_close(&directory);
    dvm_provider_release(view);
    builder = dvm_view_create(readonly_policy, NULL, NULL);
    require(builder && dvm_view_map(builder, "/some/leaf", source, "safe", 4, 0), "synthetic parent mapping");
    view = dvm_view_freeze(builder);
    FVM_FileInfo metadata;
    require(view && dvm_provider_info(view, "some", &metadata) == FVM_OK && metadata.directory &&
            metadata.mode == 0444 && dvm_provider_open(view, "some", "r", &directory) != FVM_OK,
            "synthetic parent advertised or bypassed denied search");
    dvm_provider_release(view);
    dvm_provider_release(source);
    require(destroyed == 1, "source lifetime leak");
}
