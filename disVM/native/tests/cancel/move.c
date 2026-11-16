/* Copyright (c) 2026 bhgv
 * EN: Native projected move cancellation and exact staged-root retirement.
 * RU: Native отмена проекции move и точное освобождение staged roots.
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
    int stage; /* 0 commit;2 first capture;3 final capture;4 pre-cancel;5 bad root;6 legacy capture. */
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
        fprintf(stderr, "native move cancellation: %s\n", why);
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
    if (state->stage == 2 || (state->stage == 3 && state->roots == 2))
    {
        dvm_cancel_request(state->cancel);
    }
    return root;
}


/* EN/RU: Own a two-provider projected stack through one move and compare parent/revision.
 * Stage selects a native cancellation/refusal boundary; all owners retire before return.
 * Владеть стеком из двух providers; stage задаёт границу отмены/отказа, проверить parent/revision. */
static void scenario(int stage)
{
    State state = {0};
    state.cancel = dvm_cancel_create();
    Node *a = calloc(1, sizeof(*a));
    Node *b = calloc(1, sizeof(*b));
    check(state.cancel && a && b, "fixture allocation");
    *a = (Node){&state, 0};
    *b = (Node){&state, 1};
    unsigned flags = DVM_PROVIDER_CANCEL_PATH | (stage == 6 ? 0 : DVM_PROVIDER_CANCEL_CAPTURE);
    DVM_Provider *first = dvm_provider_create_native(&source_ops, a, capture, 1, flags);
    DVM_Provider *second = dvm_provider_create_native(&source_ops, b, capture, 1, flags);
    DVM_Namespace *group = dvm_namespace_create("/", "/", NULL);
    check(first && second && group && dvm_namespace_pin(group), "fixture owners");
    check(dvm_stack_mount(group, first, "/", 0, 0) == 1 &&
          dvm_stack_mount(group, second, "/", DVM_NS_AFTER, 0) == 1, "setup stack");
    DVM_ResourcePlan plan;
    check(dvm_resource_plan(group, "/child", 0, &plan) && plan.layers, "projected plan");
    uint64_t revision = plan.names.version;
    state.stage = stage;
    state.source_queries = state.target_queries = state.root_queries = 0;
    DVM_CancelScope scope = {0};
    check(dvm_cancel_enter(&scope, state.cancel), "scope admission");
    if (stage == 4)
    {
        dvm_cancel_request(state.cancel);
        check(dvm_stack_move(&plan, revision, plan.layer_id, 0) == -1,
              "pre-cancelled no-op move acknowledged success");
    }
    int result = dvm_stack_move(&plan, revision, plan.layer_id, -1);
    check(dvm_cancel_leave(&scope) && !dvm_cancel_current(), "scope retirement");
    dvm_resource_close(&plan);
    DVM_ResourcePlan after;
    DVM_ResourcePlan parent;
    check(dvm_resource_plan(group, "/child", 0, &after) &&
          dvm_resource_plan(group, "/", 0, &parent), "post-operation routes");
    check(stage == 0 ? result == 1 && after.names.version == revision+1 && after.provider != first :
          result == -1 && after.names.version == revision && after.provider == first,
          "cancelled move published partial namespace");
    check(parent.provider == first && parent.layers && parent.layers->provider == second,
          "child projection changed parent stack");
    check(state.roots == (stage == 0 || stage == 3 ? 2u : stage == 2 || stage == 5 ? 1u : 0u),
          "capture continued after cancellation or entered unsupported callback");
    check(!state.source_queries && !state.target_queries && !state.root_queries,
          "move issued unexpected metadata callbacks");
    dvm_resource_close(&after);
    dvm_resource_close(&parent);
    dvm_namespace_release(group);
    dvm_provider_release(first);
    dvm_provider_release(second);
    check(state.roots == state.retired, "staged roots leaked");
    dvm_cancel_release(state.cancel);
}


/* EN/RU: Run committed projection and all supported cancellation/refusal boundaries.
 * Проверить committed projection и границы cancellation/refusal. */
int main(void)
{
    const int stages[] = {0, 2, 3, 4, 5, 6};
    for (unsigned i = 0; i < sizeof(stages)/sizeof(stages[0]); i++)
    {
        scenario(stages[i]);
    }
    puts("PASS native move preparation commit and root retirement");
    return 0;
}
