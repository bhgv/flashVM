/* Copyright (c) 2026 bhgv
 * EN: Optional native provider advertisement without image reads, authority inference or callback-lock reentry.
 * RU: Optional native provider advertisement без чтения image, вывода authority и callback-lock reentry.
 */
#include "provider_walk.h"
#include "namespace.h"
#include "cancel.h"
#include <string.h>


/* EN/RU: Validate borrowed canonical relative path; dot names the root, traversal/duplicate separators refuse.
 * Return1/0, no I/O/ownership change. / Проверить borrowed canonical relative path; dot — root,
 * traversal/duplicate separators отказывают;1/0, без I/O/ownership change.
 */
static int relative(const char *path)
{
    if (!path || !*path || *path == '/' || strlen(path) >= DVM_NS_PATH_LIMIT)
    {
        return 0;
    }
    if (!strcmp(path, "."))
    {
        return 1;
    }
    for (const char *part = path; *part;)
    {
        const char *end = strchr(part, '/');
        size_t length = end ? (size_t)(end - part) : strlen(part);
        if (!length || (length == 1 && part[0] == '.') || (length == 2 && !strncmp(part, "..", 2)) ||
            (end && !end[1]))
        {
            return 0;
        }
        if (!end)
        {
            break;
        }
        part = end + 1;
    }
    return 1;
}


/* EN/RU: See provider.h; caller's provider pin keeps callback code/state live; no lifetime lock during native discovery.
 * См. provider.h; pin caller удерживает callback code/state, native discovery вне lifetime lock.
 */
struct DVM_LaunchAddress *dvm_provider_launch(DVM_Provider *provider, const char *path)
{
    if (!provider || !provider->launch || !relative(path) || dvm_cancel_requested(dvm_cancel_current()) ||
        (dvm_cancel_current() && !(provider->native_flags & DVM_PROVIDER_CANCEL_LAUNCH)))
    {
        return NULL;
    }
    WalkScope scope = {0};
    if (dvm_provider_walk_enter(provider, path, WALK_LAUNCH, "", &scope) != FVM_OK)
    {
        return NULL;
    }
    struct DVM_LaunchAddress *address = provider->launch(provider->state, path);
    dvm_provider_walk_leave(&scope);
    return address;
}
