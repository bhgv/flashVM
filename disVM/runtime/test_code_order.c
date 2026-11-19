/* Copyright (c) 2026 bhgv
 * EN: Explicit native terminal/provider code roots for actual loader/cache/dependency tests.
 * RU: Native terminal/provider code roots для настоящих loader/cache/dependency tests.
 */
#include "vfs/boot.h"
#include "vfs/internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int __real_dvm_namespace_boot_initialize(const char *directory);


/* EN/RU: Fail fixture setup explicitly, independent of build mode.
 * Явно завершить fixture setup независимо от build mode. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprintf(stderr, "code order: %s\n", message);
        exit(2);
    }
}


/* EN/RU: Build one owned explicit directory binding on unpublished startup group; no implicit grant.
 * Source path/target/role borrowed; return owned binding, caller links it before publication.
 * Создать owned directory binding в private startup group; caller связывает до publication. */
static DVM_NamespaceBinding *binding(DVM_Namespace *group, const char *source, const char *target, unsigned role)
{
    DVM_ResourcePlan plan;
    check(dvm_resource_plan(group, source, 0, &plan), "native source plan");
    DVM_NamespaceBinding *entry = calloc(1, sizeof(*entry));
    check(entry != NULL, "binding allocation");
    entry->anchor = dvm_anchor_open(plan.anchor, plan.relative);
    check(entry->anchor != NULL, "directory anchor");
    strcpy(entry->on, target);
    strcpy(entry->source, plan.names.native);
    entry->origin.role = role;
    entry->identity = ++group->next_binding;
    entry->head_identity = entry->identity;
    dvm_resource_close(&plan);
    return entry;
}


/* EN/RU: Install equal-name native roots in terminal-first data order before module/guest startup; the ordinary owner starts on the terminal side.
 * Provider provenance is fixture-assigned trusted metadata, never accepted from a wire.
 * Установить same-name roots до старта guest; trusted provenance не берётся из wire. */
int __wrap_dvm_namespace_boot_initialize(const char *directory)
{
    if (!__real_dvm_namespace_boot_initialize(directory))
    {
        return 0;
    }
    DVM_Namespace *group = dvm_namespace_boot_group();
    DVM_NamespaceBinding *terminal = binding(group, "terminal", "/code", DVM_ORIGIN_TERMINAL);
    DVM_NamespaceBinding *provider = binding(group, "provider", "/code", DVM_ORIGIN_PROVIDER);
    DVM_NamespaceBinding *alias = binding(group, "provider", "/provider-code", DVM_ORIGIN_PROVIDER);
    provider->head_identity = terminal->head_identity;
    provider->flags = DVM_NS_AFTER;
    alias->next = group->bindings;
    provider->next = alias;
    terminal->next = provider;
    group->bindings = terminal;
    group->version++;
    return 1;
}
