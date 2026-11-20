/* Copyright (c) 2026 bhgv
 * EN: Run the scalar guest against the native fixture.
 * RU: Запустить скалярного гостя с нативным стендом.
 */
#include "guest_mount.h"

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


/* EN/RU: Enter the guest with the borrowed mount and saved CLI.
 * Запустить гостя с заимствованным монтом и сохранённой командной строкой. */
void scalar_guest_mounted(DVM_Provider *mount)
{
    runtime_result = scalar_runtime_mounted(runtime_argc, runtime_argv, mount);
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
