/* Independent broker metadata: no target callbacks, VM pointers or transport I/O. */
#include "broker.h"
#include <stdlib.h>
#include <string.h>


/* A live request retains its diagnostic record even after caller timeout or session loss. */
typedef struct
{
    int used;                 /* Registry occupancy. */
    int warned;               /* One slow notification until a meaningful owner reason changes. */
    DVM_BrokerSnapshot value; /* Owned copied state and immutable dispatch endpoint. */
} Request;


/* Every subscription owns its queue, so one consumer cannot steal another's events. */
typedef struct
{
    uint32_t id;                               /* Nonzero monotonically assigned listener identity. */
    uint32_t channel;                          /* Zero selects all external channels. */
    uint64_t request;                          /* Zero selects all requests in the channel selection. */
    size_t head;                               /* First queued deviation. */
    size_t count;                              /* Occupied bounded queue entries. */
    uint64_t dropped;                          /* Saturating overflow count since last poll. */
    DVM_BrokerEvent events[DVM_BROKER_EVENTS]; /* Owned copies, no foreign roots. */
} Listener;


/* All mutable fields are protected by the supplied short native metadata mutex. */
struct DVM_Broker
{
    DVM_BrokerSync sync;                    /* Borrowed lock, never used to wait for application execution. */
    uint64_t clock;                         /* Last accepted monotonic observation time. */
    uint64_t next_request;                  /* Last issued ID; exhaustion is an explicit error. */
    uint32_t next_listener;                 /* Last issued listener ID, never recycled. */
    size_t site_count;                      /* Immutable source registry prefix. */
    size_t channel_count;                   /* Channel identity prefix; reconnect reuses identity, not generation. */
    DVM_BrokerSite sites[DVM_BROKER_SITES]; /* Shared exact source positions. */
    DVM_BrokerChannel channels[DVM_BROKER_CHANNELS]; /* Current session cards. */
    Request requests[DVM_BROKER_REQUESTS];           /* Admitted operations awaiting explicit retirement. */
    Listener listeners[DVM_BROKER_LISTENERS];        /* Independent copied diagnostic queues. */
};


/* Return whether a borrowed fixed-size text field has a terminator and optional content. */
static int text(const char *value, size_t capacity, int required)
{
    size_t index;
    if (!value || (required && !value[0]))
    {
        return 0;
    }
    for (index = 0; index < capacity; index++)
    {
        if (!value[index])
        {
            return 1;
        }
    }
    return 0;
}


/* Unlock one borrowed live broker and return status; caller must hold its metadata lock. */
static DVM_BrokerResult leave(DVM_Broker *broker, DVM_BrokerResult result)
{
    broker->sync.leave(broker->sync.user);
    return result;
}


/* Resolve a live channel identity while locked; returned pointer never escapes the call. */
static DVM_BrokerChannel *channel(DVM_Broker *broker, uint32_t id)
{
    return id && id <= broker->channel_count ? &broker->channels[id - 1] : NULL;
}


/* Resolve the full immutable request key while locked; a reconnect never rewrites old keys. */
static Request *request(DVM_Broker *broker, DVM_BrokerKey key)
{
    size_t index;
    for (index = 0; index < DVM_BROKER_REQUESTS; index++)
    {
        Request *entry = &broker->requests[index];
        if (entry->used && entry->value.key.request == key.request && entry->value.key.channel == key.channel &&
            entry->value.key.generation == key.generation)
        {
            return entry;
        }
    }
    return NULL;
}


/* Enqueue a copied deviation to matching listeners; coalesce equal identities and count overflow.
 * broker is locked. snapshot is borrowed and already complete; no user callback runs here. */
static void emit(DVM_Broker *broker, DVM_BrokerEventKind kind, const DVM_BrokerSnapshot *snapshot, uint64_t now)
{
    DVM_BrokerEvent event;
    size_t index;
    memset(&event, 0, sizeof(event));
    event.kind = kind;
    event.at_ms = now;
    event.request = *snapshot;
    if (snapshot->site)
    {
        event.origin = broker->sites[snapshot->site - 1];
    }
    event.creation = broker->sites[snapshot->channel.created_at - 1];
    event.connection = broker->sites[snapshot->channel.connected_at - 1];
    for (index = 0; index < DVM_BROKER_LISTENERS; index++)
    {
        Listener *listener = &broker->listeners[index];
        size_t item;
        if (!listener->id || (listener->channel && listener->channel != snapshot->channel.id) ||
            (listener->request && listener->request != snapshot->key.request))
        {
            continue;
        }
        for (item = 0; item < listener->count; item++)
        {
            DVM_BrokerEvent *old = &listener->events[(listener->head + item) % DVM_BROKER_EVENTS];
            if (old->kind == kind && old->request.key.request == snapshot->key.request &&
                old->request.channel.id == snapshot->channel.id &&
                old->request.channel.generation == snapshot->channel.generation)
            {
                *old = event;
                break;
            }
        }
        if (item < listener->count)
        {
            continue;
        }
        if (listener->count == DVM_BROKER_EVENTS)
        {
            if (listener->dropped != UINT64_MAX)
            {
                listener->dropped++;
            }
            continue;
        }
        listener->events[(listener->head + listener->count) % DVM_BROKER_EVENTS] = event;
        listener->count++;
    }
}


/* Revoke a current session while locked; pending work remains retained until owner completion. */
static void close_channel(DVM_Broker *broker, DVM_BrokerChannel *card, uint64_t now)
{
    DVM_BrokerSnapshot snapshot;
    size_t index;
    card->open = 0;
    memset(&snapshot, 0, sizeof(snapshot));
    snapshot.channel = *card;
    snapshot.key.channel = card->id;
    snapshot.key.generation = card->generation;
    snapshot.disconnected = 1;
    emit(broker, DVM_BROKER_DISCONNECTED, &snapshot, now);
    for (index = 0; index < DVM_BROKER_REQUESTS; index++)
    {
        Request *entry = &broker->requests[index];
        if (entry->used && !entry->value.finished && !entry->value.disconnected &&
            entry->value.key.channel == card->id && entry->value.key.generation == card->generation)
        {
            entry->value.disconnected = 1;
            emit(broker, DVM_BROKER_DISCONNECTED, &entry->value, now);
        }
    }
}


/* Allocate bounded native metadata; sync/out are borrowed and failure preserves *out. */
DVM_BrokerResult dvm_broker_create(const DVM_BrokerSync *sync, DVM_Broker **out)
{
    DVM_Broker *broker;
    if (!sync || !sync->enter || !sync->leave || !out)
    {
        return DVM_BROKER_INVALID;
    }
    broker = calloc(1, sizeof(*broker));
    if (!broker)
    {
        return DVM_BROKER_LIMIT;
    }
    broker->sync = *sync;
    *out = broker;
    return DVM_BROKER_OK;
}


/* Release native copied records only; caller has already quiesced all threads and VM roots. */
void dvm_broker_destroy(DVM_Broker *broker)
{
    free(broker);
}


/* Register a bounded immutable source origin under the native metadata lock. */
DVM_BrokerResult dvm_broker_site(DVM_Broker *broker, const DVM_BrokerSite *source, uint32_t *id)
{
    if (!broker || !source || !id || !source->line || !source->column || !source->length ||
        source->offset > UINT64_MAX - source->length || !text(source->build, sizeof(source->build), 1) ||
        !text(source->path, sizeof(source->path), 1) || !text(source->module, sizeof(source->module), 0))
    {
        return DVM_BROKER_INVALID;
    }
    broker->sync.enter(broker->sync.user);
    if (broker->site_count == DVM_BROKER_SITES)
    {
        return leave(broker, DVM_BROKER_LIMIT);
    }
    broker->sites[broker->site_count++] = *source;
    *id = (uint32_t)broker->site_count;
    return leave(broker, DVM_BROKER_OK);
}


/* Register copied transport metadata and its exact creation/connection origins. */
DVM_BrokerResult dvm_broker_open(DVM_Broker *broker, const DVM_BrokerChannel *card, uint64_t now, uint32_t *out)
{
    DVM_BrokerChannel value;
    if (!broker || !card || !out || card->transport < DVM_BROKER_IN_PROCESS || card->transport > DVM_BROKER_NETWORK ||
        !text(card->logical, sizeof(card->logical), 1) || !text(card->local, sizeof(card->local), 0) ||
        !text(card->peer, sizeof(card->peer), 1))
    {
        return DVM_BROKER_INVALID;
    }
    broker->sync.enter(broker->sync.user);
    if (now < broker->clock || !card->created_at || card->created_at > broker->site_count || !card->connected_at ||
        card->connected_at > broker->site_count)
    {
        return leave(broker, DVM_BROKER_INVALID);
    }
    if (broker->channel_count == DVM_BROKER_CHANNELS)
    {
        return leave(broker, DVM_BROKER_LIMIT);
    }
    value = *card;
    value.id = (uint32_t)broker->channel_count + 1;
    value.generation = 1;
    value.last_peer_ms = now;
    value.open = 1;
    broker->channels[broker->channel_count++] = value;
    broker->clock = now;
    *out = value.id;
    return leave(broker, DVM_BROKER_OK);
}


/* Reconnect only a revoked identity; all old request cards remain immutable copies. */
DVM_BrokerResult
dvm_broker_reconnect(DVM_Broker *broker, uint32_t id, const char *local, const char *peer, uint32_t site, uint64_t now)
{
    DVM_BrokerChannel *card;
    if (!broker || !text(local, 128, 0) || !text(peer, 128, 1))
    {
        return DVM_BROKER_INVALID;
    }
    broker->sync.enter(broker->sync.user);
    card = channel(broker, id);
    if (!card)
    {
        return leave(broker, DVM_BROKER_STALE);
    }
    if (card->open)
    {
        return leave(broker, DVM_BROKER_BUSY);
    }
    if (now < broker->clock || !site || site > broker->site_count)
    {
        return leave(broker, DVM_BROKER_INVALID);
    }
    if (card->generation == UINT32_MAX)
    {
        return leave(broker, DVM_BROKER_LIMIT);
    }
    card->generation++;
    strcpy(card->local, local);
    strcpy(card->peer, peer);
    card->connected_at = site;
    card->last_peer_ms = now;
    card->open = 1;
    broker->clock = now;
    return leave(broker, DVM_BROKER_OK);
}


/* Record transport liveness without touching application progress or emitting success. */
DVM_BrokerResult dvm_broker_heartbeat(DVM_Broker *broker, uint32_t id, uint32_t generation, uint64_t now)
{
    DVM_BrokerChannel *card;
    if (!broker)
    {
        return DVM_BROKER_INVALID;
    }
    broker->sync.enter(broker->sync.user);
    card = channel(broker, id);
    if (!card || !card->open || card->generation != generation)
    {
        return leave(broker, DVM_BROKER_STALE);
    }
    if (now < broker->clock)
    {
        return leave(broker, DVM_BROKER_INVALID);
    }
    card->last_peer_ms = now;
    broker->clock = now;
    return leave(broker, DVM_BROKER_OK);
}


/* Explicit disconnect is idempotent for the same generation and never completes target work. */
DVM_BrokerResult dvm_broker_close(DVM_Broker *broker, uint32_t id, uint32_t generation, uint64_t now)
{
    DVM_BrokerChannel *card;
    if (!broker)
    {
        return DVM_BROKER_INVALID;
    }
    broker->sync.enter(broker->sync.user);
    card = channel(broker, id);
    if (!card || card->generation != generation)
    {
        return leave(broker, DVM_BROKER_STALE);
    }
    if (now < broker->clock)
    {
        return leave(broker, DVM_BROKER_INVALID);
    }
    if (card->open)
    {
        close_channel(broker, card, now);
    }
    broker->clock = now;
    return leave(broker, DVM_BROKER_OK);
}


/* Admit a copied request under one current session; no success notification is generated. */
DVM_BrokerResult dvm_broker_begin(DVM_Broker *broker, const DVM_BrokerSnapshot *input, uint64_t now, DVM_BrokerKey *key)
{
    DVM_BrokerChannel *card;
    Request *entry = NULL;
    size_t index;
    if (!broker || !input || !key || !text(input->operation, sizeof(input->operation), 1))
    {
        return DVM_BROKER_INVALID;
    }
    broker->sync.enter(broker->sync.user);
    card = channel(broker, input->key.channel);
    if (!card || !card->open || card->generation != input->key.generation)
    {
        return leave(broker, DVM_BROKER_STALE);
    }
    if (now < broker->clock || !input->site || input->site > broker->site_count ||
        (input->deadline_ms && input->deadline_ms <= now))
    {
        return leave(broker, DVM_BROKER_INVALID);
    }
    for (index = 0; index < DVM_BROKER_REQUESTS; index++)
    {
        if (!broker->requests[index].used)
        {
            entry = &broker->requests[index];
            break;
        }
    }
    if (!entry || broker->next_request == UINT64_MAX)
    {
        return leave(broker, DVM_BROKER_LIMIT);
    }
    memset(entry, 0, sizeof(*entry));
    entry->used = 1;
    entry->value.key.channel = card->id;
    entry->value.key.generation = card->generation;
    entry->value.key.request = ++broker->next_request;
    entry->value.parent = input->parent;
    entry->value.channel = *card;
    entry->value.site = input->site;
    entry->value.object = input->object;
    entry->value.started_ms = now;
    entry->value.updated_ms = now;
    entry->value.warn_ms = input->warn_ms;
    entry->value.deadline_ms = input->deadline_ms;
    strcpy(entry->value.operation, input->operation);
    *key = entry->value.key;
    broker->clock = now;
    return leave(broker, DVM_BROKER_OK);
}


/* Publish owner state only; diagnostics inspect this copy without entering the application. */
DVM_BrokerResult dvm_broker_progress(
    DVM_Broker *broker, DVM_BrokerKey key, DVM_BrokerPhase phase, uint64_t progress, const char *detail, uint64_t now)
{
    Request *entry;
    int changed;
    if (!broker || phase < DVM_BROKER_QUEUED || phase > DVM_BROKER_PAUSED || !text(detail, 128, 0))
    {
        return DVM_BROKER_INVALID;
    }
    broker->sync.enter(broker->sync.user);
    entry = request(broker, key);
    if (!entry || entry->value.finished || entry->value.disconnected)
    {
        return leave(broker, DVM_BROKER_STALE);
    }
    if (now < broker->clock || progress < entry->value.progress)
    {
        return leave(broker, DVM_BROKER_INVALID);
    }
    changed = phase != entry->value.phase || strcmp(detail, entry->value.detail);
    entry->value.phase = phase;
    entry->value.updated_ms = now;
    entry->value.progress = progress;
    strcpy(entry->value.detail, detail);
    broker->clock = now;
    if (changed && phase == DVM_BROKER_PAUSED)
    {
        emit(broker, DVM_BROKER_DEBUG_PAUSED, &entry->value, now);
    }
    else if (changed && entry->warned && !entry->value.timed_out)
    {
        emit(broker, DVM_BROKER_SLOW, &entry->value, now);
    }
    return leave(broker, DVM_BROKER_OK);
}


/* Acknowledge completion independently of caller timeout; do not route an obsolete reply. */
DVM_BrokerResult dvm_broker_finish(DVM_Broker *broker, DVM_BrokerKey key, int failed, const char *detail, uint64_t now)
{
    Request *entry;
    DVM_BrokerResult result;
    if (!broker || (failed != 0 && failed != 1) || !text(detail, 128, 0))
    {
        return DVM_BROKER_INVALID;
    }
    broker->sync.enter(broker->sync.user);
    entry = request(broker, key);
    if (!entry || entry->value.finished)
    {
        return leave(broker, DVM_BROKER_STALE);
    }
    if (now < broker->clock)
    {
        return leave(broker, DVM_BROKER_INVALID);
    }
    entry->value.phase = failed ? DVM_BROKER_FAILED : DVM_BROKER_COMPLETED;
    entry->value.finished = 1;
    entry->value.updated_ms = now;
    strcpy(entry->value.detail, detail);
    broker->clock = now;
    result = entry->value.disconnected || entry->value.timed_out ? DVM_BROKER_STALE : DVM_BROKER_OK;
    if (failed && !entry->value.disconnected)
    {
        emit(broker, DVM_BROKER_OWNER_FAILED, &entry->value, now);
    }
    return leave(broker, result);
}


/* Independent local diagnostic call: copy state under a short metadata lock only. */
DVM_BrokerResult dvm_broker_query(DVM_Broker *broker, DVM_BrokerKey key, DVM_BrokerSnapshot *out)
{
    Request *entry;
    if (!broker || !out)
    {
        return DVM_BROKER_INVALID;
    }
    broker->sync.enter(broker->sync.user);
    entry = request(broker, key);
    if (!entry)
    {
        return leave(broker, DVM_BROKER_STALE);
    }
    *out = entry->value;
    return leave(broker, DVM_BROKER_OK);
}


/* Do not retire an executing owner's record merely because its caller stopped waiting. */
DVM_BrokerResult dvm_broker_forget(DVM_Broker *broker, DVM_BrokerKey key)
{
    Request *entry;
    if (!broker)
    {
        return DVM_BROKER_INVALID;
    }
    broker->sync.enter(broker->sync.user);
    entry = request(broker, key);
    if (!entry)
    {
        return leave(broker, DVM_BROKER_STALE);
    }
    if (!entry->value.finished)
    {
        return leave(broker, DVM_BROKER_BUSY);
    }
    memset(entry, 0, sizeof(*entry));
    return leave(broker, DVM_BROKER_OK);
}


/* Advance the independent broker pump; timeouts never fabricate remote cancellation. */
DVM_BrokerResult dvm_broker_tick(DVM_Broker *broker, uint64_t now)
{
    size_t index;
    if (!broker)
    {
        return DVM_BROKER_INVALID;
    }
    broker->sync.enter(broker->sync.user);
    if (now < broker->clock)
    {
        return leave(broker, DVM_BROKER_INVALID);
    }
    broker->clock = now;
    for (index = 0; index < broker->channel_count; index++)
    {
        DVM_BrokerChannel *card = &broker->channels[index];
        if (card->open && card->heartbeat_ms && now - card->last_peer_ms >= card->heartbeat_ms)
        {
            close_channel(broker, card, now);
        }
    }
    for (index = 0; index < DVM_BROKER_REQUESTS; index++)
    {
        Request *entry = &broker->requests[index];
        DVM_BrokerSnapshot *value = &entry->value;
        if (!entry->used || value->finished || value->disconnected || value->timed_out)
        {
            continue;
        }
        if (value->deadline_ms && now >= value->deadline_ms)
        {
            value->timed_out = 1;
            emit(broker, DVM_BROKER_TIMED_OUT, value, now);
        }
        else if (!entry->warned && value->phase != DVM_BROKER_PAUSED && value->warn_ms &&
                 now - value->started_ms >= value->warn_ms)
        {
            entry->warned = 1;
            emit(broker, DVM_BROKER_SLOW, value, now);
        }
    }
    return leave(broker, DVM_BROKER_OK);
}


/* Install an independent bounded listener; all-zero filters observe every external channel. */
DVM_BrokerResult dvm_broker_watch(DVM_Broker *broker, uint32_t id, uint64_t wanted, uint32_t *out)
{
    size_t index;
    if (!broker || !out)
    {
        return DVM_BROKER_INVALID;
    }
    broker->sync.enter(broker->sync.user);
    if (id && !channel(broker, id))
    {
        return leave(broker, DVM_BROKER_STALE);
    }
    if (broker->next_listener == UINT32_MAX)
    {
        return leave(broker, DVM_BROKER_LIMIT);
    }
    for (index = 0; index < DVM_BROKER_LISTENERS; index++)
    {
        Listener *listener = &broker->listeners[index];
        if (!listener->id)
        {
            memset(listener, 0, sizeof(*listener));
            listener->id = ++broker->next_listener;
            listener->channel = id;
            listener->request = wanted;
            *out = listener->id;
            return leave(broker, DVM_BROKER_OK);
        }
    }
    return leave(broker, DVM_BROKER_LIMIT);
}


/* Retire a listener and its native copied records without touching application objects. */
DVM_BrokerResult dvm_broker_unwatch(DVM_Broker *broker, uint32_t id)
{
    size_t index;
    if (!broker || !id)
    {
        return DVM_BROKER_INVALID;
    }
    broker->sync.enter(broker->sync.user);
    for (index = 0; index < DVM_BROKER_LISTENERS; index++)
    {
        if (broker->listeners[index].id == id)
        {
            memset(&broker->listeners[index], 0, sizeof(Listener));
            return leave(broker, DVM_BROKER_OK);
        }
    }
    return leave(broker, DVM_BROKER_STALE);
}


/* Nonblocking diagnostic consumption; queue pressure never blocks the broker or target. */
DVM_BrokerResult dvm_broker_poll(DVM_Broker *broker, uint32_t id, DVM_BrokerEvent *out, uint64_t *dropped)
{
    size_t index;
    if (!broker || !id || !out || !dropped)
    {
        return DVM_BROKER_INVALID;
    }
    broker->sync.enter(broker->sync.user);
    for (index = 0; index < DVM_BROKER_LISTENERS; index++)
    {
        Listener *listener = &broker->listeners[index];
        if (listener->id == id)
        {
            *dropped = listener->dropped;
            listener->dropped = 0;
            if (!listener->count)
            {
                return leave(broker, DVM_BROKER_EMPTY);
            }
            *out = listener->events[listener->head];
            listener->head = (listener->head + 1) % DVM_BROKER_EVENTS;
            listener->count--;
            return leave(broker, DVM_BROKER_OK);
        }
    }
    return leave(broker, DVM_BROKER_STALE);
}


/* Copy the current channel card independently of immutable old request endpoints. */
DVM_BrokerResult dvm_broker_channel(DVM_Broker *broker, uint32_t id, DVM_BrokerChannel *out)
{
    DVM_BrokerChannel *card;
    if (!broker || !out)
    {
        return DVM_BROKER_INVALID;
    }
    broker->sync.enter(broker->sync.user);
    card = channel(broker, id);
    if (!card)
    {
        return leave(broker, DVM_BROKER_STALE);
    }
    *out = *card;
    return leave(broker, DVM_BROKER_OK);
}
