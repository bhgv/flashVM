/* Copyright (c) 2026 bhgv
 * EN: Immutable explicit export trees, hidden prefixes and live per-peer policy gates.
 * RU: Immutable explicit export trees, hidden prefixes и live per-peer policy gates.
 */
#include "view_internal.h"
#include "cancel.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: See internal header; forbid traversal, duplicate separators and oversized names.
 * См. internal header; запретить traversal, duplicate separators и oversized names. */
int dvm_view_path(const char *path, int absolute)
{
    const char *part;
    if (!path || !*path || strlen(path) >= DVM_NS_PATH_LIMIT || ((*path == '/') != absolute))
    {
        return 0;
    }
    if (!strcmp(path, absolute ? "/" : "."))
    {
        return 1;
    }
    part = path + absolute;
    while (*part)
    {
        const char *end = strchr(part, '/');
        size_t length = end ? (size_t)(end - part) : strlen(part);
        if (!length || (length == 1 && part[0] == '.') || (length == 2 && part[0] == '.' && part[1] == '.') ||
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


/* EN/RU: Match a complete canonical prefix; slash root covers the whole virtual tree.
 * Сопоставить полный prefix; slash root покрывает virtual tree. */
static int covers(const char *prefix, const char *path)
{
    size_t length = strlen(prefix);
    return !strncmp(prefix, path, length) && (length == 1 || !path[length] || path[length] == '/');
}


/* EN/RU: See export.h; context ownership transfers only after successful allocation.
 * См. export.h; context принять только после successful allocation. */
DVM_View *dvm_view_create(DVM_ViewPolicy policy, void *context, void (*release)(void *))
{
    DVM_View *view = calloc(1, sizeof(*view));
    if (view)
    {
        view->lock = fvm_port_sync_create();
        if (!view->lock)
        {
            free(view);
            return NULL;
        }
        view->references = 1;
        view->policy = policy;
        view->context = context;
        view->release = release;
    }
    return view;
}


/* EN/RU: See export.h; replacement retains new source before dropping the old one.
 * См. export.h; удержать new source до освобождения old source. Builder-only. */
int dvm_view_map(DVM_View *view, const char *path, DVM_Provider *source, const char *relative, unsigned access, unsigned flags)
{
    DVM_ViewRule *rule;
    if (!view || !dvm_view_path(path, 1) || !dvm_view_path(relative, 0) ||
        (access & ~FVM_ACCESS_MASK) || (flags & ~DVM_VIEW_TREE) || (source && !access))
    {
        return 0;
    }
    for (rule = view->rules; rule && strcmp(rule->path, path); rule = rule->next)
    {
    }
    if (!dvm_provider_retain(source))
    {
        return 0;
    }
    if (!rule)
    {
        if (view->count == DVM_VIEW_RULE_LIMIT)
        {
            dvm_provider_release(source);
            return 0;
        }
        rule = calloc(1, sizeof(*rule));
        if (!rule)
        {
            dvm_provider_release(source);
            return 0;
        }
        rule->next = view->rules;
        view->rules = rule;
        view->count++;
    }
    DVM_Provider *old = rule->source;
    rule->source = source;
    strcpy(rule->path, path);
    strcpy(rule->relative, relative);
    rule->access = source ? access : 0;
    rule->flags = flags;
    dvm_provider_release(old);
    return 1;
}


/* EN/RU: See internal header; all source names remain relative to retained capabilities.
 * См. internal header; source names остаются relative к retained capabilities. */
int dvm_view_route(DVM_View *view, const char *path, unsigned access, DVM_ViewRule **rule, char *relative)
{
    DVM_ViewRule *best = NULL;
    if (!dvm_view_path(path, 1) || (access & ~FVM_ACCESS_MASK) ||
        (view->policy && !view->policy(view->context, path, access)))
    {
        return 0;
    }
    for (DVM_ViewRule *entry = view->rules; entry; entry = entry->next)
    {
        if (covers(entry->path, path) && (!best || strlen(entry->path) > strlen(best->path)))
        {
            best = entry;
        }
    }
    *rule = best;
    if (!best)
    {
        for (DVM_ViewRule *entry = view->rules; entry; entry = entry->next)
        {
            if (entry->source && covers(path, entry->path) && !(access & FVM_ACCESS_WRITE))
            {
                relative[0] = 0;
                return 2;
            }
        }
        return 0;
    }
    if (!best->source || (access & ~best->access) ||
        (!(best->flags & DVM_VIEW_TREE) && strcmp(path, best->path)))
    {
        return 0;
    }
    const char *suffix = path + strlen(best->path);
    while (*suffix == '/')
    {
        suffix++;
    }
    size_t base = !strcmp(best->relative, ".") ? 0 : strlen(best->relative);
    size_t tail = strlen(suffix);
    if (base + tail + 2 > DVM_NS_PATH_LIMIT)
    {
        return 0;
    }
    memcpy(relative, best->relative, base);
    if (base && tail)
    {
        relative[base++] = '/';
    }
    memcpy(relative + base, suffix, tail + 1);
    if (!base && !tail)
    {
        strcpy(relative, ".");
    }
    return 1;
}


/* EN/RU: See internal header; an exact mapped node keeps its declared identity.
 * См. internal header; exact mapped node сохраняет declared identity. */
int dvm_view_children(DVM_View *view, const char *path)
{
    for (DVM_ViewRule *rule = view->rules; rule; rule = rule->next)
    {
        if (rule->source && strcmp(path, rule->path) && covers(path, rule->path))
        {
            return 1;
        }
    }
    return 0;
}


/* EN/RU: See internal header; metadata visibility and operational permission are separate.
 * См. internal header; metadata visibility и operational permission раздельны. */
FVM_Result dvm_view_info(DVM_View *view, const char *path, FVM_FileInfo *output)
{
    DVM_ViewRule *rule;
    char relative[DVM_NS_PATH_LIMIT];
    int route = dvm_view_route(view, path, 0, &rule, relative);
    if (!route)
    {
        return FVM_NOT_FOUND;
    }
    if (route == 2)
    {
        memset(output, 0, sizeof(*output));
        output->directory = 1;
        output->mode = 0555;
    }
    else
    {
        FVM_Result status = dvm_provider_info(rule->source, relative, output);
        if (status != FVM_OK && dvm_cancel_requested(dvm_cancel_current()))
        {
            return status;
        }
        if ((status != FVM_OK || !output->directory) && strcmp(path, rule->path) && dvm_view_children(view, path))
        {
            memset(output, 0, sizeof(*output));
            output->directory = 1;
            output->mode = 0555;
        }
        else if (status != FVM_OK)
        {
            return status;
        }
    }
    return viewmask(view, path, output) ? FVM_OK : FVM_NOT_FOUND;
}


/* EN/RU: Apply current visibility/rights and virtual basename to captured metadata.
 * No source lookup; return zero if visibility was revoked during the query.
 * Применить текущую видимость/права и виртуальное имя к захваченным metadata.
 * Без source lookup; ноль при отзыве видимости во время запроса. */
int viewmask(DVM_View *view, const char *path, FVM_FileInfo *output)
{
    DVM_ViewRule *rule;
    char relative[DVM_NS_PATH_LIMIT];
    int route = dvm_view_route(view, path, 0, &rule, relative);
    if (!route)
    {
        return 0;
    }
    unsigned mask = route == 2 ? FVM_ACCESS_READ | FVM_ACCESS_EXEC : rule->access;
    for (unsigned bit = 1; bit <= FVM_ACCESS_READ; bit <<= 1)
    {
        if (view->policy && !view->policy(view->context, path, bit))
        {
            mask &= ~bit;
        }
    }
    output->mode &= ~0777UL | (mask << 6) | (mask << 3) | mask;
    const char *name = strrchr(path, '/');
    strcpy(output->name, name && name[1] ? name + 1 : ".");
    return 1;
}


/* EN/RU: Convert ordinary capability-relative paths into this view's virtual path.
 * Преобразовать capability-relative path в virtual path;1/0, bounded output. */
static int virtual_path(const char *relative, char *path)
{
    if (!dvm_view_path(relative, 0) || strlen(relative) + 2 > DVM_NS_PATH_LIMIT)
    {
        return 0;
    }
    strcpy(path, "/");
    if (strcmp(relative, "."))
    {
        strcat(path, relative);
    }
    return 1;
}


/* EN/RU: Provider adapter for copied metadata, no native host path fallback.
 * Provider adapter metadata, без native host path fallback. */
static FVM_Result info(void *state, const char *relative, FVM_FileInfo *output)
{
    char path[DVM_NS_PATH_LIMIT];
    return virtual_path(relative, path) ? dvm_view_info(state, path, output) : FVM_INVALID_ARGUMENT;
}


/* EN/RU: Provider adapter checks the view gate then delegated effective access.
 * Проверить view gate, затем delegated effective access; unknown stays UNSUPPORTED. */
static FVM_Result access_check(void *state, const char *relative, unsigned access)
{
    char path[DVM_NS_PATH_LIMIT], source[DVM_NS_PATH_LIMIT];
    DVM_ViewRule *rule;
    if (!virtual_path(relative, path))
    {
        return FVM_INVALID_ARGUMENT;
    }
    int route = dvm_view_route(state, path, access, &rule, source);
    return !route ? FVM_ERROR : route == 2 ? FVM_OK : dvm_provider_access(rule->source, source, access);
}


/* EN/RU: Provider adapter opens independent policy-enforcing streams.
 * Provider adapter открывает independent policy-enforcing streams. */
static FVM_Result open_view(void *state, const char *relative, const char *mode, DVM_VfsHandle *output)
{
    char path[DVM_NS_PATH_LIMIT];
    return virtual_path(relative, path) ? dvm_view_open(state, path, mode, output) : FVM_INVALID_ARGUMENT;
}


/* EN/RU: Drop a native state ref; final cleanup follows all detached callbacks.
 * Поглотить state ref; final cleanup после всех detached callbacks. */
void dvm_view_drop(void *state)
{
    DVM_View *view = state;
    fvm_port_sync_lock(view->lock);
    int last = --view->references == 0;
    fvm_port_sync_unlock(view->lock);
    if (!last)
    {
        return;
    }
    while (view->rules)
    {
        DVM_ViewRule *rule = view->rules;
        view->rules = rule->next;
        dvm_provider_release(rule->source);
        free(rule);
    }
    if (view->release)
    {
        view->release(view->context);
    }
    fvm_port_sync_destroy(view->lock);
    free(view);
}


/* EN/RU: Forward source invalidations provider-wide; resource IDs are source-local.
 * Output journal stays pinned under view lock; no target execution/provider I/O.
 * Передать source invalidations без resource ID; output journal удержан view lock. */
static void changed(void *state, const DVM_ChangeEvent *event)
{
    DVM_View *view = state;
    fvm_port_sync_lock(view->lock);
    if (!view->closed && view->journal)
    {
        DVM_ChangeCursor empty = {0};
        DVM_ChangeBatch current;
        if (dvm_changes_pending(view->journal, &empty, DVM_CHANGE_ALL, &current) == 1)
        {
            if (current.event.generation == UINT64_MAX)
            {
                dvm_changes_reconnect(view->journal, current.event.session);
            }
            else
            {
                DVM_ChangeEvent forwarded = {current.event.session, current.event.generation + 1, event->kinds, 0, 0};
                dvm_changes_notify(view->journal, &forwarded);
            }
        }
    }
    fvm_port_sync_unlock(view->lock);
}


/* EN/RU: See export.h; stop publication before detaching source callbacks.
 * См. export.h; прекратить publication до detach source callbacks. */
void dvm_view_discard(DVM_View *view)
{
    if (!view)
    {
        return;
    }
    fvm_port_sync_lock(view->lock);
    view->closed = 1;
    view->journal = NULL;
    fvm_port_sync_unlock(view->lock);
    for (DVM_ViewRule *rule = view->rules; rule; rule = rule->next)
    {
        DVM_NoticeTap *tap = rule->tap;
        rule->tap = NULL;
        dvm_notice_unsubscribe(tap);
    }
    dvm_view_drop(view);
}


/* EN/RU: Consume factory state through the public builder cleanup path.
 * Поглотить factory state общим cleanup builder. */
static void destroy(void *state)
{
    dvm_view_discard(state);
}


/* EN/RU: See export.h; immutable view never promises more identity than its sources.
 * См. export.h; immutable view не обещает больше identity, чем её sources. */
DVM_Provider *dvm_view_freeze(DVM_View *view)
{
    static const DVM_ProviderOps stable = {info, open_view, destroy, DVM_PROVIDER_STABLE_BIND, access_check};
    static const DVM_ProviderOps dynamic = {info, open_view, destroy, 0, access_check};
    int identity = 1;
    unsigned native_flags = DVM_PROVIDER_CANCEL_PATH | DVM_PROVIDER_CANCEL_LIST | DVM_PROVIDER_CANCEL_CAPTURE;
    if (!view)
    {
        return NULL;
    }
    for (DVM_ViewRule *rule = view->rules; rule; rule = rule->next)
    {
        if (rule->source)
        {
            native_flags &= dvm_provider_native_flags(rule->source);
        }
        if (rule->source && !dvm_provider_bindable(rule->source))
        {
            identity = 0;
        }
    }
    if (native_flags & DVM_PROVIDER_CANCEL_PATH)
    {
        native_flags |= DVM_PROVIDER_CANCEL_LAUNCH; /* Our dispatcher checks each selected source before scoped delegation. */
    }
    DVM_Provider *provider = dvm_provider_create_routed(identity ? &stable : &dynamic, view,
        dvm_view_capture, 0, native_flags, dvm_view_launch);
    if (!provider)
    {
        dvm_view_discard(view);
        return NULL;
    }
    view->native_flags = native_flags;
    view->journal = dvm_provider_changes(provider);
    for (DVM_ViewRule *rule = view->rules; rule; rule = rule->next)
    {
        if (!rule->source)
        {
            continue;
        }
        int duplicate = 0;
        for (DVM_ViewRule *prior = view->rules; prior != rule; prior = prior->next)
        {
            duplicate |= prior->source == rule->source;
        }
        if (duplicate)
        {
            continue;
        }
        fvm_port_sync_lock(view->lock);
        view->references++;
        fvm_port_sync_unlock(view->lock);
        rule->tap = dvm_notice_subscribe(dvm_changes_notices(dvm_provider_changes(rule->source)), changed, view, dvm_view_drop);
        if (!rule->tap)
        {
            dvm_view_drop(view);
            dvm_provider_release(provider);
            return NULL;
        }
    }
    return provider;
}
