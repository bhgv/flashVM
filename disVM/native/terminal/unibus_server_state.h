/* Copyright (c) 2026 bhgv
 * EN: Private dispatcher storage for mounted and legacy Unibus servers.
 * RU: Внутреннее состояние диспетчера серверов Unibus.
 */
#ifndef STYX_UNIBUS_SERVER_STATE_H
#define STYX_UNIBUS_SERVER_STATE_H

#define MOUNTED_ROOT 4UL /* Shared directory for mounted modules. */
#define FIRST_MODULE 5UL /* Three consecutive qids per module. */

typedef struct MountedExport
{
    StyxScalarFunctionOwner owner; /* Owned routes when the endpoint uses Dis. */
    size_t interface_size; /* Advertised size of the immutable profile. */
    unsigned char interface[STYX_SCALAR_INTERFACE_CAPACITY * STYX_MOUNTED_EXPORT_LIMIT]; /* Encoded profile. */
    Styxfile *profile_node; /* Directory leader owns the profile listing entry. */
    StyxScalarFunction function; /* Copied dispatch descriptor. */
} MountedExport;

struct StyxUnibusServer
{
    MountedExport *modules; /* Immutable array after server start. */
    size_t count; /* Number of mounted modules. */
    Styxserver server;   /* Original Styx dispatcher and owned physical channel. */
    pthread_t thread;    /* Exclusive native dispatcher. */
    DVM_Provider *load_offer; /* Owned one-shot local offer for an automatic single export. */
    char load_name[INTERFACE_NAME_LIMIT + 1]; /* Name selected by the owner launcher. */
    int load_state; /* Dispatcher-owned: pending zero, ready one, rejected minus one. */
    DVM_Provider *inbox; /* Owner reference for local readers. */
    StyxBacking *profile; /* Optional compiled interface record. */
    StyxBacking *original; /* Optional read-only Dis image. */
    StyxBacking *file;   /* Retained backing for write fids. */
};

/* EN: Await the authoritative offer for up to the configured open interval.
 * RU: Дождаться подтверждённого предложения в пределах интервала открытия. */
int styxunibusawaitload(StyxUnibusServer *server);

/* EN: Reserve local readiness and call mailboxes before starting the dispatcher.
 * RU: Зарезервировать локальные приёмники готовности и вызова до запуска диспетчера. */
int styxunibusprepareload(StyxUnibusServer *server, MountedExport *module, const char *name);
/* EN: Start the mounted table. loaded is reserved for one owner-selected export.
 * RU: Запустить смонтированную таблицу. loaded означает один экспорт владельца. */
StyxUnibusServer *styxunibusstarttable(RemotePhy *channel,
    const StyxMountedScalar *entries, size_t count, int loaded);
#endif
