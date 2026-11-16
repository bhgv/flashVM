/* Copyright (c) 2026 bhgv
 * EN: Native bind preparation/commit cancellation and exact staged-root retirement.
 * RU: Native отмена подготовки/commit bind и точное освобождение staged roots.
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
    int stage; /* 0 commit;1..4 cancellation phases;5 bad root;6 legacy capture;7 legacy metadata. */
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
        fprintf(stderr, "native bind cancellation: %s\n", why);
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
    out->directory = 1;
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
static const DVM_ProviderOps source_ops = {info, openfile, destroy, 0}; /* Descendants need fresh acquisition. */
static const DVM_ProviderOps target_ops = {info, openfile, destroy, DVM_PROVIDER_STABLE_BIND}; /* Fixed target route. */


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


/* EN/RU: Own all native inputs through one real transaction and compare unchanged destination/revision.
 * Владеть native inputs до завершения настоящей transaction; сравнить destination/revision. */
static void scenario(int stage)
{
    State state = {0};
    state.cancel = dvm_cancel_create();
    Node *a = calloc(1, sizeof(*a)), *b = calloc(1, sizeof(*b));
    check(state.cancel && a && b, "fixture allocation");
    *a = (Node){&state, 0};
    *b = (Node){&state, 1};
    DVM_Provider *source = dvm_provider_create_native(&source_ops, a, capture, 1,
        DVM_PROVIDER_CANCEL_PATH | (stage == 6 ? 0 : DVM_PROVIDER_CANCEL_CAPTURE));
    DVM_Provider *target = dvm_provider_create_native(&target_ops, b, NULL, 1, stage == 7 ? 0 : DVM_PROVIDER_CANCEL_PATH);
    DVM_Namespace *group = dvm_namespace_create("/", "/", NULL);
    check(source && target && group && dvm_namespace_pin(group), "fixture owners");
    check(dvm_stack_mount(group, source, "/", 0, 0) == 1 &&
          dvm_stack_mount(group, target, "/target", 0, 0) == 1, "setup mounts");
    DVM_ResourcePlan from, on;
    check(dvm_resource_plan(group, "/source/node", 0, &from) &&
          dvm_resource_plan(group, "/target", 0, &on), "retained plans");
    uint64_t revision = on.names.version;
    state.source_queries = state.target_queries = state.root_queries = 0;
    state.stage = stage;
    DVM_CancelScope scope = {0};
    check(dvm_cancel_enter(&scope, state.cancel), "scope admission");
    if (stage == 4)
    {
        dvm_cancel_request(state.cancel);
    }
    int result = dvm_stack_bind(&from, &on, 0, 0);
    check(dvm_cancel_leave(&scope) && !dvm_cancel_current(), "scope retirement");
    dvm_resource_close(&from);
    dvm_resource_close(&on);
    DVM_ResourcePlan after;
    check(dvm_resource_plan(group, "/target", 0, &after), "post-operation route");
    check(stage == 0 ? result == 1 && after.names.version == revision+1 && after.provider != target :
          result == -1 && after.names.version == revision && after.provider == target,
          "cancelled transaction published a namespace revision");
    if (stage == 1 || stage == 4)
    {
        check(!state.target_queries && !state.roots, "callback continued after cancellation");
    }
    if (stage >= 6)
    {
        check(!state.roots && (stage != 7 || !state.target_queries), "unsupported callback was invoked");
    }
    if (stage == 5)
    {
        check(!state.root_queries, "unsupported acquired root metadata was invoked");
    }
    dvm_resource_close(&after);
    dvm_namespace_release(group);
    dvm_provider_release(source);
    dvm_provider_release(target);
    check(state.roots == state.retired, "staged roots/origin references leaked");
    dvm_cancel_release(state.cancel);
}


/* EN/RU: Run commit and every cancellation/refusal phase with independent native owners.
 * Проверить commit и каждую phase cancellation/refusal с независимыми native owners. */
int main(void)
{
    for (int stage = 0; stage <= 7; stage++)
    {
        scenario(stage);
    }
    puts("PASS native bind preparation commit and root retirement");
    return 0;
}
