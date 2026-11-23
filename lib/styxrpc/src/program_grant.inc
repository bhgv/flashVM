/* Copyright (c) 2026 bhgv
 * EN: Issue nonreusable reverse PIDs on recyclable registry slots.
 * RU: Выдача неповторяемых встречных PID в переиспользуемых слотах реестра.
 */

#define GRANT_NODES 3U /* Directory, identity file and UniBus endpoint. */


/* EN: Reserve three slots without changing the visible tree on failure.
 * RU: Подобрать три слота без изменений видимого дерева при отказе. */
static int grant_slots(const StyxRpcPrograms *programs, size_t slots[GRANT_NODES])
{
    size_t count = 0;
    for (size_t i = 2; i < programs->used && count < GRANT_NODES; ++i)
    {
        if (programs->nodes[i].revoked == FREE_GRANT_SLOT)
        {
            slots[count++] = i;
        }
    }
    for (size_t i = programs->used; i < programs->capacity && count < GRANT_NODES; ++i)
    {
        slots[count++] = i;
    }
    return count == GRANT_NODES;
}


/* EN/RU: See program.h. / См. program.h. */
uint32_t styxrpc_program_unibus_grant(StyxRpcPrograms *programs, const StyxRpcResource *resource)
{
    size_t slots[GRANT_NODES];
    if (!resource || !resource->read || !resource->write || !room(programs, 0) || !grant_slots(programs, slots))
    {
        return 0;
    }
    uint64_t pid = programs->next_grant_pid;
    while (pid <= UINT32_MAX && find_pid(programs, (uint32_t)pid))
    {
        ++pid;
    }
    if (pid > UINT32_MAX)
    {
        return 0;
    }

    char text[sizeof("4294967295")];
    snprintf(text, sizeof(text), "%" PRIu32, (uint32_t)pid);
    StyxRpcRegistration *nodes = programs->nodes;
    uint64_t generation = pid << 32;
    uint64_t parent = generation | slots[0];
    init_node(&nodes[slots[0]], parent, 1, text, 0555, 1)->pid = (uint32_t)pid;
    StyxRpcRegistration *identity = init_node(&nodes[slots[1]], generation | slots[1], parent, ".pid", 0444, 0);
    identity->pid = (uint32_t)pid;
    identity->length = strlen(text) + 1;
    init_node(&nodes[slots[2]], generation | slots[2], parent, ".unibus", 0666, 0)->resource = *resource;
    if (slots[2] >= programs->used)
    {
        programs->used = slots[2] + 1;
    }
    programs->next_grant_pid = pid + 1;
    programs->revision++;
    return (uint32_t)pid;
}


/* EN: Hide the entire temporary tree before closing callbacks, then unblock slot reuse.
 * RU: Скрыть всё временное дерево до закрытия колбэков, затем освободить слоты. */
static void revoke_grant(StyxRpcPrograms *programs, uint64_t parent)
{
    size_t slots[GRANT_NODES] = {(uint32_t)parent};
    size_t count = 1;
    programs->nodes[slots[0]].revoked = 1;
    for (size_t i = 2; i < programs->used && count < GRANT_NODES; ++i)
    {
        StyxRpcRegistration *child = &programs->nodes[i];
        if (!child->revoked && child->parent == parent)
        {
            child->revoked = 1;
            slots[count++] = i;
        }
    }
    programs->revision++;
    for (size_t i = 0; i < count; ++i)
    {
        StyxRpcRegistration *entry = &programs->nodes[slots[i]];
        while (entry->opens)
        {
            retire_resource(entry->opens);
        }
    }
    for (size_t i = 0; i < count; ++i)
    {
        programs->nodes[slots[i]].revoked = FREE_GRANT_SLOT;
    }
}
