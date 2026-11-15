/* Copyright (c) 2026 bhgv
 * EN: Acquire directory identities outside managed admission and namespace locks.
 * RU: Захватывать identity каталогов вне managed admission и namespace mutex.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "kernel.h"
#include "runtime_gc.h"
#include "resource_wait.h"
#include <string.h>


/* EN/RU: See resource_wait.h; plan cleanup covers created handle through reentry.
 * См. resource_wait.h; cleanup плана защищает created handle при reentry. */
int dvm_resource_anchor_wait(DVM_ResourcePlan *plan, int directory, char *component)
{
    char parent[DVM_NS_PATH_LIMIT];
    const char *path = plan->anchor ? plan->relative : plan->names.native;
    int released;
    component[0] = 0;
    if (plan->provider)
    {
        kwerrstr("provider directory anchors are unsupported");
        return 0;
    }
    if (!directory)
    {
        char *slash;
        memcpy(parent, path, strlen(path) + 1);
        slash = strrchr(parent, '/');
        memcpy(component, slash ? slash + 1 : parent, strlen(slash ? slash + 1 : parent) + 1);
        if (slash)
        {
            if (slash == parent)
            {
                slash[1] = 0;
            }
            else
            {
                *slash = 0;
            }
        }
        else
        {
            memcpy(parent, ".", 2);
        }
        path = parent;
    }
    released = dvm_runtime_gc_native_try_leave();
    if (released < 0)
    {
        kwerrstr("directory acquisition cancelled or unavailable");
        return 0;
    }
    dvm_anchor_release(plan->created);
    plan->created = dvm_anchor_open(plan->anchor, path);
    dvm_runtime_gc_native_enter(released);
    if (!plan->created)
    {
        kwerrstr("cannot acquire directory resource");
        return 0;
    }
    return 1;
}
