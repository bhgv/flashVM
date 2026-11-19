/* Copyright (c) 2026 bhgv
 * EN: Original libstyx server over a granted RemotePhy; one native owner and explicit wake pins.
 * RU: Исходный libstyx server поверх granted RemotePhy; один native owner и явные wake pins.
 */
#ifndef STYX_PHY_SERVER_H
#define STYX_PHY_SERVER_H
#include "phy.h"
#include "walk.h"
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
 * Ordinary single-session server: never creates P9MX or a reverse client.
 * Принять connected reliable ordered PHY всегда; NULL success/error string. Server zeroed,
 * ops borrowed; один native owner. queued_frames1..128, без socket/listener/dial/new authority.
 * Обычный server одной сессии; не создаёт P9MX или встречного клиента. */
char *styxphyinit(struct Styxserver *server, struct Styxops *ops, RemotePhy *phy,
    int permissions, int needfile, unsigned queued_frames);


/* EN/RU: Explicit S-record active-history profile around original Styx; PHY consumed always, root copied on success.
 * Both peers opt in; root describes the virtual exported endpoint of the actual prepared grant and must have nonzero domain/object.
 * Tag0 negotiates data purpose; malformed profiles close without raw fallback. Same owner/quota rules as styxphyinit.
 * Явный S-record history profile; PHY принят всегда, root — virtual exported endpoint actual grant с nonzero domain/object.
 * Обе стороны opt in; tag0 согласует purpose, malformed profiles закрываются без raw fallback.
 */
char *styxphywalkinit(struct Styxserver *server, struct Styxops *ops, RemotePhy *phy,
    int permissions, int needfile, unsigned queued_frames, const WalkEntity *root);


/* EN/RU: Return a standard Rerror text while explicitly marking this currently executing callback's result as a cycle.
 * Server owner/current synchronous callback only; does not grant access, pause a guest, or guess from error text.
 * Use only when the actual VFS result is DVM_VFS_CYCLE. Raw peers receive ordinary Rerror; profile peers also get a typed flag.
 * Вернуть Rerror text и явно отметить cycle текущего synchronous callback. Только server owner; без guesses/authority.
 * Вызывать лишь для фактического DVM_VFS_CYCLE; raw peer получает обычный Rerror, profile также typed flag.
 */
char *styxphycycleerror(struct Styxserver *server);


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
