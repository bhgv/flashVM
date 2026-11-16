/* Copyright (c) 2026 bhgv
 * EN: Captured subtrees retain dynamic policy and explicit overlays without copying a remote tree.
 * RU: Captured subtrees сохраняют dynamic policy и explicit overlays без копирования remote tree.
 */
#include "view_internal.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: Policy proxy owns the original immutable mapping/policy state through native view refs.
 * Policy proxy владеет исходным mapping/policy state через native view refs. */
typedef struct ViewPolicy
{
    DVM_View *original; /* Owned native state ref, no provider back-reference cycle. */
    char prefix[DVM_NS_PATH_LIMIT]; /* Original virtual subtree root for policy checks. */
} ViewPolicy;


/* EN/RU: Translate new view paths to original policy paths; never use old source path for I/O.
 * Перевести новые view paths в original policy paths; прежний source path не используется для I/O. */
static int policy(void *opaque, const char *path, unsigned access)
{
    ViewPolicy *proxy = opaque;
    if (!proxy->original->policy)
    {
        return 1;
    }
    char original[DVM_NS_PATH_LIMIT];
    size_t prefix = !strcmp(proxy->prefix, "/") ? 0 : strlen(proxy->prefix);
    size_t tail = !strcmp(path, "/") ? 0 : strlen(path);
    if (prefix+tail+1 > sizeof(original))
    {
        return 0;
    }
    memcpy(original, proxy->prefix, prefix);
    memcpy(original+prefix, path, tail);
    original[prefix+tail] = 0;
    if (!*original)
    {
        strcpy(original, "/");
    }
    return proxy->original->policy(proxy->original->context, original, access);
}


/* EN/RU: Consume original policy state after every captured view owner/callback retires.
 * Поглотить original policy state после retirement всех captured view owners/callbacks. */
static void release(void *opaque)
{
    ViewPolicy *proxy = opaque;
    dvm_view_drop(proxy->original);
    free(proxy);
}


/* EN/RU: Retain original native state under its lock before any new view becomes visible.
 * Удержать original native state под lock до публикации нового view. */
static ViewPolicy *retain(DVM_View *view, const char *prefix)
{
    ViewPolicy *proxy = calloc(1, sizeof(*proxy));
    if (!proxy)
    {
        return NULL;
    }
    fvm_port_sync_lock(view->lock);
    int valid = !view->closed && view->references != (size_t)-1;
    if (valid)
    {
        view->references++;
    }
    fvm_port_sync_unlock(view->lock);
    if (!valid)
    {
        free(proxy);
        return NULL;
    }
    proxy->original = view;
    strcpy(proxy->prefix, prefix);
    return proxy;
}


/* EN/RU: Map captured base node at dot; stable routes retain their existing contract.
 * Отобразить captured base node в dot; stable routes сохраняют прежний contract. */
static int base(DVM_View *out, DVM_ViewRule *rule, const char *relative)
{
    if (dvm_provider_bindable_at(rule->source, relative))
    {
        return dvm_view_map(out, "/", rule->source, relative, rule->access, rule->flags);
    }
    DVM_Provider *root = dvm_provider_capture(rule->source, relative);
    int valid = root && dvm_view_map(out, "/", root, ".", rule->access, rule->flags);
    dvm_provider_release(root);
    return valid;
}


/* EN/RU: See view_internal.h; preserve overlay definitions, not a frozen list of underlying files.
 * См. view_internal.h; сохранить overlay definitions, не snapshot underlying files. */
DVM_Provider *dvm_view_capture(void *state, const char *relative)
{
    DVM_View *view = state;
    char path[DVM_NS_PATH_LIMIT], source[DVM_NS_PATH_LIMIT];
    DVM_ViewRule *rule;
    if (!dvm_view_path(relative, 0))
    {
        return NULL;
    }
    strcpy(path, "/");
    if (strcmp(relative, "."))
    {
        strcat(path, relative);
    }
    int route = dvm_view_route(view, path, 0, &rule, source);
    if (!route)
    {
        return NULL;
    }
    ViewPolicy *proxy = retain(view, path);
    DVM_View *out = proxy ? dvm_view_create(policy, proxy, release) : NULL;
    if (!out)
    {
        if (proxy)
        {
            release(proxy);
        }
        return NULL;
    }
    if (route == 1 && !base(out, rule, source))
    {
        dvm_view_discard(out);
        return NULL;
    }
    size_t length = strlen(path);
    for (DVM_ViewRule *entry = view->rules; entry; entry = entry->next)
    {
        if (!strcmp(entry->path, path) || strncmp(entry->path, path, length) ||
            (length > 1 && entry->path[length] != '/'))
        {
            continue;
        }
        const char *suffix = entry->path+(length == 1 ? 0 : length);
        if (!dvm_view_map(out, suffix, entry->source, entry->relative, entry->access, entry->flags))
        {
            dvm_view_discard(out);
            return NULL;
        }
    }
    return dvm_view_freeze(out);
}
