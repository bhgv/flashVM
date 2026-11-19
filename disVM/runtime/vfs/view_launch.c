/* Copyright (c) 2026 bhgv
 * EN: Launch discovery through filtered VFS maps and retained live export policy.
 * RU: Launch discovery через filtered VFS maps и retained live export policy.
 */
#include "view_internal.h"
#include "launch/route.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: Guard owns native view state independently of its original provider wrapper; path never re-resolves a namespace.
 * Guard владеет native view state независимо от original provider wrapper; path не выполняет namespace lookup.
 */
typedef struct ViewLaunch
{
    DVM_View *view; /* Independent native ref keeps immutable mappings, source providers and policy context alive. */
    char path[DVM_NS_PATH_LIMIT]; /* Copied virtual node for current policy checks, never a host path. */
} ViewLaunch;


/* EN/RU: Check current permission on this exact immutable mapping without provider I/O or target execution;1/0.
 * Проверить current permission exact immutable mapping без provider I/O/target execution;1/0.
 */
static int permitted(void *state)
{
    ViewLaunch *guard = state;
    DVM_ViewRule *rule;
    char relative[DVM_NS_PATH_LIMIT];
    return dvm_view_route(guard->view, guard->path, FVM_ACCESS_READ | FVM_ACCESS_EXEC, &rule, relative) == 1;
}


/* EN/RU: Consume independent policy pin outside all provider/service/namespace locks, without I/O or guest access.
 * Поглотить independent policy pin вне provider/service/namespace locks, без I/O/guest access.
 */
static void release(void *state)
{
    ViewLaunch *guard = state;
    dvm_view_drop(guard->view);
    free(guard);
}


/* EN/RU: See view_internal.h; preserve exact source mapping and ongoing permission rather than handing out an unguarded inner address.
 * См. view_internal.h; сохранить exact source mapping и ongoing permission, не выдавать unguarded inner address.
 */
struct DVM_LaunchAddress *dvm_view_launch(void *state, const char *relative)
{
    DVM_View *view = state;
    DVM_ViewRule *rule;
    char path[DVM_NS_PATH_LIMIT];
    char source[DVM_NS_PATH_LIMIT];
    if (!dvm_view_path(relative, 0) || strlen(relative) + 2 > sizeof(path))
    {
        return NULL;
    }
    strcpy(path, "/");
    if (strcmp(relative, "."))
    {
        strcat(path, relative);
    }
    if (dvm_view_route(view, path, FVM_ACCESS_READ | FVM_ACCESS_EXEC, &rule, source) != 1)
    {
        return NULL;
    }
    DVM_LaunchAddress *inner = dvm_provider_launch(rule->source, source);
    if (!inner)
    {
        return NULL;
    }
    ViewLaunch *guard = calloc(1, sizeof(*guard));
    if (!guard)
    {
        dvm_launch_address_free(inner);
        return NULL;
    }
    fvm_port_sync_lock(view->lock);
    int valid = view->references && view->references != (size_t)-1;
    if (valid)
    {
        view->references++;
    }
    fvm_port_sync_unlock(view->lock);
    if (!valid)
    {
        free(guard);
        dvm_launch_address_free(inner);
        return NULL;
    }
    guard->view = view;
    strcpy(guard->path, path);
    DVM_LaunchAddress *address = dvm_launch_address_guard(inner, permitted, guard, release);
    if (!address)
    {
        release(guard); /* Failed wrapper consumes inner, but leaves guard-context ownership here. */
    }
    return address;
}
