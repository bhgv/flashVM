/* Copyright (c) 2026 bhgv
 * EN: Shared graph file lifetime for ordinary and scoped result-channel modules.
 * RU: Общий срок жизни файлов обычных графов и контекстных каналов результата.
 */
#ifndef STYXRPC_CHANNEL_MODULE_INTERNAL_H
#define STYXRPC_CHANNEL_MODULE_INTERNAL_H
#include "styxrpc/channel_call.h"
#include "styxrpc/channel_module.h"


/* EN: Serve one complete channel function request and stage its owned publication.
 * RU: Обслужить полный запрос канала и подготовить собственную публикацию. */
int styxrpc_channel_module_finish(const StyxRpcChannelExport *function, StyxRpcModuleChannels *registry,
    const void *request, size_t size, void *reply, size_t capacity, size_t *ready,
    StyxRpcChannelOutput *active, unsigned char token[STYXRPC_CHANNEL_TOKEN_SIZE]);

#endif
