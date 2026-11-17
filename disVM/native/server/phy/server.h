/* Copyright (c) 2026 bhgv
 * EN: Original libstyx server over a granted RemotePhy; one native owner and explicit wake pins.
 * RU: Исходный libstyx server поверх granted RemotePhy; один native owner и явные wake pins.
 */
#ifndef STYX_PHY_SERVER_H
#define STYX_PHY_SERVER_H
#include "phy.h"
#ifdef __cplusplus
extern "C" {
#endif
struct Styxserver; /* Original caller-owned zeroed server shell. */
struct Styxops; /* Original callback table, borrowed through server retirement. */
typedef struct StyxPhyWake StyxPhyWake; /* Owned native readiness pin, no server/guest pointers exposed. */


/* EN/RU: Consume connected reliable ordered stream PHY on every outcome; NULL success/error string.
 * Fresh zeroed server/borrowed ops; native owner serializes init/wait/process/reply/end.
 * queued_frames1..128 bounds encoded reply storage; no socket/listener/dial/authentication added.
 * Link infernostyx_phy instead of the original POSIX backend; original 9P API/codecs remain intact.
 * Принять connected reliable ordered PHY всегда; NULL success/error string. Server zeroed,
 * ops borrowed; один native owner. queued_frames1..128, без socket/listener/dial/new authority. */
char *styxphyinit(struct Styxserver *server, struct Styxops *ops, RemotePhy *phy,
    int permissions, int needfile, unsigned queued_frames);


/* EN/RU: Owner-only pin native wake state while server is live; owned handle/NULL on invalid/overflow.
 * Producers retain this capability until callbacks retire, including after server end.
 * Удержать native wake state под server owner; свой handle/NULL, callbacks владеют pin до завершения. */
StyxPhyWake *styxphywakepin(struct Styxserver *server);


/* EN/RU: Signal native I/O readiness through an owned wake pin; no wire traffic or server execution.
 * Сигнал native I/O readiness под owned wake pin, без wire traffic/server execution. */
void styxphywake(StyxPhyWake *wake);


/* EN/RU: Consume readiness pin without waiting for server/guest; NULL harmless.
 * Поглотить readiness pin без ожидания server/guest; NULL harmless. */
void styxphywakefree(StyxPhyWake *wake);


/* EN/RU: Owner-only current free reply slots;0 invalid/failed/full, no I/O or permission decision.
 * Owner-only число свободных reply slots;0 invalid/failed/full, без I/O/permission decision. */
unsigned styxphyspace(struct Styxserver *server);


/* EN/RU: Owner-only unsent encoded bytes,0 invalid/empty; never interprets queued contents.
 * Owner-only unsent bytes,0 invalid/empty, без интерпретации contents. */
size_t styxphypending(struct Styxserver *server);


/* EN/RU: Owner flushes staged output within one monotonic budget0..120000ms;1 drained/0 pending/failure.
 * Partial known prefix is retained, uncertain output terminally fails; no automatic replay/reconnect.
 * Native wait only, not guest cooperation. Explicit styxend discards any remaining output.
 * Owner flush с единым budget0..120000ms;1 drained/0 pending/failure, tail сохраняется,
 * uncertain terminal, без replay/reconnect. styxend явно отбрасывает остаток. */
int styxphyflush(struct Styxserver *server, unsigned timeout);
#ifdef __cplusplus
}
#endif
#endif
