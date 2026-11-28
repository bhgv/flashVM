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


/* EN: Check argument termination and whether a reverse argument channel may be used.
 * RU: Проверить аргументы по значению и допустимые каналы внутри завершённой схемы. */
int styxrpc_channel_input_schema(const StyxRpcSchema *schema, int channels);


/* EN: Require at least one channel position in the terminated result schema.
 * RU: Потребовать хотя бы одну позицию канала в завершённой схеме результата. */
int styxrpc_channel_output_schema(const StyxRpcSchema *schema);


/* EN: Publish and frame a checked output on the dispatcher. Revoke any provisional
 * token on failure. The caller still owns the resource on all outcomes.
 * RU: Опубликовать проверенный результат у диспетчера. При ошибке отозвать выданный
 * токен. Ресурс во всех случаях принадлежит вызывающему. */
int styxrpc_channel_output_publish(const StyxRpcSchema *schema, const StyxRpcChannelCall *call,
    StyxRpcModuleChannels *registry, StyxRpcChannelOutput *output,
    unsigned char token[STYXRPC_CHANNEL_TOKEN_SIZE], void *reply, size_t capacity, size_t *ready);



/* EN: Free transferred result bytes and changes, and release the staged resource.
 * RU: Освободить переданный результат, изменения и подготовленный ресурс. */
void styxrpc_channel_output_dispose(StyxRpcChannelOutput *output);
#endif
