/* Copyright (c) 2026 bhgv
 * EN: Borrow the negotiated module-reference route through playback.
 * RU: Заимствование согласованной маршрутизации ссылок на время воспроизведения.
 */
#ifndef FVM_STYXRPC_PEER_REFS_H
#define FVM_STYXRPC_PEER_REFS_H
#include "styxrpc/named_peer.h"

typedef struct FVM_PeerRefs
{
    struct StyxRpcModuleSlot *slot; /* Borrowed local publication through movie teardown. */
    StyxRpcNamedPeer *host;         /* Forward on-demand capability, or NULL on reverse. */
    unsigned lane;                 /* Local publication's ordinary service lane. */
    int enabled;                   /* Attached client verified a valid scope file. */
    int local_exports_only;        /* Mixed pair serves references to Flash-owned exports. */
    unsigned char scope[STYXRPC_MODULE_REF_SCOPE_SIZE]; /* Client copy of the shared scope. */
} FVM_PeerRefs;
#endif
