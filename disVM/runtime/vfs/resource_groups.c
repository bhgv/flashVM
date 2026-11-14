/* Copyright (c) 2026 bhgv
 * EN: Pin initial directories and transfer retained roots to a fresh namespace.
 * RU: Закрепить начальные каталоги и перенести retained roots в новый namespace.
 */
#include "internal.h"
#include <string.h>


/* EN/RU: Copy a bounded native prefix plus absolute virtual suffix; one/zero.
 * Скопировать native prefix и absolute virtual suffix; один/ноль. */
static int native_name(const char *root, const char *suffix, char *out)
{
    size_t base = strlen(root), tail;
    if (base == 1 && *root == '/' && *suffix == '/')
    {
        suffix++;
    }
    tail = strlen(suffix);
    if (base + tail >= DVM_NS_PATH_LIMIT)
    {
        return 0;
    }
    memcpy(out, root, base);
    memcpy(out + base, suffix, tail + 1);
    return 1;
}


/* EN/RU: See resource.h; private unpublished group only, no mutation mutex over I/O.
 * См. resource.h; только unpublished group, mutex не удерживается при I/O. */
int dvm_namespace_pin(DVM_Namespace *group)
{
    if (!native_name(group->root, group->cwd, group->cwd_source) ||
        !native_name(group->root, group->load_directory, group->load_source))
    {
        return 0;
    }
    group->root_anchor = dvm_anchor_open(NULL, group->root);
    if (!group->root_anchor)
    {
        return 0;
    }
    group->cwd_anchor = dvm_anchor_open(group->root_anchor, group->cwd[1] ? group->cwd + 1 : ".");
    group->load_anchor =
        dvm_anchor_open(group->root_anchor, group->load_directory[1] ? group->load_directory + 1 : ".");
    return group->root_anchor && group->cwd_anchor && group->load_anchor;
}


/* EN/RU: See resource.h; no I/O. Caller supplied plan.created owns selected cwd.
 * См. resource.h; без I/O. plan.created удерживает выбранный cwd. */
DVM_Namespace *dvm_namespace_rebase(DVM_ResourcePlan *cwd)
{
    DVM_Namespace *group;
    if (!cwd->created)
    {
        return NULL;
    }
    group = dvm_namespace_create(cwd->names.native, "/", NULL);
    if (!group)
    {
        return NULL;
    }
    if (dvm_anchor_retain(cwd->created))
    {
        group->root_anchor = cwd->created;
        if (dvm_anchor_retain(cwd->created))
        {
            group->cwd_anchor = cwd->created;
            if (dvm_anchor_retain(cwd->created))
            {
                group->load_anchor = cwd->created;
                memcpy(group->cwd_source, cwd->names.native, strlen(cwd->names.native) + 1);
                memcpy(group->load_source, cwd->names.native, strlen(cwd->names.native) + 1);
                return group;
            }
        }
    }
    dvm_namespace_release(group);
    return NULL;
}
