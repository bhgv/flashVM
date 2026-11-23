/* Copyright (c) 2026 bhgv
 * EN: Import channel arguments from a scoped Dis call and reuse its reverse Styx route.
 * RU: Приём аргументов-каналов Dis и использование встречного маршрута Styx.
 */
#ifndef STYXRPC_CHANNEL_CALL_H
#define STYXRPC_CHANNEL_CALL_H
#include "styxrpc/channel.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STYXRPC_CHANNEL_VERSION "unibus-channel-1" /* Contents of .unibus-version. */
#define STYXRPC_CHANNEL_TOKEN_SIZE 16U /* Existing opaque result-channel lease token. */

struct StyxClient; /* Borrowed reverse attachment for an active call. */


/* EN: One checked scoped request. Keep its original bytes alive until completion.
 * RU: Один проверенный контекстный запрос. Исходный буфер живёт до завершения. */
typedef struct StyxRpcChannelCall
{
    uint32_t pid;          /* Client process from the wire, usable on this session only. */
    FVM_SRecord arguments; /* Borrows the frame after its scope prefix. */
    size_t count;          /* Number of distinct admitted channel keys. */
    StyxRpcChannelGrant grants[STYXRPC_CHANNEL_SLOTS]; /* First-occurrence order, aliases coalesce. */
} StyxRpcChannelCall;


/* EN: Encode a scoped channel export, using its exact Limbo-compiled signature.
 * RU: Закодировать контекстный экспорт с каналами и точной сигнатурой Limbo. */
StyxRpcStatus styxrpc_channel_profile(const char *name, uint32_t signature, const StyxRpcSchema *schema,
    void *bytes, size_t capacity, size_t *used);


/* EN: Check the exact whole request and collect its call-local grants from value types.
 * RU: Проверить полный запрос и собрать его полномочия каналов по типам значений. */
StyxRpcStatus styxrpc_channel_call(const StyxRpcSchema *schema, const void *bytes, size_t size,
    StyxRpcChannelCall *call);


/* EN: Validate and seal a channel call's result and sparse ref update records.
 * RU: Проверить и упаковать результат и разреженные изменения ref вызова с каналами. */
StyxRpcStatus styxrpc_channel_call_reply(const StyxRpcSchema *schema, const StyxRpcChannelCall *call,
    const void *result, size_t result_size, const void *updates, size_t update_size,
    void *bytes, size_t capacity, size_t *used);


/* EN: Encode one exported channel result value. Supply an unpredictable active token;
 * the application authenticates it on the module's .unibus and revokes it on function clunk.
 * RU: Кодировать выданный канал в результате. Передать непредсказуемый активный токен;
 * приложение проверяет его в .unibus модуля и отзывает при clunk функции. */
StyxRpcStatus styxrpc_channel_export_value(const unsigned char token[STYXRPC_CHANNEL_TOKEN_SIZE],
    uint32_t key, int unbuffered, void *bytes, size_t capacity, size_t *used);


/* EN: Admit nested keys from a confirmed typed receive on this call's own route.
 * via is the original receive key. On failure the call and all grants stay unchanged.
 * RU: Принять вложенные ключи подтверждённого приёма на маршруте этого вызова.
 * via — ключ исходного приёма. При ошибке вызов и полномочия не меняются. */
StyxRpcStatus styxrpc_channel_import_element(StyxRpcChannelCall *call, uint32_t via,
    const void *bytes, size_t size);


/* EN: Run a selection on the active caller's reverse .unibus. Each operation uses one fid.
 * Buffers are disjoint and already sized for the selected types. timeout_ms is 1..60000,
 * checked between synchronous Styx calls. MORE means timeout, with the fid clunked.
 * An already committed channel operation may have taken effect. Never auto-replay.
 * RU: Выполнить выбор через встречный .unibus живого вызова. Одна операция использует один fid.
 * Буферы раздельны и вмещают выбранные типы. timeout_ms от 1 до 60000 проверяется между
 * синхронными вызовами Styx. При MORE истекло время и fid закрыт.
 * Уже совершённая операция могла изменить состояние. Автоповтора нет. */
StyxRpcStatus styxrpc_channel_exchange(struct StyxClient *reverse, const StyxRpcChannelCall *call,
    const StyxRpcSelection *selection, unsigned timeout_ms, void *scratch, size_t scratch_size,
    void *request, size_t request_capacity, void *reply, size_t reply_capacity, StyxRpcChannelResult *result);

#ifdef __cplusplus
}
#endif
#endif
