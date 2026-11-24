/* Copyright (c) 2026 bhgv
 * EN: Compose one loadable Styx image from ordinary C callbacks.
 * RU: Сборка одного загружаемого образа Styx из обычных колбэков C.
 */
#ifndef STYXRPC_MODULE_H
#define STYXRPC_MODULE_H
#include "styxrpc/scalar_module.h"
#include "styxrpc/channel_module.h"
#include "styxrpc/variable_file.h"
#include "styxrpc/control.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum StyxRpcModuleKind
{
    STYXRPC_MODULE_INT,     /* Legacy fn(int): int. */
    STYXRPC_MODULE_MANAGED, /* Managed by-value and flat-ref call. */
    STYXRPC_MODULE_GRAPH,   /* Positional graph and hybrid call. */
    STYXRPC_MODULE_CHANNEL  /* Live channel result from a value-input call. */
} StyxRpcModuleKind;

typedef struct StyxRpcModuleEntry
{
    StyxRpcModuleKind kind; /* Select the active callback descriptor below. */
    union
    {
        StyxRpcExport integer;           /* Scalar callback and state. */
        StyxRpcManagedExport managed;    /* Managed callback and state. */
        StyxRpcGraphExport graph;        /* Graph callback and state. */
        StyxRpcChannelExport channel;    /* Channel factory callback and state. */
    } callback;                         /* One ordinary callback contract. */
} StyxRpcModuleEntry;

typedef struct StyxRpcModule StyxRpcModule; /* Owns compiled offers, copied descriptors and names. */

typedef struct StyxRpcModuleVariable
{
    const char *name;      /* Literal child of .var. */
    StyxIntVariable value; /* Live signed int32 cell. Null callbacks omit the corresponding access. */
} StyxRpcModuleVariable;

typedef struct StyxRpcModuleSpec
{
    const char *name;                       /* Published basename under mounted. */
    const StyxRpcModuleEntry *entries;      /* Callback exports to compile and copy. */
    size_t count;                           /* Number of exports, one through STYXRPC_MAX_EXPORTS. */
    const StyxRpcModuleVariable *variables; /* Optional live variables in .var. */
    size_t variable_count;                  /* Zero through STYXRPC_MAX_EXPORTS. */
    StyxRpcModuleChannels *registry;        /* Borrowed private registry when exports return channels. */
    StyxRpcControl *control;                /* Optional borrowed .upload and .state at the server root. */
} StyxRpcModuleSpec;

/* EN: Compile one mounted image. Copy entries and names, borrow callback contexts and registry.
 * Run server and callbacks on the owner. Destroy image after the last server using it.
 * RU: Собрать смонтированный образ. Копировать entries и имена, заимствовать контексты и реестр.
 * Сервер и колбэки работают у владельца. Освобождать образ после последнего сервера. */
StyxRpcStatus styxrpc_module_create(const StyxRpcModuleSpec *spec, StyxRpcModule **out);


/* EN: Release a quiescent image. All using servers must already be destroyed. NULL is accepted.
 * RU: Освободить остановленный образ после уничтожения всех его серверов. Допускается NULL. */
void styxrpc_module_destroy(StyxRpcModule *module);

extern const StyxRpcTree styxrpc_module_tree; /* Serve with a created StyxRpcModule as context. */

#ifdef __cplusplus
}
#endif
#endif
