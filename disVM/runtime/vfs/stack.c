/* Copyright (c) 2026 bhgv
 * EN: Transactional exact-path stack cloning, insertion and permutation.
 * RU: Transactional exact-path stack cloning, insertion и permutation.
 */
#include "internal.h"
#include "stack.h"
#include "cancel.h"
#include "path_cancel.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: See stack.h; count+1 must be representable for insertion slots.
 * См. stack.h; count+1 должен быть representable для insertion slots. */
int dvm_stack_position(int64_t position, size_t count, int insertion, size_t *output)
{
    uint64_t total;
    if (!output || (!insertion && !count) || (insertion && count == (size_t)-1))
    {
        return 0;
    }
    total = count + (insertion != 0);
    if (position < 0)
    {
        uint64_t magnitude = (uint64_t)(-(position + 1)) + 1;
        if (magnitude > total)
        {
            return 0;
        }
        *output = (size_t)(total - magnitude);
    }
    else
    {
        if ((uint64_t)position >= total)
        {
            return 0;
        }
        *output = (size_t)position;
    }
    return 1;
}


/* EN/RU: Dispose owned detached chain after locks, invoking native destructors safely.
 * Освободить detached chain вне locks; native destructors допустимы. */
static void release(DVM_NamespaceBinding *entry)
{
    while (entry)
    {
        DVM_NamespaceBinding *next = entry->next;
        dvm_anchor_release(entry->anchor);
        dvm_provider_release(entry->provider);
        free(entry);
        entry = next;
    }
}


/* EN/RU: Copy one retained route; pin existing native directories by identity.
 * Missing/file branches keep parent+relative name. No namespace locks/managed access.
 * Скопировать route; native directory закрепить по identity, files/missing — parent+name.
 * Return owned binding/NULL; provider needs stable routing or native node capture. */
static DVM_NamespaceBinding *capture(DVM_ResourcePlan *choice, const char *on, unsigned flags, int preserve)
{
    DVM_NamespaceBinding *entry;
    FVM_FileInfo info;
    DVM_ResourcePlan single = *choice;
    single.layers = NULL;
    if (dvm_cancel_requested(dvm_cancel_current()) ||
        (dvm_cancel_current() && !dvm_capture_cancellable(&single)))
    {
        return NULL;
    }
    if ((choice->provider && !dvm_provider_bindable_at(choice->provider, choice->relative) &&
         !dvm_provider_capturable(choice->provider)) || (!choice->provider && !choice->anchor))
    {
        return NULL;
    }
    entry = calloc(1, sizeof(*entry));
    if (!entry)
    {
        return NULL;
    }
    strcpy(entry->on, on);
    strcpy(entry->source, choice->names.native);
    strcpy(entry->component, !strcmp(choice->relative, ".") ? "" : choice->relative);
    entry->flags = flags;
    entry->identity = preserve ? choice->layer_id : 0;
    if (!choice->provider && dvm_resource_raw_info(choice, &info) == FVM_OK && info.directory)
    {
        entry->anchor = dvm_anchor_open(choice->anchor, choice->relative);
        if (!entry->anchor)
        {
            release(entry);
            return NULL;
        }
        entry->component[0] = 0;
    }
    else
    {
        if (!dvm_anchor_retain(choice->anchor))
        {
            release(entry);
            return NULL;
        }
        entry->anchor = choice->anchor;
    }
    if (choice->provider && !dvm_provider_bindable_at(choice->provider, choice->relative))
    {
        entry->provider = dvm_provider_capture(choice->provider, choice->relative);
        if (!entry->provider)
        {
            release(entry);
            return NULL;
        }
        entry->component[0] = 0;
    }
    else
    {
        if (!dvm_provider_retain(choice->provider))
        {
            release(entry);
            return NULL;
        }
        entry->provider = choice->provider;
    }
    return entry;
}


/* EN/RU: Clone all effective candidates, including missing lower file branches.
 * Скопировать все effective candidates, включая missing branches; owned chain/NULL. */
static DVM_NamespaceBinding *clone(DVM_ResourcePlan *plan, const char *on, int preserve, unsigned flags, int source)
{
    DVM_NamespaceBinding *head = NULL, **tail = &head;
    for (DVM_ResourcePlan *choice = plan; choice; choice = choice->layers)
    {
        *tail = capture(choice, on, source ? flags : choice->mount_flags, preserve);
        if (!*tail)
        {
            release(head);
            return NULL;
        }
        tail = &(*tail)->next;
    }
    return head;
}


/* EN/RU: Count borrowed chain, all allocations already completed.
 * Подсчитать borrowed chain после завершения allocations. */
static size_t count(DVM_NamespaceBinding *head)
{
    size_t result = 0;
    for (; head; head = head->next)
    {
        result++;
    }
    return result;
}


/* EN/RU: Splice an owned block at validated zero-based slot in owned chain.
 * Вставить owned block в проверенный zero-based slot owned chain; no allocation. */
static void insert(DVM_NamespaceBinding **head, DVM_NamespaceBinding *block, size_t index)
{
    DVM_NamespaceBinding **link = head, *tail = block;
    while (index--)
    {
        link = &(*link)->next;
    }
    while (tail->next)
    {
        tail = tail->next;
    }
    tail->next = *link;
    *link = block;
}


/* EN/RU: Commit one config revision, retaining old head identity for exact stacks.
 * Consume new chain on every path; old refs are destroyed after namespace unlock.
 * Применить одну revision, сохраняя head ID exact stack. Поглотить new chain всегда;
 * old refs освобождать вне lock. 1 success,0 stale,-1 exhaustion. */
static int publish(DVM_ResourcePlan *target, DVM_NamespaceBinding *head)
{
    DVM_Namespace *group = target->group;
    DVM_NamespaceBinding *old = NULL, **link;
    size_t fresh = 0;
    int result = 0;
    for (DVM_NamespaceBinding *entry = head; entry; entry = entry->next)
    {
        fresh += entry->identity == 0;
    }
    fvm_port_sync_lock(group->lock);
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        result = -1;
    }
    else if (group->version == target->names.version)
    {
        result = -1;
        if (group->version != UINT64_MAX && fresh <= UINT64_MAX - group->next_binding)
        {
            uint64_t identity = target->mount_root ? target->head_id : group->next_binding + 1;
            for (DVM_NamespaceBinding *entry = head; entry; entry = entry->next)
            {
                entry->head_identity = identity;
                if (!entry->identity)
                {
                    entry->identity = ++group->next_binding;
                }
            }
            for (link = &group->bindings; *link;)
            {
                DVM_NamespaceBinding *entry = *link;
                if (!strcmp(entry->on, target->names.path))
                {
                    *link = entry->next;
                    entry->next = old;
                    old = entry;
                }
                else
                {
                    link = &entry->next;
                }
            }
            DVM_NamespaceBinding *tail = head;
            while (tail->next)
            {
                tail = tail->next;
            }
            tail->next = group->bindings;
            group->bindings = head;
            head = NULL;
            group->version++;
            result = 1;
        }
    }
    fvm_port_sync_unlock(group->lock);
    release(old);
    release(head);
    if (result == 1)
    {
        dvm_namespace_changed(group);
    }
    return result;
}


/* EN/RU: Query the first existing candidate's kind without selecting/mutating plan.
 * Получить kind первой existing ветви без изменения plan; one/zero. Native I/O. */
static int kind(DVM_ResourcePlan *plan, int *directory)
{
    FVM_FileInfo info;
    for (DVM_ResourcePlan *choice = plan; choice; choice = choice->layers)
    {
        if (dvm_cancel_requested(dvm_cancel_current()) ||
            (dvm_cancel_current() && !(dvm_provider_native_flags(choice->provider) & DVM_PROVIDER_CANCEL_PATH)))
        {
            return 0;
        }
        if (dvm_resource_raw_info(choice, &info) == FVM_OK)
        {
            *directory = info.directory;
            return 1;
        }
    }
    return 0;
}


/* EN/RU: Check actual staged roots after capture, before namespace publication.
 * Compare the first existing candidate to expected file/directory kind; zero on
 * disappearance or replacement with a different kind. Native metadata outside locks.
 * Проверить actual staged roots после capture: первый existing kind должен совпасть
 * с expected; исчезновение/замена типа дают ноль. Native metadata вне locks. */
static int samekind(DVM_NamespaceBinding *head, int expected)
{
    for (DVM_NamespaceBinding *entry = head; entry; entry = entry->next)
    {
        if (dvm_cancel_requested(dvm_cancel_current()) ||
            (dvm_cancel_current() && !(dvm_provider_native_flags(entry->provider) & DVM_PROVIDER_CANCEL_PATH)))
        {
            return 0;
        }
        FVM_FileInfo info;
        const char *relative = *entry->component ? entry->component : ".";
        FVM_Result status = entry->provider ? dvm_provider_info(entry->provider, relative, &info) :
            entry->anchor ? fvm_port_anchor_info_at(dvm_anchor_port(entry->anchor), relative, &info) :
            fvm_port_path_info(entry->source, &info);
        if (status == FVM_OK)
        {
            return info.directory == expected;
        }
    }
    return 0;
}


/* EN/RU: See stack.h; insert flattened source block, never expose native pointers.
 * См. stack.h; вставить source block целиком, без публикации native pointers. */
int dvm_stack_bind(DVM_ResourcePlan *source, DVM_ResourcePlan *target, unsigned flags, int64_t index)
{
    unsigned order = flags & (DVM_NS_ORDER | DVM_NS_INSERT);
    DVM_NamespaceBinding *head = NULL, *added;
    size_t position = 0;
    int a, b, target_exists;
    if (source->group != target->group || source->names.version != target->names.version)
    {
        return 0;
    }
    if ((flags & ~(DVM_NS_ORDER | DVM_NS_INSERT | DVM_NS_CREATE | DVM_NS_AUTORWX)) ||
        (order != 0 && order != DVM_NS_BEFORE && order != DVM_NS_AFTER && order != DVM_NS_INSERT) ||
        ((flags & DVM_NS_CREATE) && source->mount_root &&
         (source->layers || !(source->mount_flags & DVM_NS_CREATE))) ||
        (order != DVM_NS_INSERT && index) || !kind(source, &a))
    {
        return -1;
    }
    target_exists = kind(target, &b);
    if (target_exists)
    {
        if (a != b)
        {
            return -1;
        }
    }
    else
    {
        int valid = a ? -1 : dvm_stack_file_target(target);
        if (valid != 1)
        {
            return valid;
        }
    }
    added = clone(source, target->names.path, 0, flags, 1);
    if (!added || !samekind(added, a))
    {
        release(added);
        return -1;
    }
    if (order)
    {
        head = clone(target, target->names.path, target->mount_root, 0, 0);
        if (!head || (target_exists && !samekind(head, a)) ||
            (order == DVM_NS_INSERT && !dvm_stack_position(index, count(head), 1, &position)))
        {
            release(head);
            release(added);
            return -1;
        }
        if (order == DVM_NS_AFTER)
        {
            position = count(head);
        }
    }
    insert(&head, added, position);
    return publish(target, head);
}


/* EN/RU: See stack.h; exact move preserves layer IDs; projection creates child-local IDs.
 * См. stack.h; exact move сохраняет IDs, projection создаёт child-local IDs. */
int dvm_stack_move(DVM_ResourcePlan *target, uint64_t revision, uint64_t layer, int64_t index)
{
    size_t length = 0, selected = (size_t)-1, position;
    DVM_NamespaceBinding *head, **link, *entry;
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        return -1;
    }
    if (revision != target->names.version)
    {
        return 0;
    }
    for (DVM_ResourcePlan *choice = target; choice; choice = choice->layers)
    {
        if (choice->layer_id == layer)
        {
            selected = length;
        }
        length++;
    }
    if (selected == (size_t)-1 || !dvm_stack_position(index, length, 0, &position))
    {
        return -1;
    }
    if (position == selected)
    {
        return dvm_stack_current(target->group, revision) ? 1 : 0;
    }
    head = clone(target, target->names.path, target->mount_root, 0, 0);
    if (!head)
    {
        return -1;
    }
    link = &head;
    for (size_t ordinal = 0; ordinal < selected; ordinal++)
    {
        link = &(*link)->next;
    }
    entry = *link;
    *link = entry->next;
    entry->next = NULL;
    insert(&head, entry, position);
    return publish(target, head);
}


/* EN/RU: See stack.h; a query result has no lifetime pin beyond its copied revision.
 * См. stack.h; result не является lifetime pin, только copied revision. */
int dvm_stack_current(DVM_Namespace *group, uint64_t revision)
{
    int current;
    fvm_port_sync_lock(group->lock);
    current = group->version == revision;
    fvm_port_sync_unlock(group->lock);
    return current;
}
