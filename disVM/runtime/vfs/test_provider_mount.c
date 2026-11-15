/* Copyright (c) 2026 bhgv
 * EN: Retained selection/copy/rebind/unmount and factory lifetime oracle.
 * RU: Retained selection/copy/rebind/unmount и проверка lifetime factory.
 */
#include "resource.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Main owns counters; factory owns its marker/alive storage until destroy.
 * Main владеет counters; factory владеет marker/alive storage до destroy. */
typedef struct Factory
{
    unsigned *destroyed; /* Borrowed counter, outlives factory and all handles. */
    unsigned marker; /* Immutable provider identity, independent from display path. */
    int alive; /* One until factory destruction; inner callback requires it. */
} Factory;


/* EN/RU: Require condition or terminate with diagnostic label.
 * Проверить condition либо завершить с диагностической подписью. */
static void require(int ok, const char *label)
{
    if (!ok)
    {
        fprintf(stderr, "provider mount: %s\n", label);
        exit(2);
    }
}


/* EN/RU: Read factory marker through retained opened state; never touch VM memory.
 * Прочитать marker factory через opened state; без VM memory. */
static FVM_Result read_marker(void *state, void *bytes, size_t size, size_t *received)
{
    Factory *factory = *(Factory **)state;
    require(factory->alive, "factory destroyed before opened read");
    if (!size || !bytes || !received)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *(unsigned char *)bytes = (unsigned char)factory->marker;
    *received = 1;
    return FVM_OK;
}


/* EN/RU: Consume independent inner storage before its factory may be released.
 * Поглотить inner storage до освобождения factory. */
static void close_marker(void *state)
{
    require((*(Factory **)state)->alive, "factory destroyed before inner close");
    free(state);
}
static const DVM_VfsOps marker_ops = {NULL, read_marker, NULL, NULL, NULL, close_marker, NULL}; /* Read-only capability. */


/* EN/RU: Return copied marker metadata for retained provider/path; no host lookup.
 * Вернуть copied marker metadata для provider/path; без host lookup. */
static FVM_Result provider_info(void *state, const char *relative, FVM_FileInfo *output)
{
    Factory *factory = state;
    require(factory->alive && relative && *relative, "invalid retained metadata state");
    memset(output, 0, sizeof(*output));
    output->identity.low = factory->marker;
    return FVM_OK;
}


/* EN/RU: Allocate independent handle state; borrow relative/mode and factory.
 * Выделить independent handle state; заимствовать relative/mode/factory. */
static FVM_Result provider_open(void *state, const char *relative, const char *mode, DVM_VfsHandle *output)
{
    Factory **inner;
    (void)relative;
    if (strcmp(mode, "r"))
    {
        return FVM_UNSUPPORTED;
    }
    inner = malloc(sizeof(*inner));
    if (!inner)
    {
        return FVM_OUT_OF_MEMORY;
    }
    *inner = state;
    output->ops = &marker_ops;
    output->state = inner;
    return FVM_OK;
}


/* EN/RU: Consume factory; caller-provided counter remains valid in main.
 * Поглотить factory; counter остаётся валидным у main. */
static void provider_destroy(void *state)
{
    Factory *factory = state;
    factory->alive = 0;
    (*factory->destroyed)++;
    free(factory);
}
static const DVM_ProviderOps factory_ops = {provider_info, provider_open, provider_destroy, DVM_PROVIDER_STABLE_BIND}; /* Immutable trusted factory callbacks. */


/* EN: Create owned factory with immutable marker, borrowed destroyed counter and
 * immutable ops; transfer allocated state on success, fail test on refusal.
 * RU: Создать factory с marker, borrowed counter destroyed и immutable ops;
 * передать новое state при успехе, при отказе завершить тест. */
static DVM_Provider *create(unsigned marker, unsigned *destroyed, const DVM_ProviderOps *ops)
{
    Factory *state = calloc(1, sizeof(*state));
    DVM_Provider *provider;
    require(state != NULL, "factory allocation");
    state->marker = marker;
    state->alive = 1;
    state->destroyed = destroyed;
    provider = dvm_provider_create(ops, state);
    require(provider != NULL, "provider allocation");
    return provider;
}


/* EN/RU: Hold old plan/FD while copies and rebind change new lookups. Same labels
 * must not merge factory identity; last opened handle pins destroyed namespaces.
 * Удержать old plan/FD при copy/rebind. Одинаковые labels не объединяют factory
 * identity; последний FD удерживает factory после удаления всех namespaces. */
int main(void)
{
    unsigned destroyed_a = 0, destroyed_b = 0, destroyed_c = 0;
    const DVM_ProviderOps unstable_ops = {provider_info, provider_open, provider_destroy, 0};
    DVM_Provider *a = create(11, &destroyed_a, &factory_ops), *b = create(22, &destroyed_b, &factory_ops);
    DVM_Namespace *group = dvm_namespace_create("/", "/work", NULL), *copy;
    DVM_NamespacePlan target;
    DVM_ResourcePlan old, fresh;
    DVM_VfsHandle opened = {0};
    FVM_FileInfo metadata;
    unsigned char byte = 0;
    size_t count = 0;
    DVM_ProviderOps unknown_ops = factory_ops;
    Factory unowned = {&destroyed_c, 44, 1};
    DVM_Provider *c;
    unknown_ops.flags = DVM_PROVIDER_STABLE_BIND << 1;
    require(!dvm_provider_create(&unknown_ops, &unowned) && destroyed_c == 0,
            "unknown provider flags accepted or consumed caller state");
    c = create(33, &destroyed_c, &unstable_ops);
    require(group && dvm_namespace_plan(group, "/unstable", &target), "unstable target");
    require(dvm_namespace_mount_provider(group, &target, c, "/unstable") == 1, "unstable mount");
    require(dvm_resource_plan(group, "/unstable/item", 0, &fresh) &&
            dvm_namespace_plan(group, "/unstable-alias", &target), "unstable bind plans");
    require(dvm_namespace_bind_provider(group, &fresh, &target) == -1, "undeclared stable bind accepted");
    dvm_resource_close(&fresh);
    dvm_provider_release(c);
    require(group && dvm_namespace_plan(group, "/mounted", &target), "target plan");
    require(dvm_namespace_mount_provider(group, &target, a, "/same-label") == 1, "initial mount");
    require(dvm_namespace_mount_provider(group, &target, b, "/same-label") == 0, "stale mount accepted");
    require(dvm_resource_plan(group, "/mounted/item", 0, &old), "old plan");
    require(dvm_provider_open(old.provider, old.relative, "r", &opened) == FVM_OK, "old open");
    require(dvm_namespace_plan(group, "/alias", &target), "alias target");
    require(dvm_namespace_bind_provider(group, &old, &target) == 1, "provider alias bind");
    require(dvm_namespace_bind_provider(group, &old, &target) == 0, "stale alias bind accepted");
    copy = dvm_namespace_copy(group);
    require(copy != NULL && dvm_namespace_plan(group, "/mounted", &target), "namespace copy");
    require(dvm_namespace_mount_provider(group, &target, b, "/same-label") == 1, "rebind");
    require(dvm_resource_plan(group, "/mounted/item", 0, &fresh), "fresh plan");
    require(dvm_resource_info(&old, &metadata) == FVM_OK && metadata.identity.low == 11, "old selection redirected");
    require(dvm_resource_info(&fresh, &metadata) == FVM_OK && metadata.identity.low == 22, "new selection stale");
    dvm_resource_close(&fresh);
    require(dvm_resource_plan(group, "/alias/item", 0, &fresh), "alias plan");
    require(dvm_resource_info(&fresh, &metadata) == FVM_OK && metadata.identity.low == 11,
            "rebind redirected frozen provider alias");
    dvm_resource_close(&fresh);
    require(dvm_resource_plan(copy, "/mounted/item", 0, &fresh), "copy plan");
    require(dvm_resource_info(&fresh, &metadata) == FVM_OK && metadata.identity.low == 11, "copy namespace changed");
    dvm_resource_close(&fresh);
    require(dvm_namespace_plan(group, "/mounted", &target) && dvm_namespace_unmount(group, &target, NULL) == 1,
            "unmount");
    dvm_namespace_release(copy);
    dvm_namespace_release(group);
    dvm_provider_release(a);
    dvm_provider_release(b);
    dvm_resource_close(&old);
    require(destroyed_a == 0 && destroyed_b == 1 && destroyed_c == 1, "factory reference imbalance");
    require(dvm_vfs_read(&opened, &byte, 1, &count) == FVM_OK && byte == 11 && count == 1, "retained opened provider");
    dvm_vfs_close(&opened);
    require(destroyed_a == 1 && destroyed_b == 1, "last opened handle did not destroy factory");
    puts("PASS provider mount stale/copy/rebind/unmount, retained plan and final opened-handle lifetime");
    return 0;
}
