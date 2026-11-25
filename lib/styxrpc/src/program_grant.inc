/* Copyright (c) 2026 bhgv
 * EN: Issue nonreusable reverse PIDs on recyclable registry slots.
 * RU: Выдача неповторяемых встречных PID в переиспользуемых слотах реестра.
 */

#define BUS_GRANT_NODES 3U /* Process, identity and UniBus. */
#define INT_GRANT_NODES 4U /* Process, identity, .var and one cell. */


/* EN: Reserve count slots and return an unused PID without mutating the tree.
 * RU: Подобрать count слотов и свободный PID без изменений дерева. */
static uint32_t reserve_grant(const StyxRpcPrograms *programs, size_t *slots, size_t count)
{
    if (!room(programs, 0))
    {
        return 0;
    }
    size_t used = 0;
    for (size_t i = 2; i < programs->used && used < count; ++i)
    {
        if (programs->nodes[i].revoked == FREE_GRANT_SLOT)
        {
            slots[used++] = i;
        }
    }
    for (size_t i = programs->used; i < programs->capacity && used < count; ++i)
    {
        slots[used++] = i;
    }
    if (used != count)
    {
        return 0;
    }
    uint64_t pid = programs->next_grant_pid;
    while (pid <= UINT32_MAX && find_pid(programs, (uint32_t)pid))
    {
        ++pid;
    }
    return pid <= UINT32_MAX ? (uint32_t)pid : 0;
}


/* EN: Create common PID metadata after the caller has reserved every needed slot.
 * RU: Создать метаданные PID после подбора всех нужных слотов. */
static uint64_t start_grant(StyxRpcPrograms *programs, const size_t *slots, size_t count, uint32_t pid)
{
    char text[sizeof("4294967295")];
    snprintf(text, sizeof(text), "%" PRIu32, pid);
    uint64_t generation = (uint64_t)pid << 32;
    uint64_t parent = generation | slots[0];
    init_node(&programs->nodes[slots[0]], parent, 1, text, 0555, 1)->pid = pid;
    StyxRpcRegistration *identity = init_node(&programs->nodes[slots[1]],
        generation | slots[1], parent, ".pid", 0444, 0);
    identity->pid = pid;
    identity->length = strlen(text) + 1;
    if (slots[count - 1] >= programs->used)
    {
        programs->used = slots[count - 1] + 1;
    }
    programs->next_grant_pid = (uint64_t)pid + 1;
    ++programs->revision;
    return parent;
}


/* EN/RU: See program.h. / См. program.h. */
uint32_t styxrpc_program_unibus_grant(StyxRpcPrograms *programs, const StyxRpcResource *resource)
{
    size_t slots[BUS_GRANT_NODES];
    if (!resource || !resource->read || !resource->write)
    {
        return 0;
    }
    uint32_t pid = reserve_grant(programs, slots, BUS_GRANT_NODES);
    if (pid)
    {
        uint64_t parent = start_grant(programs, slots, BUS_GRANT_NODES, pid);
        init_node(&programs->nodes[slots[2]], (parent & ~UINT64_C(0xffffffff)) | slots[2],
            parent, ".unibus", 0666, 0)->resource = *resource;
    }
    return pid;
}


/* EN/RU: See variable_ref_internal.h. / См. variable_ref_internal.h. */
uint32_t styxrpc_program_int_ref_grant(StyxRpcPrograms *programs, const StyxIntVariable *variable, int writable)
{
    size_t slots[INT_GRANT_NODES];
    if (!variable || !variable->load || (writable != 0 && (writable != 1 || !variable->store)))
    {
        return 0;
    }
    uint32_t pid = reserve_grant(programs, slots, INT_GRANT_NODES);
    if (pid)
    {
        uint64_t parent = start_grant(programs, slots, INT_GRANT_NODES, pid);
        uint64_t generation = (uint64_t)pid << 32;
        uint64_t folder = generation | slots[2];
        init_node(&programs->nodes[slots[2]], folder, parent, ".var", 0555, 1);
        StyxRpcRegistration *cell = init_node(&programs->nodes[slots[3]], generation | slots[3],
            folder, "ref", writable ? 0666 : 0444, 0);
        cell->length = STYX_VARIABLE_INT_SIZE;
        cell->variable = *variable;
        if (!writable)
        {
            cell->variable.store = NULL;
        }
    }
    return pid;
}


/* EN: Hide one generation before retiring any callbacks or reusing its slots.
 * RU: Скрыть поколение до закрытия колбэков и переиспользования слотов. */
static void revoke_grant(StyxRpcPrograms *programs, uint64_t parent)
{
    uint32_t pid = (uint32_t)(parent >> 32);
    size_t slots[INT_GRANT_NODES];
    size_t count = 0;
    for (size_t i = 2; i < programs->used; ++i)
    {
        StyxRpcRegistration *entry = &programs->nodes[i];
        if (!entry->revoked && (uint32_t)(entry->entry.id >> 32) == pid && count < INT_GRANT_NODES)
        {
            entry->revoked = 1;
            slots[count++] = i;
        }
    }
    ++programs->revision;
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


/* EN/RU: See variable_ref_internal.h. / См. variable_ref_internal.h. */
int styxrpc_program_int_ref_revoke(StyxRpcPrograms *programs, const StyxRpcIntRef *ref)
{
    size_t slot = ref ? find_pid(programs, ref->pid) : 0;
    if (!slot || !room(programs, 0) || (ref->writable != 0 && ref->writable != 1))
    {
        return 0;
    }
    uint64_t parent = programs->nodes[slot].entry.id;
    if (parent == slot)
    {
        return 0;
    }
    for (size_t i = 2; i < programs->used; ++i)
    {
        const StyxRpcRegistration *cell = &programs->nodes[i];
        if (!cell->revoked && (uint32_t)(cell->entry.id >> 32) == ref->pid &&
            cell->variable.load && !!cell->variable.store == ref->writable)
        {
            revoke_grant(programs, parent);
            return 1;
        }
    }
    return 0;
}
