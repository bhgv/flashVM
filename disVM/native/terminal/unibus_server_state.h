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
    DVM_Provider *inbox; /* Owner reference for local readers. */
    StyxBacking *profile; /* Optional compiled interface record. */
    StyxBacking *original; /* Optional read-only Dis image. */
    StyxBacking *file;   /* Retained backing for write fids. */
};

#endif
