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
    (void)state;
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
    return FVM_OK;
}
static const DVM_ProviderOps factory_ops = {info, openfile, closefile, DVM_PROVIDER_STABLE_BIND, NULL}; /* Stable dot identity. */


/* EN/RU: Exercise real namespace/union/projection dispatch under a native scope.
 * Проверить настоящий namespace/union/projection dispatch под native scope. */
int main(void)
{
    for (int eof = 0; eof <= 1; eof++)
    {
        DVM_Cancel *cancel = dvm_cancel_create();
        Source upper = {cancel, 0, 0, eof}, lower = {NULL, 0, 0, 1};
        DVM_Provider *a = dvm_provider_create(&factory_ops, &upper);
        DVM_Provider *b = dvm_provider_create(&factory_ops, &lower);
        DVM_Namespace *group = dvm_namespace_create("/", "/", NULL);
        check(cancel && a && b && group && dvm_namespace_pin(group), "fixture owners");
        check(dvm_stack_mount(group, b, "/", 0, 0) == 1 &&
              dvm_stack_mount(group, a, "/", DVM_NS_BEFORE, 0) == 1, "ordered branches");
        DVM_ResourcePlan plan;
        DVM_VfsHandle cursor = {0};
        check(dvm_resource_plan(group, "/", 0, &plan) &&
              dvm_namespace_directory(&plan, &cursor) == FVM_OK, "directory projection");
        dvm_resource_close(&plan);
        check(!(cursor.native_flags & DVM_VFS_CANCEL_LIST), "mutable namespace falsely promises cancellation");
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
    puts("PASS union directory cancellation boundaries");
    return 0;
}
