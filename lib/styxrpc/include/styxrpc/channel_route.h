/* Copyright (c) 2026 bhgv
 * EN: Typed UniBus channel values on the current bus or delegated Styx routes.
 * RU: Типизированные каналы UniBus на текущей шине и делегированных маршрутах Styx.
 */
#ifndef STYXRPC_CHANNEL_ROUTE_H
#define STYXRPC_CHANNEL_ROUTE_H
#include "styxrpc/channel.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STYXRPC_CHANNEL_TOKEN_SIZE 16U /* Opaque module lease token. */


typedef enum StyxRpcChannelRouteKind
{
    STYXRPC_ROUTE_CURRENT, /* Key on the containing channel's bus. */
    STYXRPC_ROUTE_PID,     /* Delegated reverse .prog/<pid>/.unibus. */
    STYXRPC_ROUTE_MODULE   /* Delegated token on the module's .unibus. */
} StyxRpcChannelRouteKind;


typedef struct StyxRpcChannelRoute
{
    StyxRpcChannelRouteKind kind;                 /* Which address fields are populated. */
    uint32_t pid;                                 /* Reverse ID with its high bit set, otherwise zero. */
    unsigned char token[STYXRPC_CHANNEL_TOKEN_SIZE]; /* Exact module token, otherwise all zero. */
    StyxRpcChannelGrant channel;                   /* Positive 31-bit key and declared element shape. */
    int unbuffered;                                /* 1 for rendezvous, 0 for ordinary keys. */
} StyxRpcChannelRoute;


typedef struct StyxRpcChannelRoutes
{
    size_t count;                                   /* Number of distinct routes. */
    StyxRpcChannelRoute entries[STYXRPC_CHANNEL_SLOTS]; /* Order of first occurrence. */
} StyxRpcChannelRoutes;


typedef struct StyxRpcChannelPolicy
{
    int (*allow)(const void *context, const StyxRpcChannelRoute *route); /* 1 grants access; no side effects. */
    const void *context; /* Borrowed by the selection and its fid through completion. */
} StyxRpcChannelPolicy;


/* EN: Inspect a complete typed element and copy distinct channel addresses and schemas.
 * Conflicting aliases, unsupported values and malformed input leave routes unchanged.
 * RU: Проверить целый элемент и скопировать адреса и схемы разных каналов.
 * Несовместимые алиасы, неподдерживаемые значения и ошибки сохраняют прежний routes. */
StyxRpcStatus styxrpc_channel_element_routes(const char *shape, const void *bytes, size_t size,
    StyxRpcChannelRoutes *routes);


typedef struct StyxRpcChannelKeyMap
{
    uint32_t source; /* Existing positive current-bus key. */
    uint32_t target; /* Replacement positive ordinary scoped key. */
} StyxRpcChannelKeyMap;


/* EN: Verify and rewrite current-bus leaves using a unique key map. Buffers must be disjoint.
 * RU: Проверить и заменить листья текущей шины по однозначной карте. Буферы раздельны. */
StyxRpcStatus styxrpc_channel_element_remap(const char *shape, const void *bytes, size_t size,
    const StyxRpcChannelKeyMap *keys, size_t count, void *output, size_t capacity, size_t *used);


/* EN: Collect module addresses from the complete result field of a confirmed Dis call.
 * RU: Собрать адреса модуля из полного поля результата подтверждённого вызова Dis. */
StyxRpcStatus styxrpc_channel_export_routes(const char *shape, const void *bytes, size_t size,
    StyxRpcChannelRoutes *routes);


/* EN: Encode one route as a channel leaf for use inside a typed element.
 * RU: Закодировать маршрут как лист канала в типизированном элементе. */
StyxRpcStatus styxrpc_channel_route_value(const StyxRpcChannelRoute *route, void *bytes, size_t capacity, size_t *used);


/* EN: Decode guards with a borrowed policy for routed channel values.
 * RU: Декодировать ветки с заимствованной политикой маршрутов. */
StyxRpcStatus styxrpc_channel_read_routed(const StyxRpcChannelGrant *grants, size_t grant_count,
    const StyxRpcChannelPolicy *policy, const void *bytes, size_t size, StyxRpcSelection *operation);


struct StyxClient; /* Borrowed connection carrying the exact peer publication. */

/* EN: Open the delegated bus on session and perform one selection for route->channel.
 * Supply module_bus only for a module token. A PID route derives its path from the address.
 * The session, publication and nonoverlapping buffers are borrowed through completion.
 * RU: Открыть делегированную шину на session и выполнить выбор для route->channel.
 * module_bus нужен для токена модуля. Для PID путь берётся из адреса.
 * Сессия, публикация и непересекающиеся буферы заимствуются до завершения. */
StyxRpcStatus styxrpc_channel_route_exchange(struct StyxClient *session, const char *module_bus,
    const StyxRpcChannelRoute *route, const StyxRpcSelection *selection, unsigned timeout_ms,
    void *scratch, size_t scratch_size, void *request, size_t request_capacity,
    void *reply, size_t reply_capacity, StyxRpcChannelResult *result);


/* EN: Like route_exchange, with optional stop(state) between Styx operations.
 * RU: Как route_exchange с необязательным stop(state) между операциями Styx. */
StyxRpcStatus styxrpc_channel_route_exchange_until(struct StyxClient *session, const char *module_bus,
    const StyxRpcChannelRoute *route, const StyxRpcSelection *selection, unsigned timeout_ms,
    void *scratch, size_t scratch_size, void *request, size_t request_capacity,
    void *reply, size_t reply_capacity, StyxRpcChannelResult *result, StyxRpcCallStop stop, void *state);


/* EN: Admit all PID roots in an element on the exact reverse mount.
 * Confirm each PID identity and bus. A failed import preserves routes and indices.
 * RU: Принять корневые каналы PID из элемента на точном встречном монтировании.
 * Проверить PID и шину. При отказе сохранить routes и индексы. */
StyxRpcStatus styxrpc_channel_pid_import(struct StyxClient *reverse, const char *shape,
    const void *bytes, size_t size, StyxRpcChannelRoutes *routes);


/* EN: Exchange on the PID at anchor. Allow only known keys on that bus.
 * Commit newly received current-bus keys before exposing the result.
 * RU: Выполнить обмен на PID по anchor. Допустить только известные ключи шины.
 * Добавить принятые текущие ключи до выдачи результата. */
StyxRpcStatus styxrpc_channel_pid_exchange(struct StyxClient *reverse, StyxRpcChannelRoutes *routes, size_t anchor,
    const StyxRpcSelection *selection, unsigned timeout_ms, void *scratch, size_t scratch_size,
    void *request, size_t request_capacity, void *reply, size_t reply_capacity, StyxRpcChannelResult *result);


/* EN: Like pid_exchange, with optional stop(state). Cancellation preserves routes and result.
 * RU: Как pid_exchange с необязательным stop(state). Отмена сохраняет routes и result. */
StyxRpcStatus styxrpc_channel_pid_exchange_until(struct StyxClient *reverse, StyxRpcChannelRoutes *routes, size_t anchor,
    const StyxRpcSelection *selection, unsigned timeout_ms, void *scratch, size_t scratch_size,
    void *request, size_t request_capacity, void *reply, size_t reply_capacity, StyxRpcChannelResult *result,
    StyxRpcCallStop stop, void *state);

#ifdef __cplusplus
}
#endif
#endif
