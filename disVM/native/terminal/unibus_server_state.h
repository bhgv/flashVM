/* Copyright (c) 2026 bhgv
 * EN: Private dispatcher storage for mounted and legacy Unibus servers.
 * RU: Внутреннее состояние диспетчера серверов Unibus.
 */
#ifndef STYX_UNIBUS_SERVER_STATE_H
#define STYX_UNIBUS_SERVER_STATE_H

#define MOUNTED_ROOT 4UL /* Shared directory for mounted modules. */
#define LAUNCH_EXIT 197UL /* After all 64 mounted export qid triples. */
#define PROGRAM_DIRECTORY 512UL /* Beyond the mounted export and delta qids. */
#define PROGRAM_CATALOG 513UL /* Named launch list. */
#define FIRST_PROGRAM 514UL /* Two consecutive qids per additional image. */
#define FIRST_MODULE 5UL /* Three consecutive qids per module. */

typedef struct MountedExport
{
    int busy; /* Atomic gate for the export mailbox. */
    StyxScalarFunctionOwner owner; /* Owned routes when the endpoint uses Dis. */
    size_t directory_leader; /* Entry owning this module directory and its profile. */
    size_t interface_size; /* Advertised size of the immutable profile. */
    unsigned char interface[STYX_SHAPED_OFFER_MAX * STYX_MOUNTED_EXPORT_LIMIT]; /* Encoded profile. */
    Styxfile *profile_node; /* Directory leader owns the profile listing entry. */
    DVM_Provider *load_offer; /* Private load offer for this automatic export. */
    char load_name[INTERFACE_NAME_LIMIT + 1]; /* Launcher-selected name. */
    StyxScalarFunction function; /* Copied dispatch descriptor. */
} MountedExport;

typedef struct StyxLaunchQueue StyxLaunchQueue;

struct StyxUnibusServer
{
    MountedExport *modules; /* Fixed entries; the dispatcher finalizes loaded profiles. */
    size_t count; /* Number of mounted modules. */
    Styxserver server;   /* Original Styx dispatcher and owned physical channel. */
    pthread_t thread;    /* Exclusive native dispatcher. */
    DVM_Provider *caller_root; /* Owned reverse Styx provider for shape-driven calls. */
    int function_workers; /* Active async calls holding this server state. */
    int completion; /* Whether this endpoint exports completion. */
    StyxLaunchQueue *launches; /* Optional persistent request queue. */
    int exit_value; /* Atomic encoded status; zero while pending. */
    int stopped; /* Atomic flag set after the dispatch transport stops. */
    int loaded; /* Load-backed directory; all profiles arrive from the owner. */
    size_t load_ready; /* Dispatcher-owned checked prefix of the export list. */
    int load_state; /* Dispatcher-owned: pending zero, ready one, rejected minus one. */
    DVM_Provider *inbox; /* Owner reference for local readers. */
    StyxBacking *profile; /* Optional compiled interface record. */
    StyxBacking *original; /* Optional read-only Dis image. */
    StyxBacking *catalog; /* Immutable program advertisement. */
    StyxBacking **programs; /* Owned backing per additional executable. */
    size_t program_count; /* Count of slots represented by programs. */
    StyxBacking *file;   /* Retained backing for write fids. */
};

/* EN: Await the authoritative offer for up to the configured open interval.
 * RU: Дождаться подтверждённого предложения в пределах интервала открытия. */
int styxunibusawaitload(StyxUnibusServer *server);

/* EN: Reserve local readiness and call mailboxes before starting the dispatcher.
 * RU: Зарезервировать локальные приёмники готовности и вызова до запуска диспетчера. */
int styxunibusprepareload(MountedExport *module, const char *name);
/* EN: Start the mounted table, with optional owner-bound profiles and launch endpoint.
 * RU: Запустить таблицу с опциональными связанными профилями и точкой запуска. */
StyxUnibusServer *styxunibusstarttable(RemotePhy *channel,
    const StyxMountedScalar *entries, size_t count, int loaded, DVM_Provider *reverse, const StyxUnibusLaunch *launch);
/* EN/RU: Private one-shot constructor and result reader. / Конструктор и чтение результата. */
StyxUnibusServer *styxunibusstart(RemotePhy *channel, unsigned long tag,
    DVM_Provider *original, DVM_Provider *profile, int reusable, int completion, int persistent);
size_t styxunibusexitread(StyxUnibusServer *server, char *bytes, size_t count, uint64_t offset);
/* EN/RU: Wait for function jobs before releasing routes. / Дождаться задач перед освобождением маршрутов. */
void styxunibusdrainfunctions(StyxUnibusServer *server);
#endif
