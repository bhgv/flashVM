/* Copyright (c) 2026 bhgv
 * EN: Run the scalar guest against the native fixture.
 * RU: Запустить скалярного гостя с нативным стендом.
 */
#include "guest_mount.h"
#include "vfs/launch/runtime.h"
#include "vfs/export.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

extern int scalar_round_trip(void); /* Starts and joins original Styx peers. */
static int runtime_argc; /* Borrowed through the wrapped runtime invocation. */
static char **runtime_argv; /* Borrowed through the wrapped runtime invocation. */
static int runtime_result; /* Real DisVM exit status. */


/* EN/RU: Run the guest with the fixture's borrowed grants.
 * Запустить гостя с заимствованными разрешениями стенда. */
void scalar_guest(DVM_Provider *request, DVM_Provider *inbox)
{
    runtime_result = scalar_runtime_run(runtime_argc, runtime_argv, request, inbox, 0, 0, 0);
}


/* EN/RU: Mount the borrowed remote root and run the guest with ordinary VFS lookup.
 * Смонтировать заимствованный корень и запустить гостя с обычным поиском VFS. */
void scalar_guest_mounted(DVM_Provider *mount)
{
    char cwd[DVM_NS_PATH_LIMIT];
    DVM_LaunchHandoff handoff = {.local_role = DVM_ORIGIN_TERMINAL};
    if (!getcwd(cwd, sizeof(cwd)))
    {
        exit(2);
    }
    handoff.group = dvm_namespace_create(cwd, "/", NULL);
    strcpy(handoff.description.module, "scalar-mounted.dis");
    if (!handoff.group || !dvm_namespace_pin(handoff.group) ||
        dvm_stack_mount(handoff.group, mount, "/peer", 0, 0) != 1)
    {
        fprintf(stderr, "could not install the plain Styx root at /peer\n");
        exit(2);
    }
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
