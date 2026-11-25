/* Copyright (c) 2026 bhgv
 * EN: Keep shipped imports and assets distinct from the caller's live workspace.
 * RU: Отделить поставляемые импорты и ресурсы от живого каталога вызывающего.
 */
#include "mount_assets.h"
#include "../vfs/export.h"
#include "../vfs/internal.h"
#include <string.h>

#define PAIRED_ASSETS "/ide-assets" /* Ordinary read-only path in the shared VFS. */


/* EN: Allow read and traversal only within the separately confined package.
 * RU: Разрешить только чтение и проход внутри отдельно закреплённого пакета. */
static int allow_read(void *context, const char *path, unsigned access)
{
    (void)context;
    (void)path;
    return !(access & FVM_ACCESS_WRITE);
}


/* EN: Return a provider for a host-selected absolute directory, independent of cwd.
 * RU: Вернуть провайдер выбранного абсолютного каталога независимо от cwd. */
static DVM_Provider *assets_provider(const char *package)
{
    DVM_Namespace *host = dvm_namespace_create(package, "/", NULL);
    DVM_ResourcePlan root;
    DVM_Provider *native = NULL;
    if (host && dvm_namespace_pin(host) && dvm_resource_plan(host, "/", 0, &root))
    {
        native = dvm_export_native(&root, 1);
        dvm_resource_close(&root);
    }
    dvm_namespace_release(host);
    DVM_View *view = native ? dvm_view_create(allow_read, NULL, NULL) : NULL;
    int mapped = view && dvm_view_map(view, "/", native, ".", FVM_ACCESS_READ | FVM_ACCESS_EXEC, DVM_VIEW_TREE);
    dvm_provider_release(native);
    if (!mapped)
    {
        dvm_view_discard(view);
        return NULL;
    }
    return dvm_view_freeze(view);
}


/* EN/RU: See mount_assets.h. / См. mount_assets.h. */
int dvm_terminal_mount_assets(DVM_Namespace *group, const char *package)
{
    if (!package || *package != '/' || strlen(package) >= DVM_NS_PATH_LIMIT)
    {
        return 0;
    }
    DVM_Anchor *directory = dvm_anchor_open(NULL, package);
    DVM_Anchor *imports = directory ? dvm_anchor_confine(directory) : NULL;
    dvm_anchor_release(directory);
    DVM_Provider *assets = assets_provider(package);
    DVM_NamespacePlan target;
    if (!imports || !assets || !dvm_namespace_plan(group, PAIRED_ASSETS, &target) ||
        dvm_namespace_mount_provider(group, &target, assets, PAIRED_ASSETS) != 1)
    {
        dvm_anchor_release(imports);
        dvm_provider_release(assets);
        return 0;
    }
    dvm_provider_release(assets);
    /* EN: The group is private until its reverse server is started.
     * RU: Группа остаётся частной до запуска встречного сервера. */
    dvm_anchor_release(group->load_anchor);
    group->load_anchor = imports;
    strcpy(group->load_directory, PAIRED_ASSETS);
    strcpy(group->load_source, package);
    return 1;
}
