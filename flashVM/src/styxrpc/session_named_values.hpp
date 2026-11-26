/* Copyright (c) 2026 bhgv
 * EN: Bridge portable scalar frames and FlashVM value snapshots.
 * RU: Связать переносимые скалярные кадры и снимки значений FlashVM.
 */
#ifndef FVM_STYXRPC_NAMED_VALUES_HPP
#define FVM_STYXRPC_NAMED_VALUES_HPP
#include "flashvm/styxrpc_session.h"
#include "peer_refs.h"

struct FVM_NamedRoute
{
    FVM_StyxRpcSession *vfs; /* Borrowed ordinary session for paths and module references. */
    StyxClient *client;     /* Borrowed issuer used for the named channels and cells. */
    const FVM_PeerRefs *refs; /* Optional module-ref routes and connection scope. */
};


/* [GC:] EN: Encode value into an owned frame. Release bytes via free.
 * RU: Закодировать value в собственный кадр. Освободить bytes через free. */
FVM_Result fvm_named_encode(FVM_Context *context, const FVM_NamedRoute &route,
    const FVM_Value *value, unsigned char **bytes, size_t *size);

/* [GC:] EN: Decode a borrowed frame into a by-value snapshot or an issuer-backed alias.
 * RU: Декодировать кадр в снимок либо ссылку с доступом к владельцу. */
FVM_Result fvm_named_decode(FVM_Context *context, const FVM_NamedRoute &route,
    const unsigned char *bytes, size_t size, FVM_Value *out);
#endif
