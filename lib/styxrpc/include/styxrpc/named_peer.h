/* Copyright (c) 2026 bhgv
 * EN: Host one named registry for both sides of a delegated peer connection.
 * RU: Обслуживание одного реестра имён для обеих сторон выданного соединения.
 */
#ifndef STYXRPC_NAMED_PEER_H
#define STYXRPC_NAMED_PEER_H
#include "styxrpc/named_channel.h"
#include "styxrpc/module_ref.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STYXRPC_NAMED_PEER_ATTACH "styxrpc-named-peer" /* Capability in the ordinary Tattach name. */
#define STYXRPC_NAMED_PEER_LANE 2U /* Logical lane after agreeing on the attach capability. */
#define STYXRPC_MODULE_REF_ATTACH "styxrpc-module-refs" /* Named-lane client capability. */
#define STYXRPC_MODULE_REF_SCOPE_FILE ".varref" /* Private scope file on the named lane. */

struct RemotePhy; /* Owned extra stream, provided by the caller. */
typedef struct StyxRpcNamedPeer StyxRpcNamedPeer; /* Owns two native dispatchers and the local client. */


/* EN: Consume lane always. Borrow registry through destroy. Serve the same
 * registry on lane and a private local stream. msize is in bytes, timeout
 * in milliseconds and tags bounds pending local Styx requests. NULL on failure.
 * RU: Всегда принять lane. Заимствовать registry до destroy. Обслуживать общий
 * реестр через lane и частный локальный поток. msize в байтах, timeout в мс,
 * tags ограничивает ожидающие локальные запросы Styx. NULL при отказе. */
StyxRpcNamedPeer *styxrpc_named_peer_host(struct RemotePhy *lane, StyxRpcNamedRegistry *registry,
    unsigned msize, unsigned timeout, unsigned tags);


/* EN: As peer_host, optionally advertise a copied connection scope for module refs.
 * Supply STYXRPC_MODULE_REF_SCOPE_SIZE fresh random bytes or NULL.
 * RU: Как peer_host, с возможностью объявить копию области ссылок на модули.
 * Передать STYXRPC_MODULE_REF_SCOPE_SIZE случайных байтов или NULL. */
StyxRpcNamedPeer *styxrpc_named_peer_host_scoped(struct RemotePhy *lane, StyxRpcNamedRegistry *registry,
    unsigned msize, unsigned timeout, unsigned tags, const void *scope);


/* EN: Copy the host's scope once the remote mount advertises module refs.
 * RU: Скопировать область хоста после объявления поддержки удалённым клиентом. */
int styxrpc_named_peer_ref_scope(StyxRpcNamedPeer *peer, void *scope);


/* EN: Read the optional scope into an STYXRPC_MODULE_REF_SCOPE_SIZE-byte buffer.
 * The remote client uses STYXRPC_MODULE_REF_ATTACH as its named attach name.
 * RU: Прочитать необязательную область в STYXRPC_MODULE_REF_SCOPE_SIZE байтов.
 * Удалённый клиент использует STYXRPC_MODULE_REF_ATTACH в attach службы имён. */
int styxrpc_named_peer_read_ref_scope(struct StyxClient *client, void *scope);


/* EN: Borrow the local named client through peer destroy. NULL is accepted.
 * RU: Заимствовать локального клиента до закрытия peer. NULL допустим. */
struct StyxClient *styxrpc_named_peer_client(StyxRpcNamedPeer *peer);


/* EN: Cancel the streams, join the native servers and consume peer. NULL is valid.
 * End the use of the borrowed client before destroying peer.
 * RU: Остановить потоки, дождаться серверов и освободить peer. NULL допустим.
 * Прекратить использование заимствованного клиента до закрытия peer. */
void styxrpc_named_peer_destroy(StyxRpcNamedPeer *peer);

#ifdef __cplusplus
}
#endif
#endif
