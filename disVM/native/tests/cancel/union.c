/* Copyright (c) 2026 bhgv
 * EN: Cancellation at a union branch boundary must never open a lower provider.
 * RU: Отмена на границе union ветви не должна открывать нижний provider.
 */
#include "stack.h"
#include "export.h"
#include "cancel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Main owns these native counters through every factory and stream.
 * Main владеет native counters до закрытия всех factory и stream. */
typedef struct Source
{
    DVM_Cancel *cancel; /* Borrowed source, NULL for the lower branch. */
    unsigned opens; /* Actual branch open callbacks. */
    unsigned reads; /* Actual enumeration callbacks. */
    unsigned stats; /* Actual source metadata callbacks, including forbidden legacy probes. */
    int eof; /* Return apparent EOF after requesting cancellation. */
} Source;


/* EN/RU: Check a contract in optimized and sanitizer builds; exit on refusal.
 * Проверить contract во всех сборках; завершить тест при отказе. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "union list cancellation: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Borrow source/path; return an ordinary readable directory, without I/O.
 * Source/path borrowed; вернуть readable directory без I/O. */
static FVM_Result info(void *state, const char *path, FVM_FileInfo *out)
{
    ((Source *)state)->stats++;
    (void)path;
    memset(out, 0, sizeof(*out));
    out->directory = 1;
    out->mode = 0555;
    return FVM_OK;
}


/* EN/RU: Signal from the operation boundary, then return failure or apparent EOF.
 * Signal на границе операции, затем failure либо apparent EOF; outputs empty. */
static FVM_Result list(void *state, FVM_FileInfo **entries, size_t *count)
{
    Source *source = state;
    source->reads++;
    *entries = NULL;
    *count = 0;
    dvm_cancel_request(source->cancel);
    return source->eof ? FVM_OK : FVM_ERROR;
}


/* EN/RU: Stream borrows main-owned state; close transfers no ownership.
 * Stream заимствует state main; close не передаёт ownership. */
static void closefile(void *state)
{
    (void)state;
}
static const DVM_VfsOps stream_ops = {NULL, NULL, NULL, NULL, list, closefile, closefile}; /* Native directory fixture. */


/* EN/RU: Open a borrowed fixture directory; outer provider wrapper pins the factory.
 * Открыть borrowed directory; outer wrapper удерживает factory, no blocking. */
static FVM_Result openfile(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    (void)path;
    (void)mode;
    Source *source = state;
    source->opens++;
    out->ops = &stream_ops;
    out->state = source;
    out->native_flags = DVM_VFS_CANCEL_LIST;
    return FVM_OK;
}
static const DVM_ProviderOps factory_ops = {info, openfile, closefile, DVM_PROVIDER_STABLE_BIND, NULL}; /* Stable dot identity. */


/* EN/RU: Add a new capable/legacy union branch or child after opening the cursor.
 * Require refusal before any legacy callback, or live visibility of a capable bind.
 * Добавить capable/legacy union branch либо child после open; отказ до любого
 * legacy callback, а capable bind должен остаться видимым. */
static void dynamic(int child, int capable)
{
    DVM_Cancel *cancel = dvm_cancel_create();
    Source base = {NULL, 0, 0, 0, 1}, added = {NULL, 0, 0, 0, 1};
    unsigned flags = DVM_PROVIDER_CANCEL_PATH | DVM_PROVIDER_CANCEL_LIST;
    DVM_Provider *a = dvm_provider_create_native(&factory_ops, &base, NULL, 0, flags);
    DVM_Provider *b = dvm_provider_create_native(&factory_ops, &added, NULL, 0, capable ? flags : 0);
    DVM_Namespace *group = dvm_namespace_create("/", "/", NULL);
    check(cancel && a && b && group && dvm_namespace_pin(group), "dynamic owners");
    check(dvm_stack_mount(group, a, "/", 0, 0) == 1, "initial dynamic mount");
    if (!child)
    {
        check(dvm_stack_mount(group, a, "/", DVM_NS_AFTER, 0) == 1, "initial union head");
    }
    DVM_ResourcePlan plan;
    DVM_VfsHandle cursor = {0};
    check(dvm_resource_plan(group, "/", 0, &plan) &&
          dvm_namespace_directory(&plan, &cursor) == FVM_OK, "dynamic initial open");
    dvm_resource_close(&plan);
    check(cursor.native_flags & DVM_VFS_CANCEL_LIST, "initial capability");
    check(dvm_stack_mount(group, b, child ? "/child" : "/", child ? 0 : DVM_NS_AFTER, 0) == 1,
          "live namespace mutation");
    added.opens = added.stats = 0;
    DVM_CancelScope scope = {0};
    FVM_FileInfo *entries = NULL;
    size_t count = 0;
    check(dvm_cancel_enter(&scope, cancel), "dynamic scope");
    FVM_Result status = dvm_vfs_list(&cursor, &entries, &count);
    check(dvm_cancel_leave(&scope), "dynamic scope retirement");
    if (capable)
    {
        check(status == FVM_OK && count == (size_t)child &&
              (child ? added.stats != 0 : added.opens == 1), "capable new bind was frozen/hidden");
    }
    else
    {
        check(status == FVM_UNSUPPORTED && !count && !entries && !added.opens && !added.stats,
              "legacy callback entered under cancellation promise");
    }
    fvm_port_directory_entries_free(entries);
    dvm_vfs_close(&cursor);
    check(dvm_resource_plan(group, "/", 0, &plan) &&
          dvm_namespace_directory(&plan, &cursor) == FVM_OK, "reopen changed namespace");
    dvm_resource_close(&plan);
    check(!!(cursor.native_flags & DVM_VFS_CANCEL_LIST) == capable, "reopen did not refresh capability");
    entries = NULL;
    count = 0;
    check(dvm_vfs_list(&cursor, &entries, &count) == FVM_OK && count == (size_t)child,
          "ordinary enumeration after reopen failed");
    fvm_port_directory_entries_free(entries);
    dvm_vfs_close(&cursor);
    dvm_namespace_release(group);
    dvm_provider_release(a);
    dvm_provider_release(b);
    dvm_cancel_release(cancel);
}


/* EN/RU: Exercise real namespace/union/projection dispatch under a native scope.
 * Проверить настоящий namespace/union/projection dispatch под native scope. */
int main(void)
{
    for (int eof = 0; eof <= 1; eof++)
    {
        DVM_Cancel *cancel = dvm_cancel_create();
        Source upper = {cancel, 0, 0, 0, eof}, lower = {NULL, 0, 0, 0, 1};
        DVM_Provider *a = dvm_provider_create_native(&factory_ops, &upper, NULL, 0, DVM_PROVIDER_CANCEL_PATH | DVM_PROVIDER_CANCEL_LIST);
        DVM_Provider *b = dvm_provider_create_native(&factory_ops, &lower, NULL, 0, DVM_PROVIDER_CANCEL_PATH | DVM_PROVIDER_CANCEL_LIST);
        DVM_Namespace *group = dvm_namespace_create("/", "/", NULL);
        check(cancel && a && b && group && dvm_namespace_pin(group), "fixture owners");
        check(dvm_stack_mount(group, b, "/", 0, 0) == 1 &&
              dvm_stack_mount(group, a, "/", DVM_NS_BEFORE, 0) == 1, "ordered branches");
        DVM_ResourcePlan plan;
        DVM_VfsHandle cursor = {0};
        check(dvm_resource_plan(group, "/", 0, &plan) &&
              dvm_namespace_directory(&plan, &cursor) == FVM_OK, "directory projection");
        dvm_resource_close(&plan);
        check(cursor.native_flags & DVM_VFS_CANCEL_LIST, "capable namespace lost cancellation");
        DVM_CancelScope scope = {0};
        FVM_FileInfo *entries = NULL;
        size_t count = 0;
        check(dvm_cancel_enter(&scope, cancel), "scope admission");
        check(dvm_vfs_list(&cursor, &entries, &count) != FVM_OK && !entries && !count,
              "cancellation became EOF");
        check(dvm_cancel_leave(&scope) && !dvm_cancel_current(), "scope retirement");
        check(upper.reads == 1 && !lower.opens && !lower.reads, "lower provider entered after cancellation");
        check(dvm_vfs_list(&cursor, &entries, &count) != FVM_OK, "failed cursor resumed");
        dvm_vfs_close(&cursor);
        dvm_namespace_release(group);
        dvm_provider_release(a);
        dvm_provider_release(b);
        dvm_cancel_release(cancel);
    }
    for (int child = 0; child <= 1; child++)
    {
        for (int capable = 0; capable <= 1; capable++)
        {
            dynamic(child, capable);
        }
    }
    puts("PASS union directory cancellation boundaries and live binds");
    return 0;
}
