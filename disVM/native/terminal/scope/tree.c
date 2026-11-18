/* Copyright (c) 2026 bhgv
 * EN: Native-only scope creation and atomic sibling remapping, independent directional generations.
 * RU: Native-only создание scope и атомарное sibling remapping с отдельными directional generations.
 */
#include "internal.h"


/* EN/RU: Append an owned group/client under borrowed pinned parent; output handle adopted only on success.
 * Добавить group/client под pinned parent, output handle меняется только при успехе.
 */
static StyxScope *create(StyxScope *parent, StyxScopeClient **client)
{
    if (!parent || parent->leaf || parent->depth >= STYX_SCOPE_DEPTH)
    {
        return NULL;
    }
    StyxScope *scope = calloc(1, sizeof(*scope)); /* Private until tree admission. */
    StyxScopeClient *inbox = client ? calloc(1, sizeof(*inbox)) : NULL; /* Separate application capability. */
    if (!scope || (client && !inbox))
    {
        free(scope);
        free(inbox);
        return NULL;
    }
    ScopeHub *hub = parent->hub; /* Host pins parent/root throughout call. */
    pthread_mutex_lock(&hub->lock);
    if (hub->reason)
    {
        pthread_mutex_unlock(&hub->lock);
        free(scope);
        free(inbox);
        return NULL;
    }
    scope->hub = hub;
    scope->parent = parent;
    scope->depth = parent->depth + 1;
    scope->leaf = client != NULL;
    scope->client = inbox;
    scope->sibling = parent->children;
    parent->children = scope;
    if (inbox)
    {
        inbox->scope = scope;
        *client = inbox;
    }
    pthread_mutex_unlock(&hub->lock);
    return scope;
}


/* EN/RU: See scope.h; native group authority, no client capability or automatic tag allocation.
 * См. scope.h; native group authority без client capability и автоматического tag allocation.
 */
StyxScope *styxscopegroup(StyxScope *parent)
{
    return create(parent, NULL);
}


/* EN/RU: See scope.h; output required, native node and client inbox have different opaque types.
 * См. scope.h; output обязателен, native node и client inbox имеют разные opaque types.
 */
StyxScope *styxscopeclient(StyxScope *parent, StyxScopeClient **client)
{
    return client ? create(parent, client) : NULL;
}


/* EN/RU: Borrow list, find one child's proposed final tag; caller holds hub lock, no mutation.
 * Найти final tag одного child в borrowed list под hub lock, без mutation.
 */
static unsigned long finaltag(StyxScope *scope, unsigned direction, const StyxScopeAssignment *list, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        if (list[i].scope == scope)
        {
            return list[i].tag;
        }
    }
    return scope->tags[direction];
}


/* EN/RU: See scope.h; atomic transaction may swap sibling tags without transient duplicates or gaps.
 * См. scope.h; atomic transaction меняет sibling tags без transient duplicates либо gaps.
 */
int styxscopeassign(StyxScope *parent, unsigned direction, const StyxScopeAssignment *list, size_t count)
{
    if (!parent || parent->leaf || direction > STYX_SCOPE_TX || (count && !list))
    {
        return 0;
    }
    ScopeHub *hub = parent->hub; /* Pinned by caller for entire transaction. */
    pthread_mutex_lock(&hub->lock);
    int valid = !hub->reason; /* Never remap a retired broker. */
    for (size_t i = 0; valid && i < count; i++)
    {
        StyxScope *scope = list[i].scope; /* Borrowed host authority, not an on-wire pointer. */
        valid = scope && scope->parent == parent && scope->hub == hub && list[i].tag <= 0xffffffffUL &&
            scope->generations[direction] == list[i].expected && list[i].expected != ~0ULL;
        for (size_t j = 0; valid && j < i; j++)
        {
            valid = list[i].scope != list[j].scope;
        }
    }
    for (StyxScope *a = parent->children; valid && a; a = a->sibling)
    {
        unsigned long tag = finaltag(a, direction, list, count); /* Proposed whole sibling namespace. */
        for (StyxScope *b = a->sibling; valid && tag && b; b = b->sibling)
        {
            valid = tag != finaltag(b, direction, list, count);
        }
    }
    if (valid)
    {
        for (size_t i = 0; i < count; i++)
        {
            list[i].scope->tags[direction] = list[i].tag;
            list[i].scope->generations[direction]++;
        }
    }
    pthread_mutex_unlock(&hub->lock);
    return valid;
}


/* EN/RU: See scope.h; copy current local binding, no transport or application queue access.
 * См. scope.h; copied current binding, без transport либо application queue access.
 */
int styxscopebinding(StyxScope *scope, unsigned direction, unsigned long *tag, unsigned long long *generation)
{
    if (!scope || !scope->parent || direction > STYX_SCOPE_TX || !tag || !generation)
    {
        return 0;
    }
    pthread_mutex_lock(&scope->hub->lock);
    *tag = scope->tags[direction];
    *generation = scope->generations[direction];
    pthread_mutex_unlock(&scope->hub->lock);
    return 1;
}


/* EN/RU: See internal.h; root owns all nodes/handles and every unread copied record.
 * См. internal.h; root владеет nodes/handles и всеми непрочитанными copies.
 */
void scopeclear(StyxScope *scope)
{
    while (scope)
    {
        StyxScope *next = scope->sibling; /* Transfer owned continuation before disposal. */
        scopeclear(scope->children);
        while (scope->head)
        {
            ScopeMessage *message = scope->head; /* One owned unread message. */
            scope->head = message->next;
            free(message);
        }
        free(scope->client);
        free(scope);
        scope = next;
    }
}
