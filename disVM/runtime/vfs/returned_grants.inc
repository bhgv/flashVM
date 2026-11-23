/* Copyright (c) 2026 bhgv
 * [GC:] EN: Publish lease-scoped message channels on the existing reverse Styx session.
 * RU: Публикация каналов из сообщений в существующей встречной сессии Styx.
 */


/* EN: Reserve reverse-only identities above the signed range of guest PIDs.
 * RU: Резервировать встречные идентификаторы за пределами знаковых PID. */
static uint32_t next_grant_pid = UINT32_C(0x80000000); /* Monotonic across sessions. */


static unsigned char grant_graph_locked; /* Serializes bounded traversal and adjacency changes. */


/* EN: Take the process-wide native dependency lock without a managed safepoint.
 * RU: Захватить общий нативный замок зависимостей без перехода через safe point. */
static void lock_grant_graph(void)
{
    while (__atomic_test_and_set(&grant_graph_locked, __ATOMIC_ACQUIRE))
    {
        fvm_port_thread_sleep_us(1);
    }
}


/* EN: Release after the adjacency update or validation completes.
 * RU: Отпустить после проверки или обновления зависимостей. */
static void unlock_grant_graph(void)
{
    __atomic_clear(&grant_graph_locked, __ATOMIC_RELEASE);
}


/* EN: Reject a route back to target, conservatively bounding search work.
 * The graph is locked and every edge pins its target through the original proxy.
 * RU: Отклонить путь обратно к target, ограничив работу обхода.
 * Граф закрыт замком. Каждое ребро удерживает цель через исходный прокси. */
static int acyclic_grant(const DVM_ReturnedLease *source, const DVM_ReturnedLease *target,
    unsigned *budget)
{
    if (!source)
    {
        return 1;
    }
    if (source == target || !*budget)
    {
        return 0;
    }
    --*budget;
    for (size_t i = 0; i < source->grant_count; i++)
    {
        if (!acyclic_grant(source->grants[i].source, target, budget))
        {
            return 0;
        }
    }
    return 1;
}


/* EN: Stop a private pump, revoke its published tree and retire its object pin.
 * RU: Остановить частную службу, отозвать её дерево и снять ссылку на объект. */
static void retire_grant(ReturnedGrant *grant)
{
    dvm_cancel_request(grant->cancel);
#ifdef DVM_TERMINAL_LAUNCH
    styxchannel_export_revoke(grant->control);
#endif
    dvm_provider_release(grant->control);
    if (grant->publication)
    {
        grant->binding.end(grant->binding.state, grant->publication);
    }
    dvm_provider_release(grant->tree);
    dvm_cancel_release(grant->cancel);
    if (grant->root.object)
    {
        /* Managed tickets enqueue retirement from native or collector threads. */
        DVM_OwnerTransfer root = grant->root;
        grant->root = (DVM_OwnerTransfer){0};
        root.ops->drop(root.object);
    }
}


/* EN: Unlink a last-owner lease from the graph before retiring its pinned edges.
 * RU: Снять аренду последнего владельца с графа перед освобождением её рёбер. */
static void retire_grants(DVM_ReturnedLease *lease)
{
    lock_grant_graph();
    size_t count = lease->grant_count;
    lease->grant_count = 0;
    unlock_grant_graph();
    for (size_t i = 0; i < count; i++)
    {
        retire_grant(&lease->grants[i]);
    }
}


/* EN: Allocate one nonreusable native identity. Zero signals exhaustion.
 * RU: Выделить непереиспользуемый нативный идентификатор. Ноль означает исчерпание. */
static uint32_t grant_pid(void)
{
    uint32_t next = __atomic_load_n(&next_grant_pid, __ATOMIC_RELAXED);
    while (next != UINT32_MAX)
    {
        if (__atomic_compare_exchange_n(&next_grant_pid, &next, next + 1, 1,
                __ATOMIC_RELAXED, __ATOMIC_RELAXED))
        {
            return next;
        }
    }
    return 0;
}


/* EN: Publish a reverse PID tree and prepare the mounted route for peer grants.
 * RU: Выдать встречное дерево PID и подготовить маршрут для выдач партнёра. */
static int publish_reverse(DVM_ReturnedLease *lease, ReturnedGrant *grant,
    DVM_Provider *provider, DVM_ChannelTransport *transport)
{
    if (!dvm_provider_caller_binding(lease->endpoint, &grant->binding))
    {
        return 0;
    }
    grant->identity.pid = grant_pid();
    grant->cancel = grant->identity.pid ? dvm_cancel_create() : NULL;
    DVM_ProgramView entry = {.pid = grant->identity.pid, .unibus = provider};
    grant->tree = grant->cancel ? dvm_program_view(&entry, 1) : NULL;
    grant->publication = grant->tree ? grant->binding.begin(grant->binding.state,
        lease->endpoint, grant->tree, grant->identity.pid) : NULL;
    transport->forward = lease->endpoint;
    return grant->publication != NULL;
}


/* EN: Publish a child control in the same module and prepare the reverse route.
 * RU: Выдать дочернее управление тому же модулю и подготовить встречный маршрут. */
static int publish_module(DVM_ReturnedLease *lease, ReturnedGrant *grant,
    DVM_Provider *provider, DVM_ChannelTransport *transport)
{
#ifdef DVM_TERMINAL_LAUNCH
    grant->control = styxchannel_export_child(lease->publication);
    grant->cancel = styxchannel_export_cancel(grant->control);
    transport->reverse = lease->reverse;
    transport->control = grant->control;
    transport->revoke = 1;
    return grant->cancel && styxchannel_export_token(grant->control, grant->identity.token) &&
        styxchannel_export_publish(grant->control, provider);
#else
    (void)lease;
    (void)grant;
    (void)provider;
    (void)transport;
    return 0;
#endif
}


/* [GC:] EN: Root the original and publish one independent pump in either direction.
 * The active Dis resident lends its channel, and cleanup rolls back partial publication.
 * RU: Укоренить исходный канал и опубликовать свою службу в нужном направлении.
 * Текущий поток заимствует канал. При отказе снять частичную публикацию. */
static int start_grant(DVM_ReturnedLease *lease, ReturnedGrant *grant,
    Channel *channel, const char *type)
{
    grant->original = channel;
    grant->source = dvm_call_channel_is_proxy(channel) ? dvm_returned_proxy_lease(channel) : NULL;
    DVM_CallChannels channels = {0};
    unsigned char record[32];
    size_t used;
    int valid = dvm_call_channel_encode(&channels, type, channel, record, sizeof(record), &used) &&
        channels.count == 1 && dvm_managed_owner_capture(channel, &grant->root);
    DVM_ChannelBus *bus = NULL;
    DVM_Provider *provider = valid ? dvm_channel_bus_create(channels.schemas, 1, &bus) : NULL;
    DVM_ChannelTransport transport = {0};
    grant->schema = channels.schemas[0];
    if (provider)
    {
        dvm_channel_bus_allow_grants(bus);
        valid = lease->publication ? publish_module(lease, grant, provider, &transport) :
            publish_reverse(lease, grant, provider, &transport);
        valid = valid && dvm_result_channels_start(&channels, bus, provider, grant->cancel, &transport);
    }
    else
    {
        valid = 0;
    }
    dvm_provider_release(provider);
    dvm_call_channels_release(&channels);
    return valid;
}


/* EN: Find a reused original or a free entry in the lease's locked grant table.
 * RU: Найти повторный оригинал или свободную запись в закрытой таблице аренды. */
static size_t grant_slot(const DVM_ReturnedLease *lease, const Channel *channel)
{
    size_t slot = 0;
    while (slot < lease->grant_count && lease->grants[slot].original != channel)
    {
        slot++;
    }
    return slot;
}


/* EN/RU: See returned_proxy.h. / См. returned_proxy.h. */
int dvm_returned_lease_grant(DVM_ReturnedLease *lease, Channel *channel,
    const char *type, DVM_GrantIdentity *identity, uint32_t *key)
{
    char shape[STYX_SHAPE_MAX + 1];
    if ((!lease->token_set && !lease->publication) || !dvm_channel_value_shape(type, shape))
    {
        return 0;
    }
    ReturnedGrant prepared = {0};
    fvm_port_sync_lock(lease->lock);
    size_t slot = grant_slot(lease, channel);
    int needs_grant = slot == lease->grant_count && slot < DVM_CHANNEL_BUS_SLOTS;
    fvm_port_sync_unlock(lease->lock);
    int started = needs_grant && start_grant(lease, &prepared, channel, type);
    fvm_port_sync_lock(lease->lock);
    slot = grant_slot(lease, channel);
    if (started && slot == lease->grant_count && slot < DVM_CHANNEL_BUS_SLOTS)
    {
        unsigned budget = 128;
        lock_grant_graph();
        if (acyclic_grant(prepared.source, lease, &budget))
        {
            lease->grants[lease->grant_count++] = prepared;
            prepared = (ReturnedGrant){0};
        }
        unlock_grant_graph();
    }
    int valid = slot < lease->grant_count && !strcmp(lease->grants[slot].schema.shape, shape);
    if (valid)
    {
        const ReturnedGrant *grant = &lease->grants[slot];
        *identity = grant->identity;
        *key = grant->schema.key | (grant->schema.unbuffered ? UINT32_C(0x80000000) : 0);
    }
    fvm_port_sync_unlock(lease->lock);
    retire_grant(&prepared);
    return valid;
}


/* EN/RU: See returned_proxy.h. / См. returned_proxy.h. */
Channel *dvm_returned_lease_original(DVM_ReturnedLease *lease, const DVM_GrantIdentity *identity,
    const DVM_ChannelSchema *schema)
{
    Channel *result = NULL;
    fvm_port_sync_lock(lease->lock);
    for (size_t i = 0; i < lease->grant_count; i++)
    {
        const ReturnedGrant *grant = &lease->grants[i];
        if (dvm_grant_identity_equal(&grant->identity, identity) && grant->schema.key == schema->key &&
            grant->schema.unbuffered == schema->unbuffered && !strcmp(grant->schema.shape, schema->shape))
        {
            Channel *channel = grant->original;
            if (dvm_refcount_retain_u(&D2H(channel)->ref) == DVM_REFCOUNT_LIVE)
            {
                result = channel;
                Setmark(D2H(result));
            }
            break;
        }
    }
    fvm_port_sync_unlock(lease->lock);
    return result;
}
