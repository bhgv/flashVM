/* Copyright (c) 2026 bhgv
 * EN: Build module-private process selectors over exact caller grants.
 * RU: Собрать селекторы процессов модуля из точных полномочий клиентов.
 */
#include "program_view.h"
#include "export.h"
#include "module_adapter.h"
#include "path_cancel.h"
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "launch_files.h"
#include "runqueue.h"
#include "sysevents_internal.h"
#include <string.h>
#include <inttypes.h>
#include <stdio.h>


/* EN: Map one borrowed optional directory into the unpublished view.
 * RU: Добавить заимствованный необязательный каталог в собираемое дерево. */
static int map_tree(DVM_View *view, uint32_t pid, const char *name, DVM_Provider *grant)
{
    if (!grant)
    {
        return 1;
    }
    char path[sizeof("/.prog/4294967295/.vfs")];
    snprintf(path, sizeof(path), "/.prog/%" PRIu32 "/%s", pid, name);
    DVM_Provider *directory = dvm_provider_capture(grant, ".");
    FVM_FileInfo info;
    int mapped = directory && dvm_provider_info(directory, ".", &info) == FVM_OK &&
        info.directory && dvm_view_map(view, path, directory, ".", FVM_ACCESS_MASK, DVM_VIEW_TREE);
    dvm_provider_release(directory);
    return mapped;
}


/* EN: Add identity and the three explicitly granted subtrees for one caller.
 * RU: Добавить идентификатор и три явно выданных поддерева клиента. */
static int map_program(DVM_View *view, const DVM_ProgramView *program)
{
    char pid_text[sizeof("4294967295\n")];
    char path[sizeof("/.prog/4294967295/.sysevents")];
    int size = snprintf(pid_text, sizeof(pid_text), "%" PRIu32 "\n", program->pid);
    DVM_Provider *identity = dvm_module_adapter(pid_text, (size_t)size);
    snprintf(path, sizeof(path), "/.prog/%" PRIu32 "/.pid", program->pid);
    int mapped = identity && dvm_view_map(view, path, identity, ".", FVM_ACCESS_READ, 0);
    dvm_provider_release(identity);
    snprintf(path, sizeof(path), "/.prog/%" PRIu32 "/.sysevents", program->pid);
    mapped = mapped && (!program->events || dvm_view_map(view, path, program->events, ".", FVM_ACCESS_READ, 0));
    snprintf(path, sizeof(path), "/.prog/%" PRIu32 "/.unibus", program->pid);
    return mapped && (!program->unibus ||
        dvm_view_map(view, path, program->unibus, ".", FVM_ACCESS_READ | FVM_ACCESS_WRITE, 0)) &&
        map_tree(view, program->pid, ".fd", program->fds) &&
        map_tree(view, program->pid, ".vfs", program->vfs) &&
        map_tree(view, program->pid, ".var", program->vars);
}


/* EN/RU: See program_view.h. Freeze only the complete module tree.
 * См. program_view.h. Опубликовать только полностью собранное дерево модуля. */
DVM_Provider *dvm_program_view(const DVM_ProgramView *programs, size_t count)
{
    if (!programs || !count || count > 256)
    {
        return NULL;
    }
    DVM_View *view = dvm_view_create(NULL, NULL, NULL);
    if (!view)
    {
        return NULL;
    }
    for (size_t i = 0; i < count; i++)
    {
        size_t previous = 0;
        while (previous < i && programs[previous].pid != programs[i].pid)
        {
            previous++;
        }
        if (previous != i || !programs[i].pid || !map_program(view, &programs[i]))
        {
            dvm_view_discard(view);
            return NULL;
        }
    }
    return dvm_view_freeze(view);
}


/* EN/RU: See program_view.h. Release an invalid capture before returning.
 * См. program_view.h. Освободить неверный захват перед возвратом. */
DVM_Provider *dvm_program_capture(DVM_Provider *root, uint32_t pid)
{
    if (!root || !pid)
    {
        return NULL;
    }
    char path[sizeof(".prog/4294967295")];
    snprintf(path, sizeof(path), ".prog/%" PRIu32, pid);
    DVM_Provider *program = dvm_provider_capture(root, path);
    FVM_FileInfo info;
    if (program && dvm_provider_info(program, ".", &info) == FVM_OK && info.directory)
    {
        return program;
    }
    dvm_provider_release(program);
    return NULL;
}


/* EN: Capture an explicitly mounted caller subtree in native wait scope.
 * Missing mounts are optional. Present but unrepresentable mounts fail closed.
 * RU: Захватить явно смонтированное поддерево в нативном ожидании.
 * Отсутствие допустимо. Неподдерживаемый маунт отменяет вызов. */
static int capture_grant(DVM_Namespace *group, const char *path, DVM_Provider **out)
{
    DVM_ResourcePlan plan = {0};
    if (!dvm_resource_plan(group, path, 0, &plan))
    {
        return 0;
    }
    int valid = 1;
    if (plan.mount_root)
    {
        *out = !plan.union_root && dvm_capture_cancellable(&plan) ?
            dvm_provider_capture(plan.provider, plan.relative) : NULL;
        valid = *out != NULL;
    }
    dvm_resource_close(&plan);
    return valid;
}


/* EN: Describe the borrowed reverse event node without exposing its watched providers.
 * RU: Описать встречный узел событий без раскрытия наблюдаемых провайдеров. */
static FVM_Result events_info(void *state, const char *path, FVM_FileInfo *info)
{
    (void)state;
    if (strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    memset(info, 0, sizeof(*info));
    strcpy(info->name, ".sysevents");
    info->mode = 0444;
    return FVM_OK;
}


/* EN: Open an independent FD for this grant's shared observer, read-only.
 * RU: Открыть FD очереди этой выдачи в режиме чтения. */
static FVM_Result events_open(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    return strcmp(path, ".") ? FVM_NOT_FOUND : dvm_sysevents_poll_open(state, mode, out);
}


/* EN: Retire the observer and release its owned reference with the last grant.
 * RU: Завершить наблюдение и освободить очередь с последней ссылкой на выдачу. */
static void events_destroy(void *state)
{
    dvm_sysevents_retire(state);
    dvm_sysevents_release(state);
}


/* EN: Observe the selected VFS and variable grants.
 * RU: Наблюдать выбранные деревья VFS и переменных. */
static DVM_Provider *events_provider(const DVM_ProgramView *entry)
{
    static const DVM_ProviderOps ops = {.info = events_info, .open = events_open,
        .destroy = events_destroy, .flags = DVM_PROVIDER_STABLE_BIND}; /* Observer owned by its file. */
    DVM_Namespace *group = dvm_namespace_create("/", "/", NULL);
    int ready = group && (!entry->vfs || dvm_program_mount_private(group, entry->vfs, "/.vfs", NULL)) &&
        (!entry->vars || dvm_program_mount_private(group, entry->vars, "/.var", NULL));
    DVM_SysEvents *events = ready ? dvm_sysevents_create(group) : NULL;
    dvm_namespace_release(group);
    DVM_Provider *provider = events ? dvm_provider_create_native(&ops, events, NULL, 1,
        DVM_PROVIDER_CANCEL_PATH | DVM_PROVIDER_CANCEL_CAPTURE) : NULL;
    if (!provider && events)
    {
        events_destroy(events);
    }
    return provider;
}


/* EN: Use the retained caller directory for any grant without a local override.
 * RU: Взять выдачу из удерживаемого каталога клиента, если нет местной замены. */
static int inherit_grant(DVM_Provider *context, const char *name, DVM_Provider **grant)
{
    if (*grant)
    {
        return 1;
    }
    FVM_Result status = dvm_program_child(context, name, grant);
    return status == FVM_OK || status == FVM_NOT_FOUND;
}


/* EN/RU: See program_view.h. Return an owned tree built from the caller's grants.
 * См. program_view.h. Вернуть своё дерево из полномочий клиента. */
DVM_Provider *dvm_program_current_unibus(DVM_Namespace *group, DVM_LaunchFiles *descriptors,
    uint32_t pid, DVM_Provider *unibus)
{
    DVM_ProgramView entry = {.pid = pid, .unibus = unibus};
    int captured = capture_grant(group, "/.fd", &entry.fds) &&
        capture_grant(group, "/.vfs", &entry.vfs) &&
        capture_grant(group, "/.var", &entry.vars);
    Prog *process = currun();
    DVM_Provider *incoming = process && process->pid > 0 && (uint32_t)process->pid == pid ?
        ((DVM_RuntimeProcess *)process)->remote_call_context : NULL;
    if (captured && incoming)
    {
        captured = inherit_grant(incoming, ".fd", &entry.fds) &&
            inherit_grant(incoming, ".vfs", &entry.vfs) && inherit_grant(incoming, ".var", &entry.vars);
    }
    if (captured && !incoming && !entry.vars)
    {
        captured = dvm_launch_vars_view(descriptors, &entry.vars);
    }
    if (captured && !incoming)
    {
        DVM_Provider *explicit_fds = entry.fds;
        entry.fds = dvm_launch_fd_view_with_base(descriptors, explicit_fds, &captured);
        dvm_provider_release(explicit_fds);
    }
    entry.events = captured ? events_provider(&entry) : NULL;
    DVM_Provider *tree = entry.events ? dvm_program_view(&entry, 1) : NULL;
    dvm_provider_release(entry.events);
    dvm_provider_release(entry.fds);
    dvm_provider_release(entry.vfs);
    dvm_provider_release(entry.vars);
    return tree;
}


/* EN/RU: See program_view.h. / См. program_view.h. */
DVM_Provider *dvm_program_current(DVM_Namespace *group, DVM_LaunchFiles *descriptors, uint32_t pid)
{
    return dvm_program_current_unibus(group, descriptors, pid, NULL);
}
