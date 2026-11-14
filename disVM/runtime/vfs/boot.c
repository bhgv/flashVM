/* Copyright (c) 2026 bhgv
 * EN: Host startup adapter creates ordinary namespace bindings for bundled files.
 * RU: Host startup adapter создаёт обычные namespace bindings для bundled files.
 */
#include "boot.h"
#include <stdio.h>
#include <unistd.h>

static DVM_Namespace *startup_group; /* Published before guests; retained until every owner joins. */


/* EN/RU: See boot.h. Source snapshots stay native; no filesystem I/O under lock.
 * См. boot.h. Source snapshots native; без файлового I/O под mutex. */
int dvm_namespace_boot_initialize(const char *package_directory)
{
    static const char *names[] = {"bufio.dis",
                                  "string.dis",
                                  "arg.dis",
                                  "readdir.dis",
                                  "debug.dis",
                                  "dis.dis",
                                  "styx.dis",
                                  "styxservers.dis",
                                  "regex.dis"}; /* Existing bundled resource allowlist. */
    char cwd[DVM_NS_PATH_LIMIT], target[DVM_NS_PATH_LIMIT];
    DVM_NamespacePlan source, on;
    DVM_Namespace *group;
    size_t index;
    if (startup_group || !package_directory || !getcwd(cwd, sizeof(cwd)))
    {
        return 0;
    }
    group = dvm_namespace_create("/", cwd, package_directory);
    if (!group)
    {
        return 0;
    }
    for (index = 0; index < sizeof(names) / sizeof(names[0]); index++)
    {
        snprintf(target, sizeof(target), "/dis/lib/%s", names[index]);
        if (!dvm_namespace_module_plan(group, names[index], &source) || !dvm_namespace_plan(group, target, &on) ||
            dvm_namespace_bind(group, &source, &on) != 1)
        {
            dvm_namespace_release(group);
            return 0;
        }
    }
    startup_group = group;
    return 1;
}


/* EN/RU: See boot.h. Pointer lifetime is governed by startup/join, not a GC root.
 * См. boot.h. Lifetime задан startup/join, это не GC root. */
DVM_Namespace *dvm_namespace_boot_group(void)
{
    return startup_group;
}


/* EN/RU: See boot.h; remaining guest references are independent owners.
 * См. boot.h; оставшиеся guest references имеют независимое владение. */
void dvm_namespace_boot_shutdown(void)
{
    dvm_namespace_release(startup_group);
    startup_group = NULL;
}
