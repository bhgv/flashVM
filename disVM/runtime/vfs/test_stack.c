/* Copyright (c) 2026 bhgv
 * EN: Real provider stack insertion, signed positions, per-file projection and stale mutation refusal.
 * RU: Provider stack insertion, signed positions, per-file projection и stale mutation refusal.
 */
#include "stack.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static unsigned fail_after; /* One-shot native allocation failure, only during a prepared mutation. */
void *__real_calloc(size_t count, size_t size); /* Original allocator, preserved outside injection. */


/* EN/RU: Refuse the selected real allocation, then restore normal allocation.
 * Отказать выбранной allocation, затем восстановить normal allocation. */
void *__wrap_calloc(size_t count, size_t size)
{
    if (fail_after && !--fail_after)
    {
        return NULL;
    }
    return __real_calloc(count, size);
}


/* EN/RU: Native source lives through final provider/factory retirement.
 * Native source живёт до final provider/factory retirement. */
typedef struct Source
{
    char value; /* Distinguishing immutable file byte. */
    unsigned destroyed; /* Final source destruction count. */
} Source;


/* EN/RU: Fail explicitly, without compile-mode assertions.
 * Явный failure, без compile-mode assertions. */
static void require(int ok, const char *label)
{
    if (!ok)
    {
        fprintf(stderr, "stack control: %s\n", label);
        exit(2);
    }
}


/* EN/RU: Fixed directory and file identities for this retained provider.
 * Фиксированные identities directory/files, output copied only on success. */
static FVM_Result info(void *state, const char *path, FVM_FileInfo *output)
{
    (void)state;
    if (strcmp(path, ".") && strcmp(path, "f") && strcmp(path, "g"))
    {
        return FVM_NOT_FOUND;
    }
    memset(output, 0, sizeof(*output));
    output->directory = !strcmp(path, ".");
    output->mode = output->directory ? 0755 : 0644;
    return FVM_OK;
}


/* EN/RU: Read one immutable byte, state borrowed from the retained factory.
 * Прочитать один immutable byte из borrowed factory state. */
static FVM_Result read_file(void *state, void *bytes, size_t size, size_t *received)
{
    Source *source = state;
    *received = size ? 1 : 0;
    if (size)
    {
        *(char *)bytes = source->value;
    }
    return FVM_OK;
}


/* EN/RU: Inner stream owns no allocation; outer provider wrapper retains factory.
 * Inner stream без allocation; outer wrapper удерживает factory. */
static void close_file(void *state)
{
    (void)state;
}
static const DVM_VfsOps file_ops = {NULL, read_file, NULL, NULL, NULL, close_file}; /* Fixed readonly byte resource. */


/* EN/RU: Open only fixed file names for read, preserving exact factory state.
 * Открыть fixed file names для read; output owns one stream on success. */
static FVM_Result open_file(void *state, const char *path, const char *mode, DVM_VfsHandle *output)
{
    if ((strcmp(path, "f") && strcmp(path, "g")) || strcmp(mode, "r"))
    {
        return FVM_UNSUPPORTED;
    }
    output->ops = &file_ops;
    output->state = state;
    return FVM_OK;
}


/* EN/RU: Count final provider destruction; stack-owned state is not freed.
 * Посчитать final destruction; stack-owned state не освобождать. */
static void destroy(void *state)
{
    ((Source *)state)->destroyed++;
}
static const DVM_ProviderOps ops = {info, open_file, destroy, DVM_PROVIDER_STABLE_BIND, NULL}; /* Stable capability-relative nodes. */


/* EN/RU: Trusted host mount through actual namespace revision checks.
 * Trusted host mount через настоящие namespace revision checks. */
static void mount(DVM_Namespace *group, DVM_Provider *provider, const char *on)
{
    DVM_NamespacePlan target;
    require(dvm_namespace_plan(group, on, &target) &&
            dvm_namespace_mount_provider(group, &target, provider, "/private-source") == 1, "host mount");
}


/* EN/RU: Exercise retained plans and the real transactional bind implementation.
 * Проверить retained plans и real transactional bind. */
static void bind(DVM_Namespace *group, const char *source, const char *target, unsigned flags, int index, int expected)
{
    DVM_ResourcePlan a, b;
    require(dvm_resource_plan(group, source, 0, &a) && dvm_resource_plan(group, target, 0, &b), "bind plans");
    require(dvm_stack_bind(&a, &b, flags, index) == expected, "bind result");
    dvm_resource_close(&b);
    dvm_resource_close(&a);
}


/* EN/RU: Inspect effective candidate ordering, including inherited child routes.
 * Проверить effective ordering, включая inherited child routes; no guest snapshot copy. */
static void order(DVM_Namespace *group, const char *path, const char *expected)
{
    DVM_ResourcePlan plan, *entry;
    size_t index = 0;
    require(dvm_resource_plan(group, path, 0, &plan), "inspect plan");
    for (entry = &plan; entry; entry = entry->layers)
    {
        FVM_FileInfo metadata;
        require(expected[index] && entry->provider && dvm_resource_raw_info(entry, &metadata) == FVM_OK,
                "unexpected candidate");
        DVM_VfsHandle fd = {0};
        char byte = 0;
        size_t received;
        if (metadata.directory)
        {
            require(dvm_provider_open(entry->provider, "f", "r", &fd) == FVM_OK, "directory source byte");
        }
        else
        {
            require(dvm_provider_open(entry->provider, entry->relative, "r", &fd) == FVM_OK, "file source byte");
        }
        require(dvm_vfs_read(&fd, &byte, 1, &received) == FVM_OK && received == 1 && byte == expected[index], "candidate ordering");
        dvm_vfs_close(&fd);
        index++;
    }
    require(!expected[index], "missing candidate");
    dvm_resource_close(&plan);
}


/* EN/RU: Validate configuration identities, child override, stale and boundary behavior.
 * Проверить config IDs, child override, stale и boundary behavior. */
int main(void)
{
    Source a = {'A',0}, b = {'B',0}, c = {'C',0};
    DVM_Provider *pa = dvm_provider_create(&ops,&a), *pb = dvm_provider_create(&ops,&b), *pc = dvm_provider_create(&ops,&c);
    DVM_Namespace *group = dvm_namespace_create("/", "/", NULL), *copy;
    DVM_ResourcePlan plan, stale;
    size_t position;
    require(group && pa && pb && pc, "construction");
    require(dvm_stack_position(0, 3, 0, &position) && position == 0 &&
            dvm_stack_position(-1, 3, 0, &position) && position == 2 &&
            dvm_stack_position(-1, 3, 1, &position) && position == 3 &&
            dvm_stack_position(-4, 3, 1, &position) && position == 0 &&
            !dvm_stack_position(INT64_MIN, 3, 1, &position) && !dvm_stack_position(4, 3, 1, &position), "signed indexing");
    mount(group, pa, "/a");
    mount(group, pb, "/b");
    mount(group, pc, "/c");
    mount(group, pa, "/view");
    bind(group, "/b", "/view", DVM_NS_AFTER, 0, 1);
    bind(group, "/c", "/view", DVM_NS_INSERT, 1, 1);
    order(group, "/view", "ACB");
    bind(group, "/a", "/view", DVM_NS_INSERT, -1, 1);
    order(group, "/view/f", "ACBA");
    require(dvm_resource_plan(group, "/view/f", 0, &plan) && dvm_resource_plan(group, "/view", 0, &stale), "child plans");
    require(dvm_stack_move(&plan, plan.names.version, plan.layers->layer_id, 0) == 1, "per-file move");
    require(dvm_stack_move(&stale, stale.names.version, stale.layer_id, -1) == 0, "stale move accepted");
    dvm_resource_close(&plan);
    dvm_resource_close(&stale);
    order(group, "/view/f", "CABA");
    order(group, "/view/g", "ACBA");
    order(group, "/view", "ACBA");
    require(dvm_resource_plan(group, "/view/f", 0, &plan), "projected file stack");
    uint64_t id = plan.layer_id;
    require(dvm_stack_move(&plan, plan.names.version, id, -1) == 1, "exact file move");
    dvm_resource_close(&plan);
    order(group, "/view/f", "ABAC");
    require(dvm_resource_plan(group, "/view/f", 0, &plan), "id-preserving view");
    require(plan.layers->layers->layers->layer_id == id, "exact move changed stable ID");
    require(dvm_stack_move(&plan, plan.names.version, id, 9) == -1, "invalid destination accepted");
    dvm_resource_close(&plan);
    require(dvm_resource_plan(group, "/view/f", 0, &plan), "allocation-failure plan");
    fail_after = 2;
    require(dvm_stack_move(&plan, plan.names.version, plan.layer_id, -1) == -1 &&
            dvm_stack_current(group, plan.names.version), "partial allocation committed a stack edit");
    dvm_resource_close(&plan);
    order(group, "/view/f", "ABAC");
    copy = dvm_namespace_copy(group);
    require(copy && dvm_resource_plan(copy, "/view/f", 0, &plan), "namespace copy");
    require(dvm_stack_move(&plan, plan.names.version, plan.layer_id, -1) == 1, "copy move");
    dvm_resource_close(&plan);
    order(copy, "/view/f", "BACA");
    order(group, "/view/f", "ABAC");
    bind(group, "/b/f", "/view/f", DVM_NS_INSERT, -2, 1);
    order(group, "/view/f", "ABABC");
    bind(group, "/b/f", "/view/f", DVM_NS_INSERT | DVM_NS_BEFORE, 0, -1);
    bind(group, "/b/f", "/view/f", DVM_NS_INSERT, 99, -1);
    DVM_NamespacePlan target;
    require(dvm_namespace_plan(group, "/view/f", &target) && dvm_namespace_unmount(group, &target, NULL) == 1, "remove child override");
    order(group, "/view/f", "ACBA");
    dvm_namespace_release(copy);
    dvm_namespace_release(group);
    dvm_provider_release(pa);
    dvm_provider_release(pb);
    dvm_provider_release(pc);
    require(a.destroyed == 1 && b.destroyed == 1 && c.destroyed == 1, "stack lifetime leak");
    puts("PASS indexed provider stack, signed slots, per-file projection, stable IDs, stale refusal, namespace isolation and unmount inheritance");
    return 0;
}
