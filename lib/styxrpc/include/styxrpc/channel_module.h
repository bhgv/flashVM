/* Copyright (c) 2026 bhgv
 * EN: Publish typed result channels on a scoped graph module and retire them on clunk.
 * RU: Публикация каналов результата контекстного модуля и отзыв при clunk.
 */
#ifndef STYXRPC_CHANNEL_MODULE_H
#define STYXRPC_CHANNEL_MODULE_H
#include "styxrpc/channel_call.h"
#include "styxrpc/graph_module.h"
#include "styxrpc/module_channels.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct StyxRpcChannelOutput
{
    StyxRpcResource resource; /* Operational channel bus; borrow until release. */
    uint32_t key;              /* First exported key, zero if no live channels occur. */
    int unbuffered;            /* Capacity marker for the implicit single channel result. */
    void (*release)(void *);   /* Optional owner cleanup for resource.context. */
    void *updates;            /* Optional malloc-owned positional update children transferred to SDK. */
    size_t update_size;       /* Bytes of updates, excluding the outer update-list record. */
    void *result;              /* Optional malloc-owned typed child, with current-group channel keys. */
    size_t result_size;        /* Bytes of result, excluding the outer graph-result record. */
} StyxRpcChannelOutput;


typedef struct StyxRpcChannelExport
{
    const char *name;       /* Compiled export component. */
    uint32_t signature;     /* Compiled signature. */
    StyxRpcSchema schema;   /* Typed result containing channels; inputs depend on the module. */
    StyxRpcStatus (*begin)(void *context, const StyxRpcChannelCall *call, StyxRpcChannelOutput *output); /* Owner callback. */
    void *context;          /* Borrowed provider state. */
} StyxRpcChannelExport;


typedef struct StyxRpcChannelModule
{
    StyxRpcGraphModule graph;             /* Common mounted directory and profile. */
    const StyxRpcChannelExport *exports; /* Borrowed immutable scoped descriptors. */
    StyxRpcModuleChannels *registry;     /* Borrowed token registry for this module. */
    StyxRpcEntry version;                /* Channel protocol marker. */
    StyxRpcEntry bus;                     /* Authenticated operation resource. */
} StyxRpcChannelModule;


/* EN: Initialize a scoped module borrowing exports and an initialized lease registry.
 * RU: Инициализировать контекстный модуль с описателями и готовым реестром аренд. */
StyxRpcStatus styxrpc_channel_module_init(StyxRpcChannelModule *module, const char *name,
    const StyxRpcChannelExport *exports, size_t count, StyxRpcModuleChannels *registry);

extern const StyxRpcTree styxrpc_channel_module_tree; /* Context is an initialized StyxRpcChannelModule. */

#ifdef __cplusplus
}
#endif
#endif
