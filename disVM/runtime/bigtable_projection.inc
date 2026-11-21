/* Copyright (c) 2026 bhgv
 * EN: Retain scalar-only table storage across native VFS operations.
 * RU: Удерживать скалярное хранилище таблицы между операциями VFS.
 */

/* EN: Pin storage from a borrowed exact managed handle.
 * RU: Удержать хранилище заимствованного точного дескриптора VM. */
TableStorage *dvm_bigtable_storage_retain(void *value)
{
    if (!value || value == H || D2H(value)->t != table_type)
    {
        return NULL;
    }
    TableOwner *owner = value;
    if (!owner->ready)
    {
        return NULL;
    }
    TableStorage *storage = owner->storage;
    unsigned references = __atomic_load_n(&storage->references, __ATOMIC_RELAXED);
    while (references && references < UINT_MAX)
    {
        if (__atomic_compare_exchange_n(&storage->references, &references, references + 1, 1,
            __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
        {
            return storage;
        }
    }
    return NULL;
}


/* EN: Drop one native reference, destroying quiescent storage after its last user.
 * RU: Снять нативную ссылку и уничтожить хранилище после последнего пользователя. */
void dvm_bigtable_storage_release(TableStorage *storage)
{
    if (storage && __atomic_sub_fetch(&storage->references, 1, __ATOMIC_ACQ_REL) == 0)
    {
        if (fvm_table_owner_destroy(&storage->native))
        {
            sysfatal("finalizing active projected table storage");
        }
        free(storage);
    }
}


/* EN: Apply one native VFS request without parking a guest or stealing its held guard.
 * RU: Выполнить запрос VFS без парковки гостя и захвата удерживаемого им guard. */
int dvm_bigtable_storage_request(TableStorage *storage, FvmTableRequest *request)
{
    uint64_t token;
    int previous;
    int status = fvm_owned_token(&token);
    if (status || pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &previous))
    {
        return EINVAL;
    }
    pthread_mutex_lock(&table_scheduler_lock);
    status = storage->wait_count ? EBUSY : fvm_owned_try_enter(&storage->native.guard, token);
    if (!status)
    {
        status = fvm_table_request(&storage->native, token, request);
        fvm_owned_leave(&storage->native.guard, token);
    }
    pthread_mutex_unlock(&table_scheduler_lock);
    pthread_setcancelstate(previous, NULL);
    return status;
}
