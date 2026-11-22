/* Copyright (c) 2026 bhgv
 * EN: Atomic private launch environment construction; no native host root fallback.
 * RU: Атомарная сборка приватного окружения запуска без fallback к native host root.
 */
#include "ticket.h"
#include "../internal.h"
#include "../cancel.h"
#include "../export.h"
#include "../program_view.h"
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
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

/* EN/RU: See ticket.h; hold a provider pin across I/O after unlocking the namespace.
 * См. ticket.h; ссылка удерживает провайдер при I/O после снятия блокировки. */
DVM_Provider *dvm_launch_handoff_owner_root(const DVM_LaunchHandoff *handoff)
{
    if (!handoff || !handoff->group)
    {
        return NULL;
    }
    DVM_Namespace *original = handoff->group;
    DVM_Provider *execution = NULL;
    fvm_port_sync_lock(original->lock);
    DVM_NamespaceBinding *first = original->bindings;
    DVM_NamespaceBinding *second = first ? first->next : NULL;
    int owner_first = handoff->description.policy == DVM_LAUNCH_OWNER_FIRST;
    DVM_NamespaceBinding *provider = owner_first ? first : second;
    if (original->version == 1 && original->no_native_fallback && second && !second->next &&
        provider->origin.role == DVM_ORIGIN_PROVIDER && !strcmp(provider->on, "/") &&
        dvm_provider_retain(provider->provider))
    {
        execution = provider->provider;
    }
    fvm_port_sync_unlock(original->lock);
    return execution;
}


/* EN/RU: Keep the accepted execution root while replacing the terminal view.
 * Сохранить принятый корень исполнения при подстановке представления терминала. */
int dvm_launch_handoff_project_terminal(DVM_LaunchHandoff *handoff, DVM_Provider *terminal)
{
    if (!terminal)
    {
        return 0;
    }
    DVM_Provider *execution = dvm_launch_handoff_owner_root(handoff);
    if (!execution)
    {
        return 0;
    }
    DVM_LaunchOptions options = {0};
    options.policy = handoff->description.policy;
    options.terminal = terminal;
    options.provider = execution;
    options.terminal_origin = handoff->description.terminal_origin;
    options.provider_origin = handoff->provider_origin;
    DVM_Namespace *projected = dvm_launch_prepare(&options);
    dvm_provider_release(execution);
    if (!projected)
    {
        return 0;
    }
    DVM_Namespace *original = handoff->group;
    handoff->group = projected;
    dvm_namespace_release(original);
    return 1;
}


/* EN/RU: Resolve and pin the requested decimal PID directory inside the borrowed root.
 * Найти и удержать каталог указанного десятичного PID внутри заимствованного корня. */
static DVM_Provider *capture_program(DVM_Provider *source, uint32_t pid)
{
    char path[sizeof(".prog/4294967295")];
    snprintf(path, sizeof(path), ".prog/%" PRIu32, pid);
    DVM_Provider *program = dvm_provider_capture(source, path);
    FVM_FileInfo info;
    if (program && dvm_provider_info(program, ".", &info) == FVM_OK && info.directory)
    {
        return program;
    }
    dvm_provider_release(program);
    return NULL;
}


/* EN/RU: Confirm ambiguous lookup through a complete bounded directory listing.
 * Подтвердить неоднозначный поиск полным ограниченным списком каталога. */
static FVM_Result missing_program_child(DVM_Provider *program, const char *name)
{
    DVM_VfsHandle directory = {0};
    if (dvm_provider_open(program, ".", "r", &directory) != FVM_OK)
    {
        return FVM_ERROR;
    }
    FVM_Result status = FVM_ERROR;
    size_t remaining = 4096;
    while (!dvm_cancel_requested(dvm_cancel_current()))
    {
        FVM_FileInfo *entries = NULL;
        size_t count = 0;
        FVM_Result read = dvm_vfs_list(&directory, &entries, &count);
        int valid = read == FVM_OK && count <= remaining;
        for (size_t i = 0; valid && i < count; i++)
        {
            valid = strcmp(entries[i].name, name) != 0;
        }
        fvm_port_directory_entries_free(entries);
        if (!valid || !count)
        {
            status = valid ? FVM_NOT_FOUND : FVM_ERROR;
            break;
        }
        remaining -= count;
    }
    dvm_vfs_close(&directory);
    return status;
}


/* EN/RU: Capture one optional process subtree and preserve lookup errors.
 * Захватить необязательное поддерево процесса, сохранив ошибки поиска. */
FVM_Result dvm_program_child(DVM_Provider *program, const char *name, DVM_Provider **out)
{
    if (!program)
    {
        return FVM_NOT_FOUND;
    }
    FVM_FileInfo info;
    FVM_Result status = dvm_provider_info(program, name, &info);
    if (status == FVM_ERROR)
    {
        return missing_program_child(program, name);
    }
    if (status != FVM_OK)
    {
        return status;
    }
    if (!info.directory)
    {
        return FVM_ERROR;
    }
    DVM_Provider *root = dvm_provider_capture(program, name);
    if (!root || dvm_provider_info(root, ".", &info) != FVM_OK || !info.directory)
    {
        dvm_provider_release(root);
        return FVM_ERROR;
    }
    *out = root;
    return FVM_OK;
}


/* EN/RU: Commit both retained roots only when the whole trusted selection succeeds.
 * Установить оба удерживаемых каталога только при полном успехе доверенного выбора. */
int dvm_launch_handoff_program(DVM_LaunchHandoff *handoff, DVM_Provider *source,
    uint32_t pid, uint32_t parent_pid, uint32_t hidden_fds)
{
    if (!handoff || !handoff->group || handoff->program || !source || !pid || pid == parent_pid)
    {
        return 0;
    }
    DVM_Provider *program = capture_program(source, pid);
    DVM_Provider *parent = program && parent_pid ? capture_program(source, parent_pid) : NULL;
    if (!program || (parent_pid && !parent))
    {
        dvm_provider_release(program);
        return 0;
    }
    DVM_Provider *vfs = NULL;
    FVM_Result status = dvm_program_child(program, ".vfs", &vfs);
    if (status == FVM_NOT_FOUND)
    {
        status = dvm_program_child(parent, ".vfs", &vfs);
    }
    if (status != FVM_OK && status != FVM_NOT_FOUND)
    {
        dvm_provider_release(parent);
        dvm_provider_release(program);
        return 0;
    }
    DVM_Provider *vars = NULL;
    status = dvm_program_child(program, ".var", &vars);
    if (status != FVM_OK && status != FVM_NOT_FOUND)
    {
        dvm_provider_release(vfs);
        dvm_provider_release(parent);
        dvm_provider_release(program);
        return 0;
    }
    handoff->program_var = vars;
    handoff->program_vfs = vfs;
    handoff->program = program;
    handoff->program_parent = parent;
    handoff->program_hidden_fds = hidden_fds;
    return 1;
}


/* EN/RU: Add a retained subtree to the unpublished namespace, refusing existing mounts.
 * Добавить удерживаемое поддерево в неопубликованное пространство, исключая конфликт привязок. */
int dvm_program_mount_private(DVM_Namespace *group, DVM_Provider *provider, const char *target,
    const DVM_Origin *origin)
{
    if (!group || !provider || !target || target[0] != '/' || !target[1] ||
        strlen(target) >= DVM_NS_PATH_LIMIT)
    {
        return 0;
    }
    size_t length = strlen(target);
    for (const DVM_NamespaceBinding *old = group->bindings; old; old = old->next)
    {
        if (!strncmp(old->on, target, length) && (!old->on[length] || old->on[length] == '/'))
        {
            return 0;
        }
    }
    DVM_NamespaceBinding *entry = calloc(1, sizeof(*entry));
    if (!entry || group->next_binding == UINT64_MAX || group->version == UINT64_MAX ||
        !dvm_provider_retain(provider))
    {
        free(entry);
        return 0;
    }
    strcpy(entry->on, target);
    strcpy(entry->source, target);
    if (origin)
    {
        entry->origin = *origin;
    }
    entry->provider = provider;
    entry->identity = entry->head_identity = ++group->next_binding;
    entry->next = group->bindings;
    group->bindings = entry;
    group->version++;
    return 1;
}


/* EN/RU: Project the process identity and explicitly granted subtrees.
 * Спроецировать идентификатор процесса и явно выданные поддеревья. */
static DVM_Provider *program_self(const DVM_LaunchHandoff *handoff, DVM_Provider *fds)
{
    DVM_View *view = dvm_view_create(NULL, NULL, NULL);
    if (!view || !dvm_view_map(view, "/.self/.pid", handoff->program, ".pid", FVM_ACCESS_READ, 0) ||
        (fds && !dvm_view_map(view, "/.self/.fd", fds, ".", FVM_ACCESS_MASK, DVM_VIEW_TREE)) ||
        (handoff->program_vfs && !dvm_view_map(view, "/.self/.vfs", handoff->program_vfs,
            ".", FVM_ACCESS_MASK, DVM_VIEW_TREE)) ||
        (handoff->program_var && !dvm_view_map(view, "/.self/.var", handoff->program_var,
            ".", FVM_ACCESS_MASK, DVM_VIEW_TREE)))
    {
        dvm_view_discard(view);
        return NULL;
    }
    return dvm_view_freeze(view);
}


/* EN/RU: Stage private aliases before publishing the launched process.
 * Подготовить частные пути до публикации запущенного процесса. */
DVM_Namespace *dvm_launch_program_namespace_with_fds(const DVM_LaunchHandoff *handoff, DVM_Provider *fds)
{
    if (!handoff || !handoff->group)
    {
        return NULL;
    }
    if (!handoff->program)
    {
        return dvm_namespace_retain(handoff->group) ? handoff->group : NULL;
    }
    DVM_Namespace *group = dvm_namespace_copy(handoff->group);
    DVM_Provider *self = group ? program_self(handoff, fds) : NULL;
    int mounted = self && dvm_program_mount_private(group, self, "/.prog", &handoff->description.terminal_origin) &&
        (!handoff->program_vfs || dvm_program_mount_private(group, handoff->program_vfs, "/.vfs", &handoff->description.terminal_origin));
    dvm_provider_release(self);
    if (!mounted)
    {
        dvm_namespace_release(group);
        return NULL;
    }
    return group;
}

/* EN/RU: See ticket.h. / См. ticket.h. */
DVM_Namespace *dvm_launch_program_namespace(const DVM_LaunchHandoff *handoff)
{
    return dvm_launch_program_namespace_with_fds(handoff, NULL);
}
