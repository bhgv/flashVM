/* Copyright (c) 2026 bhgv
 * EN: Mount the two live Styx file grants before the real guest starts.
 * RU: Смонтировать два живых файла Styx до старта настоящего гостя.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "vfs/export.h"
#include "vfs/loader.h"
#include "vfs/module_adapter.h"
#include "vfs/launch/runtime.h"

#include "guest_mount.h"


/* EN: Copy a bounded local adapter, mount it and bind it to the ordinary load name.
 * Borrow the private test group. Retire the staging file before starting the guest.
 * RU: Скопировать локальный адаптер, смонтировать и связать с обычным именем load.
 * Заимствовать приватную группу. Удалить staging-файл до запуска гостя. */
static int publish_adapter(DVM_Namespace *group)
{
    FILE *file = fopen("adapter.dis", "rb");
    unsigned char *bytes = malloc(DVM_MODULE_IMAGE_LIMIT);
    size_t size = file && bytes ? fread(bytes, 1, DVM_MODULE_IMAGE_LIMIT, file) : 0;
    int valid = file && !ferror(file);
    if (file)
    {
        valid = fclose(file) == 0 && valid;
    }
    DVM_Provider *adapter = valid ? dvm_module_adapter(bytes, size) : NULL;
    free(bytes);
    DVM_ResourcePlan plain = {0};
    DVM_Provider *ordinary = dvm_resource_plan(group, "/staging.dis", 0, &plain) ?
        dvm_export_native(&plain, 0) : NULL;
    dvm_resource_close(&plain);
    int mounted = ordinary && dvm_stack_mount(group, ordinary, "/ordinary.dis", 0, 0) == 1 &&
        adapter && dvm_stack_mount(group, adapter, "/published.dis", 0, 0) == 1;
    dvm_provider_release(ordinary);
    dvm_provider_release(adapter);
    if (!mounted)
    {
        return 0;
    }
    DVM_ResourcePlan source = {0};
    DVM_ResourcePlan target = {0};
    int bound = dvm_resource_plan(group, "/published.dis", 0, &source) &&
        dvm_resource_plan(group, "/implementation.dis", 0, &target) &&
        dvm_stack_bind(&source, &target, 0, 0) == 1;
    dvm_resource_close(&target);
    dvm_resource_close(&source);
    return bound && unlink("adapter.dis") == 0;
}


/* EN/RU: Mount the borrowed routes in a private test namespace with local code.
 * Смонтировать заимствованные маршруты в отдельном VFS с локальным кодом. */
int scalar_runtime_run(int argc, char **argv, DVM_Provider *writer, DVM_Provider *reader, int owner)
{
    char cwd[DVM_NS_PATH_LIMIT];
    if (!getcwd(cwd, sizeof(cwd)))
    {
        exit(2);
    }
    DVM_View *view = dvm_view_create(NULL, NULL, NULL);
    if (!view || !dvm_view_map(view, owner ? "/reply" : "/request", writer, ".", FVM_ACCESS_WRITE, 0) ||
        !dvm_view_map(view, owner ? "/request" : "/reply", reader, ".", FVM_ACCESS_READ, 0))
    {
        exit(2);
    }
    DVM_Provider *routes = dvm_view_freeze(view);
    DVM_LaunchHandoff handoff = {.local_role = owner ? DVM_ORIGIN_PROVIDER : DVM_ORIGIN_TERMINAL};
    handoff.group = dvm_namespace_create(cwd, "/", NULL);
    strcpy(handoff.description.module, "scalar-guest.dis");
    if (!routes || !handoff.group || !dvm_namespace_pin(handoff.group) || dvm_stack_mount(handoff.group, routes, "/scalar", 0, 0) != 1)
    {
        fprintf(stderr, "scalar test could not mount route tree\n");
        exit(2);
    }
    dvm_provider_release(routes);
    if (!owner && !publish_adapter(handoff.group))
    {
        fprintf(stderr, "scalar test could not bind trusted adapter\n");
        exit(2);
    }
    int result = dvm_runtime_main_with_handoff(argc, argv, &handoff);
    dvm_namespace_release(handoff.group);
    return result;
}
