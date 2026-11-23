/* Copyright (c) 2026 bhgv
 * [GC:] EN: Avail nested channel grants on a provider-owned UniBus.
 * RU: Применение встречных полномочий каналов в UniBus провайдера.
 */


/* EN: One canonical reverse route admitted for this service lifetime.
 * RU: Один канонический встречный маршрут на срок службы. */
typedef struct MessageImport
{
    DVM_GrantIdentity identity; /* Exact PID or module token. */
    Channel *proxy; /* Owned and rooted while the service admits this grant. */
    DVM_ChannelSchema schema; /* Immutable original wire identity and type. */
} MessageImport;


/* EN: Bounded provider-side ledger of reverse message grants.
 * RU: Ограниченный журнал встречных полномочий сообщений провайдера. */
typedef struct DVM_MessageImports
{
    size_t count; /* Number of live imported bindings. */
    MessageImport bindings[DVM_CHANNEL_BUS_SLOTS]; /* Resident-owned table. */
} DVM_MessageImports;


/* EN: Release this service's native reverse route and all canonical roots.
 * RU: Освободить встречный маршрут службы и корни канонических прокси. */
static void release_message_grants(DVM_CallChannels *channels)
{
    DVM_MessageImports *imports = channels->message_imports;
    if (imports)
    {
        for (size_t i = 0; i < imports->count; i++)
        {
            Channel *proxy = imports->bindings[i].proxy;
            ptrdel(D2H(proxy));
            destroy(proxy);
        }
        free(imports);
    }
    dvm_provider_release(channels->reverse);
    dvm_provider_release(channels->forward);
    dvm_provider_release(channels->control);
    channels->message_imports = NULL;
    channels->reverse = NULL;
    channels->forward = NULL;
    channels->control = NULL;
}


/* EN: Pack a grant address followed by its exact key, including the capacity bit.
 * RU: Записать адрес выдачи и ключ с битом ёмкости. */
static int write_grant(const DVM_GrantIdentity *address, uint32_t key,
    void *bytes, size_t capacity, size_t *size)
{
    unsigned char identity[STYX_CHANNEL_EXPORT_TOKEN_SIZE + 4];
    size_t prefix = sizeof(address->token);
    unsigned long tag = STYX_MANAGED_CHANNEL_MODULE_GRANT_TAG;
    if (address->pid)
    {
        uint32_t pid = address->pid;
        identity[0] = pid >> 24;
        identity[1] = pid >> 16;
        identity[2] = pid >> 8;
        identity[3] = pid;
        prefix = 4;
        tag = STYX_MANAGED_CHANNEL_GRANT_TAG;
    }
    else
    {
        memcpy(identity, address->token, prefix);
    }
    identity[prefix] = key >> 24;
    identity[prefix + 1] = key >> 16;
    identity[prefix + 2] = key >> 8;
    identity[prefix + 3] = key;
    return fvm_srecord_write(tag, identity, prefix + 4, bytes, capacity, size) == FVM_S_OK;
}


/* EN: Preserve an admitted reverse identity when echoing it back to its origin.
 * RU: Сохранить принятое встречное полномочие при возврате владельцу. */
static const MessageImport *message_import(const DVM_CallChannels *channels, const Channel *channel)
{
    const DVM_MessageImports *imports = channels->message_imports;
    for (size_t i = 0; imports && i < imports->count; i++)
    {
        if (imports->bindings[i].proxy == channel)
        {
            return &imports->bindings[i];
        }
    }
    return NULL;
}


/* EN: Capture the exact reverse tree while the service is in cancellable native I/O.
 * RU: Захватить точное встречное дерево в отменяемой нативной операции. */
static DVM_Provider *capture_grant(DVM_Provider *root, uint32_t pid)
{
    DVM_VfsWait *wait;
    int native = dvm_vfs_path_enter_cancel(&wait, dvm_cancel_current());
    if (native < 0)
    {
        return NULL;
    }
    DVM_Provider *target = dvm_program_capture(root, pid);
    int cancelled = dvm_vfs_wait_end(wait);
    dvm_runtime_gc_native_enter(native);
    if (cancelled)
    {
        dvm_provider_release(target);
        return NULL;
    }
    return target;
}


/* [GC:] EN: Reuse or create a reverse proxy for the checked identity and type.
 * Return an owned reference. The service ledger holds an independent root.
 * RU: Найти или создать встречный прокси по проверенным идентификатору и типу.
 * Вернуть свою ссылку. Журнал службы держит независимый корень. */
static Channel *import_grant(DVM_CallChannels *channels, const DVM_GrantIdentity *identity,
    const DVM_ChannelSchema *schema)
{
    DVM_MessageImports *imports = channels->message_imports;
    if (!imports)
    {
        imports = channels->message_imports = calloc(1, sizeof(*imports));
        if (!imports)
        {
            return NULL;
        }
    }
    for (size_t i = 0; i < imports->count; i++)
    {
        const MessageImport *entry = &imports->bindings[i];
        if (dvm_grant_identity_equal(&entry->identity, identity))
        {
            return entry->schema.key == schema->key && entry->schema.unbuffered == schema->unbuffered &&
                !strcmp(entry->schema.shape, schema->shape) &&
                dvm_refcount_retain_u(&D2H(entry->proxy)->ref) == DVM_REFCOUNT_LIVE ?
                entry->proxy : NULL;
        }
    }
    if (imports->count == DVM_CHANNEL_BUS_SLOTS)
    {
        return NULL;
    }
    DVM_ReturnedLease *lease = NULL;
    if (identity->pid)
    {
        DVM_Provider *target = capture_grant(channels->reverse, identity->pid);
        lease = dvm_returned_lease_import(target, channels->control, channels->reverse, identity);
        dvm_provider_release(target);
    }
    else
    {
        lease = dvm_returned_lease_import(channels->forward, NULL, NULL, identity);
    }
    Channel *proxy = lease ? lease_proxy(lease, schema) : NULL;
    dvm_returned_lease_release(lease);
    if (!proxy)
    {
        return NULL;
    }
    if (!root_channel(proxy))
    {
        destroy(proxy);
        return NULL;
    }
    imports->bindings[imports->count++] = (MessageImport){.identity = *identity, .schema = *schema, .proxy = proxy};
    return proxy;
}


/* [GC:] EN: Apply the route and key to the correct end of the current lease.
 * RU: Применить маршрут и ключ к нужной стороне текущей аренды. */
static int decode_grant(DVM_CallChannels *channels, const char *type,
    const FVM_SRecord *record, void **result)
{
    size_t prefix = record->tag == STYX_MANAGED_CHANNEL_GRANT_TAG ? 4 : STYX_CHANNEL_EXPORT_TOKEN_SIZE;
    if (record->size != prefix + 4 ||
        (prefix == 4 ? record->payload[0] < 128 || (!channels->persistent && !channels->reverse) :
            !channels->persistent && !channels->forward))
    {
        return 0;
    }
    DVM_GrantIdentity identity = {0};
    if (prefix == 4)
    {
        identity.pid = read_key(record->payload);
    }
    else
    {
        memcpy(identity.token, record->payload, prefix);
    }
    uint32_t wire = read_key(record->payload + prefix);
    DVM_ChannelSchema schema = {.key = wire & INT32_MAX, .unbuffered = wire >> 31};
    if (!schema.key || !dvm_channel_value_shape(type, schema.shape))
    {
        return 0;
    }
    Channel *proxy = channels->persistent ? dvm_returned_lease_original(channels->persistent, &identity, &schema) :
        import_grant(channels, &identity, &schema);
    if (!proxy)
    {
        return 0;
    }
    *result = proxy;
    return 1;
}
