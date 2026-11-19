/* Copyright (c) 2026 bhgv
 * EN: Atomic private launch environment construction; no native host root fallback.
 * RU: Атомарная сборка приватного окружения запуска без fallback к native host root.
 */
#include "prepare.h"
#include "../internal.h"
#include "../cancel.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: Check copied identity and role without accepting identity text as credentials.
 * Nickname must be terminated; trusted adapter validates its UTF-8.
 * Проверить identity/role, не считать строки credentials; nickname завершён, UTF-8 проверяет adapter. */
int dvm_launch_origin_valid(const DVM_Origin *origin, unsigned role)
{
    return origin && (role == DVM_ORIGIN_TERMINAL || role == DVM_ORIGIN_PROVIDER) && origin->role == role &&
        dvm_origin_id_valid(origin->system) &&
        dvm_origin_id_valid(origin->export_id) && dvm_origin_id_valid(origin->session) &&
        memchr(origin->nickname, 0, sizeof(origin->nickname));
}


/* EN/RU: Capture a root and validate current directory type; own result in unpublished binding.
 * Borrow source/origin; refuse unsupported scoped callbacks and stop after cancellation.
 * Захватить root и проверить directory type; результат принадлежит unpublished binding.
 * Source/origin borrowed; scoped callback только capable, после отмены остановиться. */
static int acquire(DVM_NamespaceBinding *binding, DVM_Provider *source, const DVM_Origin *origin)
{
    FVM_FileInfo info;
    if (!source || dvm_cancel_requested(dvm_cancel_current()) || (dvm_cancel_current() &&
        !(dvm_provider_native_flags(source) & DVM_PROVIDER_CANCEL_PATH)))
    {
        return 0;
    }
    binding->provider = dvm_provider_capture(source, ".");
    if (!binding->provider || dvm_cancel_requested(dvm_cancel_current()) ||
        dvm_provider_info(binding->provider, ".", &info) != FVM_OK || !info.directory ||
        dvm_cancel_requested(dvm_cancel_current()))
    {
        return 0;
    }
    strcpy(binding->on, "/");
    strcpy(binding->source, "/");
    binding->origin = *origin;
    return 1;
}


/* EN/RU: See prepare.h; ownership is attached before any failure can require cleanup.
 * См. prepare.h; ownership подключено до любого failure, требующего cleanup. */
DVM_Namespace *dvm_launch_prepare(const DVM_LaunchOptions *options)
{
    if (!options || (options->policy != DVM_LAUNCH_TERMINAL_FIRST && options->policy != DVM_LAUNCH_OWNER_FIRST) ||
        !dvm_launch_origin_valid(&options->terminal_origin, DVM_ORIGIN_TERMINAL) ||
        !dvm_launch_origin_valid(&options->provider_origin, DVM_ORIGIN_PROVIDER) ||
        strcmp(options->terminal_origin.session, options->provider_origin.session))
    {
        return NULL;
    }
    DVM_Namespace *group = dvm_namespace_create("/", "/", NULL);
    DVM_NamespaceBinding *first = group ? calloc(1, sizeof(*first)) : NULL;
    if (!first)
    {
        dvm_namespace_release(group);
        return NULL;
    }
    group->bindings = first;
    first->next = calloc(1, sizeof(*first));
    int owner_first = options->policy == DVM_LAUNCH_OWNER_FIRST;
    if (!first->next ||
        !acquire(first, owner_first ? options->provider : options->terminal,
            owner_first ? &options->provider_origin : &options->terminal_origin) ||
        !acquire(first->next, owner_first ? options->terminal : options->provider,
            owner_first ? &options->terminal_origin : &options->provider_origin))
    {
        dvm_namespace_release(group);
        return NULL;
    }
    first->identity = first->head_identity = first->next->head_identity = 1;
    first->next->identity = group->next_binding = 2;
    first->next->flags = DVM_NS_AFTER;
    group->version = 1;
    group->no_native_fallback = 1;
    group->absence_only = 1;
    group->module_stack_order = owner_first;
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        dvm_namespace_release(group);
        return NULL;
    }
    return group;
}
