/* Copyright (c) 2026 bhgv
 * EN: Prepare and run the ordinary mounted guest namespace.
 * RU: Подготовка и запуск обычного пространства смонтированного гостя.
 */


/* EN: Create and pin the same live group used by the mounted guest and the reverse server.
 * RU: Создать и закрепить общую живую группу гостя и встречного сервера. */
static DVM_Namespace *mounted_group(void)
{
    char cwd[DVM_NS_PATH_LIMIT];
    DVM_Namespace *group = getcwd(cwd, sizeof(cwd)) ? dvm_namespace_create(cwd, "/", NULL) : NULL;
    if (group && !dvm_namespace_pin(group))
    {
        dvm_namespace_release(group);
        group = NULL;
    }
    return group;
}


/* EN: Install the mount in group and run the guest. The caller retains group.
 * RU: Установить маунт в group и запустить гостя. Ссылка group остаётся у вызывающего. */
static int run_mounted(int argc, char **argv, DVM_Provider *mount, const char *path,
    const int *descriptors, DVM_Namespace *group, StyxClient *client, const char *document, int fresh)
{
    if (!group || !dvm_namespace_retain(group))
    {
        return 1;
    }
    DVM_LaunchHandoff handoff = {.local_role = DVM_ORIGIN_TERMINAL, .group = group};
    strcpy(handoff.description.module, "styx-mounted");
    DVM_NamespacePlan target;
    int installed = dvm_namespace_plan(handoff.group, path, &target) &&
        dvm_namespace_mount_provider_create(handoff.group, &target, mount, "/styx-peer") == 1;
    if (installed && descriptors)
    {
        for (unsigned number = 3; number < DVM_LAUNCH_FD_COUNT; number++)
        {
            if (descriptors[number] >= 3)
            {
                handoff.reserved_fds |= UINT32_C(1) << number;
            }
        }
        int captured;
        DVM_Provider *fds = dvm_launch_host_fd_view(descriptors, &captured);
        installed = captured && fds && dvm_namespace_plan(handoff.group, "/.fd", &target) &&
            dvm_namespace_mount_provider(handoff.group, &target, fds, "/launch-descriptors") == 1;
        dvm_provider_release(fds);
    }
    UnibusBinding *binding = installed && client ? dvm_paired_ui_begin(client, document, fresh) : NULL;
    if (installed && client)
    {
        installed = binding && dvm_launch_handoff_unibus(&handoff, binding);
    }
    int result = installed ? dvm_runtime_main_with_handoff(argc, argv, &handoff) : 1;
    if (!installed)
    {
        fprintf(stderr, "disvm: cannot mount Styx at %s\n", path);
    }
    unibusrelease(handoff.unibus);
    dvm_paired_ui_end(binding);
    dvm_namespace_release(handoff.group);
    return result;
}
