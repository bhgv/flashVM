/* Copyright (c) 2026 bhgv
 * EN: Per-connection native publication admission; no wire identity authority.
 * RU: Native admission публикации на соединение; wire identity не даёт полномочий.
 */
#ifndef STYX_PEER_H
#define STYX_PEER_H
#include "file.h"

typedef struct StyxPeer StyxPeer; /* Exclusive native connection admission state. */


/* EN/RU: Request logical name/kind (0 any,1 file,2 directory) for borrowed peer.
 * Return1 with owned root,0 pending,-1 refused; output cleared on failure/pending.
 * Repeated same attach gets independent backing of the same selected provider.
 * Different name/kind cannot replace a pending or granted root. Serialize per peer.
 * Запросить logical name/kind;1 и свой root,0 pending,-1 отказ; сериализовать peer. */
int styxpeerattach(StyxPeer *peer, const char *name, unsigned kind, StyxBacking **root);


/* EN/RU: Cancel only the outstanding admission ticket; keep an existing bound root.
 * Owner-thread serialized; late owner reply refuses; no wire response is generated.
 * Отменить только pending ticket; выбранный корень сохранить, wire-ответ не посылать. */
void styxpeercancel(StyxPeer *peer);


/* EN/RU: Consume peer, cancel pending request and release selected provider/endpoint.
 * Existing independently owned nodes/opens survive. NULL allowed; callers retired.
 * Поглотить peer и pending request; выданные nodes/opens живут дальше. */
void styxpeerfree(StyxPeer *peer);
#endif
