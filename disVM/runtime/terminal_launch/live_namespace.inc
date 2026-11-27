/* Copyright (c) 2026 bhgv
 * EN: Same-group live namespace adapter for the reverse Styx direction.
 * RU: Адаптер живого namespace той же группы для встречного Styx.
 */


/* EN: Keep regular files out of the exact process and execute service routes.
 * RU: Исключить обычные файлы из точных маршрутов портала и исполнения. */
static int visible_shared_path(const char *path)
{
    while (*path)
    {
        if (*path == '/')
        {
            ++path;
            continue;
        }
        size_t length = strcspn(path, "/");
        if ((length == sizeof(".prog") - 1 && !memcmp(path, ".prog", length)) ||
            (length == sizeof(STYXRPC_NODE_EXEC_SERVICE) - 1 && !memcmp(path, STYXRPC_NODE_EXEC_SERVICE, length)) ||
            (length == sizeof(STYXRPC_NODE_EXEC_OWNER) - 1 && !memcmp(path, STYXRPC_NODE_EXEC_OWNER, length)))
        {
            return 0;
        }
        path += length;
    }
    return 1;
}


/* EN: Omit reserved StyxRPC names from the shared ordinary directory listing.
 * RU: Пропустить служебные имена StyxRPC в общем списке обычного каталога. */
static int visible_shared_entry(const char *name)
{
    return !styxrpc_vfs_service_name(name, strlen(name));
}


/* EN: Capture one current absolute path without using the group's cwd.
 * RU: Захватить текущий абсолютный путь независимо от cwd группы. */
static int live_plan(void *group, const char *relative, DVM_ResourcePlan *plan)
{
    char absolute[DVM_NS_PATH_LIMIT];
    int size = snprintf(absolute, sizeof(absolute), "/%s", relative);
    return size > 0 && (size_t)size < sizeof(absolute) &&
        visible_shared_path(absolute) && dvm_resource_plan(group, absolute, 0, plan);
}


/* EN: Resolve and inspect the currently visible relative resource.
 * RU: Разрешить relative и прочитать метаданные текущего ресурса. */
static FVM_Result live_info(void *group, const char *relative, FVM_FileInfo *output)
{
    DVM_ResourcePlan plan;
    if (!live_plan(group, relative, &plan))
    {
        return FVM_NOT_FOUND;
    }
    FVM_Result status = dvm_resource_info(&plan, output);
    if (status == FVM_OK && !strcmp(relative, STYXRPC_MODULE_REF_SCOPE_FILE))
    {
        output->mode &= 0444;
    }
    dvm_resource_close(&plan);
    return status;
}


/* EN: Check effective rights on the current selected resource, without opening it.
 * RU: Проверить фактические права на выбранный ресурс без открытия файла. */
static FVM_Result live_access(void *group, const char *relative, unsigned access)
{
    DVM_ResourcePlan plan;
    if (((access & ~FVM_ACCESS_READ) && !strcmp(relative, STYXRPC_MODULE_REF_SCOPE_FILE)) ||
        !live_plan(group, relative, &plan))
    {
        return FVM_NOT_FOUND;
    }
    plan.requested_access = access;
    FVM_Result status = dvm_union_select(&plan, 0);
    if (status == FVM_OK)
    {
        if (plan.provider)
        {
            status = dvm_provider_access(plan.provider, plan.relative, access);
        }
        else
        {
            status = plan.anchor ? fvm_port_anchor_access_at(dvm_anchor_port(plan.anchor), plan.relative, access) :
                FVM_NOT_FOUND;
        }
    }
    dvm_resource_close(&plan);
    return status;
}


/* EN: Exclude internal StyxRPC components from creation through this view.
 * RU: Исключить создание служебных компонентов StyxRPC в этом дереве. */
static int live_create_path(const char *relative)
{
    while (*relative)
    {
        size_t length = strcspn(relative, "/");
        if (styxrpc_vfs_service_name(relative, length))
        {
            return 0;
        }
        relative += length;
        relative += *relative == '/';
    }
    return 1;
}


/* EN: Remove a node only if its live selection still agrees with the walked fid.
 * RU: Удалить узел, только если выбранный живой узел совпадает с пройденным fid. */
static int live_remove(void *group, const char *relative, const StyxMeta *expected)
{
    DVM_ResourcePlan plan;
    if (!live_create_path(relative) || !live_plan(group, relative, &plan))
    {
        return 0;
    }
    FVM_FileInfo info;
    int removed = dvm_resource_info(&plan, &info) == FVM_OK && !plan.provider && !plan.mount_root &&
        info.directory == expected->directory &&
        (((uint64_t)info.device.high << 32) | info.device.low) == expected->device &&
        (((uint64_t)info.identity.high << 32) | info.identity.low) == expected->identity &&
        dvm_resource_remove(&plan) == FVM_OK;
    dvm_resource_close(&plan);
    return removed;
}


/* EN: Make a native directory and open it using the same resolved parent anchor.
 * RU: Создать нативный каталог и открыть его от того же выбранного anchor. */
static FVM_Result live_mkdir(DVM_ResourcePlan *plan, DVM_VfsHandle *output)
{
    if (!plan->anchor)
    {
        return FVM_UNSUPPORTED;
    }
    FVM_PortAnchor *anchor = dvm_anchor_port(plan->anchor);
    FVM_Result status = fvm_port_anchor_mkdir_at(anchor, plan->relative);
    if (status == FVM_OK)
    {
        FVM_PortDirectory *directory = NULL;
        status = fvm_port_directory_open_at(anchor, plan->relative, &directory);
        if (status == FVM_OK && !dvm_vfs_adopt(output, NULL, directory))
        {
            fvm_port_directory_close(directory);
            status = FVM_ERROR;
        }
    }
    return status;
}


/* EN: Open the current resource, returning an independent file or directory cursor.
 * RU: Открыть текущий ресурс и вернуть независимый файл либо курсор каталога. */
static FVM_Result live_open(void *group, const char *relative, const char *mode, DVM_VfsHandle *output)
{
    DVM_ResourcePlan plan;
    int directory = !strcmp(mode, "mkdir");
    int creating = directory || !strcmp(mode, "wx");
    if ((!strcmp(relative, STYXRPC_MODULE_REF_SCOPE_FILE) && strcmp(mode, "r")) ||
        (creating && !live_create_path(relative)) || !live_plan(group, relative, &plan))
    {
        return FVM_NOT_FOUND;
    }
    plan.requested_access = !strcmp(mode, "r") ? FVM_ACCESS_READ :
        !strcmp(mode, "rw") ? FVM_ACCESS_READ | FVM_ACCESS_WRITE : FVM_ACCESS_WRITE;
    FVM_Result status = dvm_union_select(&plan, creating);
    if (status == FVM_OK && creating && (plan.provider ||
        (plan.layer_id && !(plan.mount_flags & DVM_NS_CREATE))))
    {
        status = FVM_UNSUPPORTED;
    }
    if (status == FVM_OK)
    {
        if (directory)
        {
            status = live_mkdir(&plan, output);
        }
        else if (plan.directory && !strcmp(mode, "r"))
        {
            status = dvm_namespace_directory_filtered(&plan, visible_shared_entry, output);
        }
        else if (plan.provider)
        {
            status = dvm_provider_open(plan.provider, plan.relative, mode, output);
        }
        else
        {
            FVM_PortFile *file = NULL;
            FVM_PortDirectory *directory = NULL;
            status = dvm_resource_open(&plan, !strcmp(mode, "rw") ? "rwo" : mode, &file, &directory);
            if (status == FVM_OK && !dvm_vfs_adopt(output, file, directory))
            {
                fvm_port_file_close(file);
                fvm_port_directory_close(directory);
                status = FVM_ERROR;
            }
        }
    }
    dvm_resource_close(&plan);
    return status;
}


/* EN: Release the group retained by the provider.
 * RU: Освободить ссылку на группу, удержанную провайдером. */
static void live_release(void *group)
{
    dvm_namespace_release(group);
}


/* EN: Retain the guest's group and publish the live adapter.
 * RU: Удержать группу гостя и опубликовать адаптер живого namespace. */
static DVM_Provider *live_namespace(DVM_Namespace *group)
{
    static const DVM_ProviderOps ops = {.info = live_info, .open = live_open,
        .destroy = live_release, .access = live_access};
    if (!group || !dvm_namespace_retain(group))
    {
        return NULL;
    }
    DVM_Provider *provider = dvm_provider_create(&ops, group);
    if (!provider)
    {
        dvm_namespace_release(group);
    }
    return provider;
}
