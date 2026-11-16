/* Copyright (c) 2026 bhgv
 * EN: Trusted host constructor for isolated Styx admission state.
 * RU: Конструктор доверенного host для отдельного состояния Styx admission.
 */
#ifndef STYX_PEER_PROVIDER_H
#define STYX_PEER_PROVIDER_H
#include "peer.h"
#include "export.h"


/* EN/RU: Bind live borrowed publisher, native peer label, attested provenance flags
 * and fixed access ceiling. Owned peer/NULL; creation serialized with owner close.
 * REMOTE requires AUTHENTICATED from a verified host transport, never from Tattach.
 * No authentication is implemented here. Native callbacks only; no guest pauses.
 * Связать publisher, native peer, flags и ceiling; свой peer/NULL, до owner close. */
StyxPeer *styxpeer(DVM_Publication *publication, const char *trusted, unsigned flags, unsigned access);
#endif
