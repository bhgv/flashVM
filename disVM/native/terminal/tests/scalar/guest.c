/* Copyright (c) 2026 bhgv
 * EN: Mount the two live Styx file grants before the real guest starts.
 * RU: Смонтировать два живых файла Styx до старта настоящего гостя.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "vfs/export.h"
#include "vfs/launch/runtime.h"

extern int scalar_round_trip(void); /* Starts and joins original Styx peers. */
static int runtime_argc; /* Borrowed through the wrapped runtime invocation. */
static char **runtime_argv; /* Borrowed through the wrapped runtime invocation. */
static int runtime_result; /* Real DisVM exit status. */


/* EN/RU: Mount the borrowed routes in a private test namespace with local code.
 * Смонтировать заимствованные маршруты в отдельном VFS с локальным кодом. */
void scalar_guest(DVM_Provider *request, DVM_Provider *inbox)
{
    char cwd[DVM_NS_PATH_LIMIT];
    if (!getcwd(cwd, sizeof(cwd)))
    {
        exit(2);
    }
    DVM_View *view = dvm_view_create(NULL, NULL, NULL);
    if (!view || !dvm_view_map(view, "/request", request, ".", FVM_ACCESS_WRITE, 0) ||
        !dvm_view_map(view, "/reply", inbox, ".", FVM_ACCESS_READ, 0))
    {
        exit(2);
    }
    DVM_Provider *routes = dvm_view_freeze(view);
    DVM_LaunchHandoff handoff = {.local_role = DVM_ORIGIN_TERMINAL};
    handoff.group = dvm_namespace_create("/", cwd, cwd);
    char target[DVM_NS_PATH_LIMIT];
    int length = snprintf(target, sizeof(target), "%s/scalar", cwd);
    strcpy(handoff.description.module, "scalar-guest.dis");
    if (!routes || !handoff.group || length < 0 || (size_t)length >= sizeof(target) ||
        dvm_stack_mount(handoff.group, routes, target, 0, 0) != 1)
    {
        fprintf(stderr, "scalar test could not mount route tree\n");
        exit(2);
    }
    dvm_provider_release(routes);
    runtime_result = dvm_runtime_main_with_handoff(runtime_argc, runtime_argv, &handoff);
    dvm_namespace_release(handoff.group);
}


/* EN/RU: Borrow CLI arguments while the original peers and VM run.
 * Заимствовать параметры CLI на время работы исходного Styx и VM. */
int __wrap_main(int argc, char **argv)
{
    runtime_argc = argc;
    runtime_argv = argv;
    scalar_round_trip();
    return runtime_result;
}
