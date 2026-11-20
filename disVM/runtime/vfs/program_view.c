/* Copyright (c) 2026 bhgv
 * EN: Build module-private process selectors over exact caller grants.
 * RU: Собрать селекторы процессов модуля из точных полномочий клиентов.
 */
#include "program_view.h"
#include "export.h"
#include "module_adapter.h"
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
    char path[sizeof("/.prog/4294967295/.pid")];
    int size = snprintf(pid_text, sizeof(pid_text), "%" PRIu32 "\n", program->pid);
    DVM_Provider *identity = dvm_module_adapter(pid_text, (size_t)size);
    snprintf(path, sizeof(path), "/.prog/%" PRIu32 "/.pid", program->pid);
    int mapped = identity && dvm_view_map(view, path, identity, ".", FVM_ACCESS_READ, 0);
    dvm_provider_release(identity);
    return mapped && map_tree(view, program->pid, ".fd", program->fds) &&
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
