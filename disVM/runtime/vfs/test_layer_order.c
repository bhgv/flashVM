/* Copyright (c) 2026 bhgv
 * EN: Pure ordered layer publication, copied stacks and stale/exact removal.
 * RU: Pure ordered layer publication, copied stacks и stale/exact removal.
 */
#include "resource.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Fail on contract mismatch; no assertion build mode.
 * Завершить при нарушении contract; без assertion build mode. */
static void require(int ok, const char *label)
{
    if (!ok)
    {
        fprintf(stderr, "union layer: %s\n", label);
        exit(2);
    }
}


/* EN/RU: Capture same-revision pure plans and publish one directory branch.
 * No host I/O. Захватить pure plans одной revision, добавить directory branch. */
static void bind(DVM_Namespace *group, const char *from, unsigned flags)
{
    DVM_ResourcePlan source, target;
    require(dvm_resource_plan(group, from, 0, &source) && dvm_resource_plan(group, "/view", 0, &target), "plans");
    require(dvm_namespace_bind_layers(group, &source, &target, flags, 1, "") == 1, "bind commit");
    require(dvm_namespace_bind_layers(group, &source, &target, flags, 1, "") == 0, "stale bind accepted");
    dvm_resource_close(&source);
    dvm_resource_close(&target);
}


/* EN/RU: Require captured branch order and exact count; return top layer id.
 * Проверить captured order/count; вернуть top layer id. No host I/O. */
static uint64_t order(DVM_Namespace *group, const char *a, const char *b, const char *c)
{
    const char *expected[3] = {a, b, c};
    DVM_ResourcePlan plan, *choice;
    size_t index = 0;
    uint64_t id;
    require(dvm_resource_plan(group, "/view/token", 0, &plan), "order plan");
    id = plan.layer_id;
    for (choice = &plan; choice; choice = choice->layers)
    {
        require(index < 3 && expected[index] && !strcmp(choice->names.native, expected[index]), "captured order");
        index++;
    }
    require(index == (size_t)(!!a + !!b + !!c), "branch count");
    dvm_resource_close(&plan);
    return id;
}


/* EN/RU: Exercise initial before/after, replace, copy and exact/all unmount.
 * Проверить initial before/after, replace, copy и exact/all unmount. */
int main(void)
{
    DVM_Namespace *group = dvm_namespace_create("/", "/", NULL), *copy;
    DVM_NamespacePlan target;
    uint64_t top;
    require(group != NULL, "group");
    bind(group, "/a", DVM_NS_BEFORE);
    bind(group, "/b", DVM_NS_AFTER | DVM_NS_CREATE);
    top = order(group, "/a/token", "/view/token", "/b/token");
    copy = dvm_namespace_copy(group);
    require(copy != NULL, "copy");
    order(copy, "/a/token", "/view/token", "/b/token");
    require(dvm_namespace_plan(group, "/view", &target), "remove plan");
    require(dvm_namespace_unmount_layer(group, &target, top) == 1, "remove top");
    order(group, "/view/token", "/b/token", NULL);
    order(copy, "/a/token", "/view/token", "/b/token");
    bind(group, "/c", DVM_NS_CREATE);
    order(group, "/c/token", NULL, NULL);
    require(dvm_namespace_plan(group, "/view", &target) && dvm_namespace_unmount_layer(group, &target, 0) == 1,
            "remove all");
    order(group, "/view/token", NULL, NULL);
    require(dvm_namespace_plan(copy, "/view", &target) && dvm_namespace_unmount_layer(copy, &target, 0) == 1,
            "remove copied all");
    order(copy, "/view/token", NULL, NULL);
    dvm_namespace_release(copy);
    dvm_namespace_release(group);
    puts("PASS union before/after/baseline/replace/copy/stale/exact/all removal");
    return 0;
}
