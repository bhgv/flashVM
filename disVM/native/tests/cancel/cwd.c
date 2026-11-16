/* Copyright (c) 2026 bhgv
 * EN: Native provider cwd identity, copy, cancellation and staged-root retirement.
 * RU: Native provider cwd: identity, copy, отмена и освобождение staged roots.
 */
#include "export.h"
#include "stack.h"
#include "cancel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Main owns this fixture state until namespace/factory/scope teardown finishes.
 * Main владеет fixture state до завершения namespace/factory/scope teardown. */
typedef struct State
{
    DVM_Cancel *cancel; /* Borrowed current operation source. */
    unsigned roots; /* Fresh roots allocated by source capture. */
    unsigned retired; /* Fresh root destructors. */
    unsigned source_queries; /* Original source metadata callbacks. */
    unsigned target_queries; /* Original target metadata callbacks. */
    unsigned root_queries; /* Staged captured-root metadata callbacks. */
    int stage; /* 0 commit;1..4 cancel;5 bad root;6 legacy;7 deny;8 stable;9 wrong kind;10 stale;11 cancel commit;12..14 access. */
} State;


/* EN/RU: Native factory node owns one copied identity and borrows main fixture counters.
 * Native factory node владеет copied identity и заимствует счётчики main. */
typedef struct Node
{
    State *state; /* Borrowed through all native factory lifetime pins. */
    int identity; /* 0 original source,1 original target,2 fresh captured root. */
} Node;


/* EN/RU: Keep contracts enabled under optimization/sanitizers; terminate on failure.
 * Contracts включены под optimization/sanitizers; завершить тест при нарушении. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "native provider cwd: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Return acknowledged directory metadata, optionally signalling the exact tested boundary.
 * Вернуть подтверждённые metadata каталога, optional signal на проверяемой границе. */
static FVM_Result info(void *opaque, const char *path, FVM_FileInfo *out)
{
    Node *node = opaque;
    State *state = node->state;
    (void)path;
    if (node->identity == 0)
    {
        state->source_queries++;
    }
    else if (node->identity == 1)
    {
        state->target_queries++;
    }
    else
    {
        state->root_queries++;
    }
    memset(out, 0, sizeof(*out));
    out->directory = !(state->stage == 9 && node->identity == 2);
    out->mode = 0555;
    out->identity.low = node->identity;
    if ((state->stage == 1 && node->identity == 0) || (state->stage == 3 && node->identity == 2))
    {
        dvm_cancel_request(state->cancel);
    }
    return FVM_OK;
}


/* EN/RU: This metadata/capture fixture never opens streams; output stays empty.
 * Metadata/capture fixture не открывает streams; output остаётся пустым. */
static FVM_Result openfile(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    (void)state;
    (void)path;
    (void)mode;
    (void)out;
    return FVM_UNSUPPORTED;
}


/* EN/RU: Consume one native node; only fresh roots affect retirement counters.
 * Поглотить native node; только fresh roots меняют retirement counters. */
static void destroy(void *opaque)
{
    Node *node = opaque;
    node->state->retired += node->identity == 2;
    free(node);
}


/* EN/RU: Borrow Node state; path/access are unused. Stages7/12 refuse search,
 * stages13/14 request cancellation; return provider status without granting authority.
 * Заимствовать Node state; path/access не используются. Stages7/12 отклоняют search,
 * stages13/14 запрашивают отмену; вернуть provider status без выдачи authority. */
static FVM_Result access_check(void *state, const char *path, unsigned access)
{
    Node *node = state;
    (void)path;
    (void)access;
    if (node->state->stage == 13 || (node->state->stage == 14 && node->identity == 2))
    {
        dvm_cancel_request(node->state->cancel);
    }
    return node->state->stage == 7 || (node->state->stage == 12 && node->identity == 2) ? FVM_ERROR : FVM_OK;
}
static const DVM_ProviderOps source_ops = {info, openfile, destroy, 0, access_check}; /* Descendants need fresh acquisition. */
static const DVM_ProviderOps target_ops = {info, openfile, destroy, DVM_PROVIDER_STABLE_BIND, access_check}; /* Fixed target route. */


/* EN/RU: Allocate one owned fresh root; stage5 deliberately violates the PATH promise.
 * Создать свой fresh root; stage5 намеренно нарушает обещание PATH. */
static DVM_Provider *capture(void *opaque, const char *path)
{
    Node *original = opaque;
    State *state = original->state;
    (void)path;
    Node *node = calloc(1, sizeof(*node));
    check(node != NULL, "root allocation");
    *node = (Node){state, 2};
    DVM_Provider *root = dvm_provider_create_native(&source_ops, node, capture, 1,
        state->stage == 5 ? 0 : DVM_PROVIDER_CANCEL_PATH | DVM_PROVIDER_CANCEL_CAPTURE);
    check(root != NULL, "root factory");
    state->roots++;
    if (state->stage == 2)
    {
        dvm_cancel_request(state->cancel);
    }
    return root;
}


/* EN/RU: Resolve and query borrowed group/path/module, returning the selected provider identity.
 * Resolve/query borrowed group/path/module, вернуть selected provider identity; fail explicitly. */
static uint64_t identity(DVM_Namespace *group, const char *path, int module)
{
    DVM_ResourcePlan plan;
    FVM_FileInfo metadata;
    check(dvm_resource_plan(group, path, module, &plan) &&
          dvm_resource_info(&plan, &metadata) == FVM_OK, "resolved cwd metadata");
    uint64_t value = metadata.identity.low;
    dvm_resource_close(&plan);
    return value;
}


/* EN/RU: Exercise one native cwd acquisition boundary with independent group/factory owners.
 * Stage selects failure or stable routing; check sticky/copy/import behaviour after commit.
 * Проверить native cwd stage с отдельными owners; после commit проверить sticky/copy/import. */
static void scenario(int stage)
{
    State state = {0};
    state.cancel = dvm_cancel_create();
    Node *a = calloc(1, sizeof(*a));
    Node *b = calloc(1, sizeof(*b));
    check(state.cancel && a && b, "fixture allocation");
    *a = (Node){&state, 0};
    *b = (Node){&state, 1};
    DVM_Provider *source = dvm_provider_create_native(stage == 8 ? &target_ops : &source_ops, a,
        capture, 1, DVM_PROVIDER_CANCEL_PATH | (stage == 6 ? 0 : DVM_PROVIDER_CANCEL_CAPTURE));
    DVM_Provider *other = dvm_provider_create_native(&target_ops, b, NULL, 1, DVM_PROVIDER_CANCEL_PATH);
    DVM_Namespace *group = dvm_namespace_create("/", "/", NULL);
    check(source && other && group && dvm_namespace_pin(group) &&
          dvm_stack_mount(group, source, "/", 0, 0) == 1, "source mount");
    DVM_ResourcePlan plan;
    check(dvm_resource_plan(group, "/dir", 0, &plan), "directory plan");
    uint64_t revision = plan.names.version;
    state.stage = stage;
    DVM_CancelScope scope = {0};
    check(dvm_cancel_enter(&scope, state.cancel), "scope admission");
    if (stage == 4)
    {
        dvm_cancel_request(state.cancel);
    }
    int acquired = dvm_resource_directory_acquire(&plan);
    if (stage == 10)
    {
        DVM_NamespacePlan changed;
        check(dvm_namespace_plan(group, "/elsewhere", &changed) &&
              dvm_namespace_chdir(group, &changed) == 1, "concurrent revision");
    }
    if (stage == 11)
    {
        dvm_cancel_request(state.cancel);
    }
    int outcome = acquired ? dvm_namespace_chdir_resource(group, &plan) : -1;
    check(dvm_cancel_leave(&scope) && !dvm_cancel_current(), "scope retirement");
    dvm_resource_close(&plan);
    int success = stage == 0 || stage == 8;
    check(success ? outcome == 1 : stage == 10 ? outcome == 0 : outcome == -1,
          "cwd acquisition/cancellation result");
    DVM_NamespacePlan after;
    check(dvm_namespace_plan(group, ".", &after), "post-cwd plan");
    check(success ? !strcmp(after.path, "/dir") && after.version == revision+1 :
          !strcmp(after.path, stage == 10 ? "/elsewhere" : "/") &&
          after.version == revision+(stage == 10), "partial cwd commit");
    if (success)
    {
        state.stage = 0;
        uint64_t held = stage == 8 ? 0 : 2;
        DVM_Namespace *copy = dvm_namespace_copy(group);
        check(copy && dvm_stack_mount(group, other, "/", 0, 0) == 1, "copy and covering rebind");
        check(identity(group, "item", 0) == held && identity(group, "provider.dis", 1) == held &&
              identity(group, "/dir/item", 0) == 1 && identity(group, "../item", 0) == 1 &&
              identity(copy, "item", 0) == held, "cwd/import replaced by covering bind");
        check(dvm_stack_mount(group, other, "/dir/sub", 0, 0) == 1 &&
              identity(group, "sub/item", 0) == 1 && identity(copy, "sub/item", 0) == held,
              "descendant overlay or namespace copy escaped");
        dvm_namespace_release(copy);
    }
    dvm_namespace_release(group);
    dvm_provider_release(source);
    dvm_provider_release(other);
    check(state.roots == state.retired, "staged cwd/origin leak");
    dvm_cancel_release(state.cancel);
}


/* EN/RU: Run directory acquisition, cancellation, stale and sticky/copy cases.
 * Проверить acquisition, отмену, stale и sticky/copy cases. */
int main(void)
{
    for (int stage = 0; stage <= 14; stage++)
    {
        scenario(stage);
    }
    puts("PASS native provider cwd identity copy cancellation and retirement");
    return 0;
}
