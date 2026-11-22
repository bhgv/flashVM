/* Copyright (c) 2026 bhgv
 * EN: Safe native snapshots, reconstruction and batched updates of table owners.
 * RU: Безопасные снимки, восстановление и пакетные обновления владельцев таблиц.
 */


/* EN: Check the exact private descriptor before borrowing the native storage.
 * RU: Проверить точный частный тип до заимствования нативного хранилища. */
static TableOwner *transfer_owner(void *object)
{
    if (!object || object == H || D2H(object)->t != table_type || !((TableOwner *)object)->ready)
    {
        return NULL;
    }
    return object;
}


/* EN: Enter a rooted table under the scheduler mutex without parking. A
 * resident may recurse into its own explicit lease ahead of queued waiters.
 * RU: Войти в укоренённую таблицу под мьютексом планировщика без парковки.
 * Поток вправе повторно зайти под собственный lease до ожидающих. */
static int transfer_enter(TableOwner *owner, Prog *process, uint64_t token)
{
    return (!owner->storage->wait_count || (process && owner->lease_owner == process)) &&
        !fvm_owned_try_enter(&owner->storage->native.guard, token);
}


/* EN: Reuse the caller's lease token or obtain an independent native token.
 * The caller holds the scheduler mutex. Return zero on token exhaustion.
 * RU: Использовать токен lease клиента либо взять отдельный нативный токен.
 * Вызывающий держит мьютекс планировщика. Ноль при исчерпании токенов. */
static int transfer_token(Prog *process, uint64_t *token)
{
    TableActor *actor = table_actor(process, 0);
    if (actor && actor->held)
    {
        *token = actor->token;
        return 1;
    }
    return !fvm_owned_token(token);
}


/* EN/RU: See bigtable_owner.h. / См. bigtable_owner.h. */
int dvm_bigtable_snapshot(void *object, int (*visit)(const Fht *, void *), void *state)
{
    TableOwner *owner = transfer_owner(object);
    uint64_t token;
    int previous;
    if (!owner || !visit || pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &previous))
    {
        return 0;
    }
    Prog *process = currun();
    pthread_mutex_lock(&table_scheduler_lock);
    int valid = transfer_token(process, &token) && transfer_enter(owner, process, token);
    if (valid)
    {
        valid = visit(owner->storage->native.payload, state);
        if (fvm_owned_leave(&owner->storage->native.guard, token))
        {
            sysfatal("unbalanced native table transfer");
        }
    }
    pthread_mutex_unlock(&table_scheduler_lock);
    pthread_setcancelstate(previous, NULL);
    return valid;
}


/* EN/RU: See bigtable_owner.h. / См. bigtable_owner.h. */
void *dvm_bigtable_from_payload(Fht *payload)
{
    if (!payload)
    {
        return NULL;
    }
    TableOwner *owner = H2D(TableOwner *, heapz(table_type));
    int failure;
    TableStorage *storage = dvm_bigtable_storage_create(&failure);
    if (!storage)
    {
        destroy(owner);
        return NULL;
    }
    fht_destroy(storage->native.payload);
    storage->native.payload = payload;
    attach_table(owner, storage);
    return owner;
}


typedef struct StagedTable
{
    TableOwner *owner; /* Borrowed checked owner of a held native guard. */
    Fht *payload; /* Own prepared table or displaced contents after commit. */
} StagedTable;


/* EN/RU: See bigtable_owner.h. / См. bigtable_owner.h. */
int dvm_bigtable_apply_edits(const DVM_BigtableEdit *edits, size_t count)
{
    uint64_t token;
    if (!count)
    {
        return 1;
    }
    StagedTable *stages = edits ? calloc(count, sizeof(*stages)) : NULL;
    int previous;
    if (!stages || pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &previous))
    {
        free(stages);
        return 0;
    }
    Prog *process = currun();
    pthread_mutex_lock(&table_scheduler_lock);
    int ready = transfer_token(process, &token);
    size_t acquired = 0;
    while (ready && acquired < count)
    {
        TableOwner *owner = transfer_owner(edits[acquired].object);
        if (!owner || !edits[acquired].prepare || !transfer_enter(owner, process, token))
        {
            break;
        }
        stages[acquired++].owner = owner;
    }
    int valid = acquired == count;
    for (size_t i = 0; valid && i < count; i++)
    {
        Fht *current = stages[i].owner->storage->native.payload;
        stages[i].payload = edits[i].prepare(current, edits[i].state);
        valid = stages[i].payload != NULL;
    }
    if (valid)
    {
        for (size_t i = 0; i < count; i++)
        {
            Fht **current = &stages[i].owner->storage->native.payload;
            Fht *old = *current;
            *current = stages[i].payload;
            stages[i].payload = old;
        }
    }
    while (acquired)
    {
        TableOwner *owner = stages[--acquired].owner;
        if (fvm_owned_leave(&owner->storage->native.guard, token))
        {
            sysfatal("unbalanced native table transfer");
        }
    }
    pthread_mutex_unlock(&table_scheduler_lock);
    for (size_t i = 0; i < count; i++)
    {
        fht_destroy(stages[i].payload);
    }
    free(stages);
    pthread_setcancelstate(previous, NULL);
    return valid;
}
