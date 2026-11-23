/* Copyright (c) 2026 bhgv
 * EN: Native result leases and managed alias staging for returned channels.
 * RU: Нативная аренда результата и временные алиасы возвращённых каналов.
 */
#ifndef DVM_RETURNED_PROXY_H
#define DVM_RETURNED_PROXY_H
#include "channel_bus.h"
#include "cancel.h"

struct Channel;
typedef struct DVM_ReturnedLease DVM_ReturnedLease; /* Shared native ownership of one function fid. */


/* EN: One bounded message grant address. A zero PID selects the module token.
 * RU: Адрес выдачи из сообщения. Нулевой PID выбирает токен модуля. */
typedef struct DVM_GrantIdentity
{
    uint32_t pid; /* Synthetic reverse PID, or zero for the module direction. */
    unsigned char token[STYX_CHANNEL_EXPORT_TOKEN_SIZE]; /* Module token when pid is zero. */
} DVM_GrantIdentity;


/* EN: Compare exact addresses in their respective transport directions.
 * RU: Сверить точные адреса в соответствующих направлениях транспорта. */
int dvm_grant_identity_equal(const DVM_GrantIdentity *left, const DVM_GrantIdentity *right);


/* EN: Create an imported message route with a borrowed native publication anchor,
 * reverse root, or module token. Return an owned unpublished lease or NULL.
 * RU: Создать входящий маршрут с заимствованным управлением публикацией,
 * встречным корнем либо токеном модуля. Вернуть свою аренду или NULL. */
DVM_ReturnedLease *dvm_returned_lease_import(DVM_Provider *endpoint,
    DVM_Provider *control, DVM_Provider *reverse, const DVM_GrantIdentity *identity);


/* [GC:] EN: Cache result aliases until the decoded graph has been installed.
 * RU: Сохранить алиасы результата до установки декодированного графа. */
typedef struct DVM_ReturnedImport
{
    DVM_ReturnedLease *lease; /* Owned until result installation or failure. */
    struct Channel *objects[DVM_CHANNEL_BUS_SLOTS]; /* Temporary rooted aliases. */
    DVM_ChannelSchema schemas[DVM_CHANNEL_BUS_SLOTS]; /* Exact keys and types of cached proxies. */
    size_t count; /* Number of live entries. */
} DVM_ReturnedImport;


/* EN: Retain the mounted endpoint and reserve deferred cleanup before I/O.
 * RU: Удержать узел маунта и зарезервировать отложенное закрытие до I/O. */
DVM_ReturnedLease *dvm_returned_lease_create(DVM_Provider *endpoint);


/* EN: Transfer the original open function fid into the unpublished lease.
 * RU: Передать исходный открытый fid функции в неопубликованную аренду. */
int dvm_returned_lease_take(DVM_ReturnedLease *lease, DVM_VfsHandle *fid);


/* EN: Drop an optional pin, deferring final clunk and endpoint release.
 * RU: Снять необязательную ссылку, отложив последний clunk и освобождение узла. */
void dvm_returned_lease_release(DVM_ReturnedLease *lease);


/* EN: Match a native token against earlier leaves of this same result.
 * RU: Сверить нативный токен с прежними полями того же результата. */
int dvm_returned_lease_token(DVM_ReturnedLease *lease, const unsigned char *token);


/* EN: Attach a lease pin and copied schema to a new, rooted remote proxy.
 * RU: Передать ссылку на аренду и копию схемы новому укоренённому прокси. */
int dvm_returned_proxy_arm(struct Channel *channel, DVM_ReturnedLease *lease,
    const DVM_ChannelSchema *schema);


/* EN: Borrow route data while the caller independently owns the proxy.
 * RU: Заимствовать маршрут, пока вызывающий независимо удерживает прокси. */
const DVM_ChannelSchema *dvm_returned_proxy_route(const struct Channel *channel,
    DVM_Provider **endpoint, DVM_Cancel **cancel, const unsigned char **token);


/* EN: Borrow the lease from an independently held proxy.
 * RU: Заимствовать аренду из независимо удерживаемого прокси. */
DVM_ReturnedLease *dvm_returned_proxy_lease(const struct Channel *channel);


/* [GC:] EN: Register candidate or return an owned canonical alias for its key.
 * Return NULL for a conflicting type. Candidate ownership remains with the caller.
 * RU: Зарегистрировать candidate или вернуть свою ссылку на канонический прокси.
 * При конфликте типов вернуть NULL. Кандидат остаётся у вызывающего. */
struct Channel *dvm_returned_proxy_canonical(struct Channel *candidate);


/* [GC:] EN: Publish channel for the exact lease, returning its address and wire key.
 * RU: Опубликовать канал точной аренды и вернуть адрес и сетевой ключ. */
int dvm_returned_lease_grant(DVM_ReturnedLease *lease, struct Channel *channel,
    const char *type, DVM_GrantIdentity *identity, uint32_t *key);


/* [GC:] EN: Return an owned original matching the lease's exact address, key and type.
 * RU: Вернуть свою ссылку на оригинал по адресу, ключу и типу данной аренды. */
struct Channel *dvm_returned_lease_original(DVM_ReturnedLease *lease, const DVM_GrantIdentity *identity,
    const DVM_ChannelSchema *schema);


/* EN: Dispose a detached aux binding once, after its managed channel retires.
 * RU: Один раз освободить отсоединённую aux-привязку после освобождения канала. */
void dvm_returned_proxy_dispose(void *binding);


/* [GC:] EN: Drop cached result roots and the decoder's native pin.
 * RU: Снять корни результата и нативную ссылку декодировщика. */
void dvm_returned_import_clear(DVM_ReturnedImport *imports);

#endif
