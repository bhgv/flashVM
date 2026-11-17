/* Copyright (c) 2026 bhgv
 * EN: Explicit two-stream native multiplexing over one granted reliable carrier.
 * RU: Явное native мультиплексирование двух потоков поверх одного надёжного носителя.
 */
#ifndef PHY_MUX_H
#define PHY_MUX_H
#include "phy.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct PhyMux PhyMux; /* Owned native pump, never a VFS authority. */


/* EN: Consume connected reliable ordered stream on every outcome; owned mux/NULL.
 * capacity1024..1048576 bounds EACH direction of EACH channel. Starts one native
 * I/O owner. Both peers must explicitly choose P9MX v1; no probing/fallback/auth.
 * RU: Поглотить carrier всегда; свой mux/NULL. capacity ограничивает каждую очередь,
 * один native I/O owner; оба конца явно выбирают P9MX v1, без автодетекта/авторизации. */
PhyMux *phymuxcreate(RemotePhy *carrier, size_t capacity);


/* EN: Owner-only take channel0 or1 once; owned RemotePhy/NULL, no extra wire open.
 * Match channel IDs at both peers: e.g.0 A-client/B-server,1 B-client/A-server.
 * Each returned stream has its own I/O owner and thread-safe sticky wake.
 * RU: Owner получает канал0/1 один раз; свой RemotePhy/NULL. ID согласованы с peer;
 * у каждого потока свой I/O owner и thread-safe sticky wake. */
RemotePhy *phymuxchannel(PhyMux *mux, unsigned channel);


/* EN: Owner stops/joins native pump and consumes mux; NULL harmless. Never call on
 * pump thread. Other channel owners may still retire concurrently: their handles
 * and wake pins stay valid, reads drain confirmed data then fail. Final channel
 * destruction releases carrier storage. No automatic reconnect or replay.
 * RU: Owner останавливает/join pump и поглощает mux. Каналы/wake pins живут до
 * своего release; confirmed input дочитывается, затем failure; без reconnect/replay. */
void phymuxfree(PhyMux *mux);
#ifdef __cplusplus
}
#endif
#endif
