/* Copyright (c) 2026 bhgv
 * EN: Scalar native 9P client boundary, using original Inferno codecs internally.
 * RU: Скалярная native-граница 9P client с исходными кодеками Inferno внутри.
 */
#ifndef STYX_CLIENT_H
#define STYX_CLIENT_H
#include "file.h"

typedef struct StyxClient StyxClient; /* Owned connection reference. */
typedef struct StyxCancel StyxCancel; /* Owned sticky native cancellation capability. */
typedef struct StyxFid StyxFid; /* Owned independent remote fid and connection pin. */


/* EN/RU: Native stream callbacks; positive bounded counts,0 EOF,-1 error.
 * Host supplies bounded I/O/deadline and trusted connection; no guest callbacks.
 * close consumes state once without waiting for protocol or external work. Code/state live until close. No implicit authentication.
 * Native stream callbacks; host задаёт bounded I/O и доверенное соединение. */
typedef struct StyxTransport
{
    void *state; /* Owned stream state, no VM pointers. */
    long (*read)(void *, void *, size_t); /* Borrow bytes for one native receive. */
    long (*write)(void *, const void *, size_t); /* Borrow bytes for one native send. */
    void (*close)(void *); /* Consuming stream teardown. */
    int (*begin)(void *); /* Establish one total RPC I/O deadline;0 refuses before wire effects. */
} StyxTransport;


/* EN/RU: Consume a valid transport on every outcome; negotiate256..65536 bytes and
 * attach NOFID with copied host-supplied identity hints. Invalid callback struct
 * is not adopted. Caller serializes creation; returned connection is owned/NULL.
 * Принять valid transport всегда; negotiate+attach, свой client/NULL. */
StyxClient *styxclientmount(const StyxTransport *transport, unsigned msize, const char *user, const char *name);


/* EN/RU: Consume connected POSIX stream fd always; independent native owner supports
 * up to limit1..128 outstanding RPCs. timeout1..120000ms per request, then at most
 * one extra timeout interval for flush. No transport authentication is implied.
 * Принять connected POSIX fd; отдельный native owner, bounded RPC/flush. */
StyxClient *styxclientmountfd(int fd, unsigned msize, const char *user, const char *name, unsigned timeout, unsigned limit);


/* EN/RU: Configure fd-owner heartbeat without protocol wait;1/0. All zeros disables.
 * idle/reply1..120000ms, misses2..16; NULL/legacy/terminal/invalid refuses unchanged.
 * One probe retains its tags/deadline through reconfiguration or disabling.
 * Настроить fd-owner heartbeat без ожидания;1/0, три нуля отключают будущие probes. */
int styxclientkeepalive(StyxClient *client, unsigned idle, unsigned reply, unsigned misses);


/* EN/RU: Retain borrowed live client;1/0 overflow. Drop with clientfree.
 * Удержать borrowed live client;1/0 overflow, release через clientfree. */
int styxclientretain(StyxClient *client);


/* EN/RU: Explicit shutdown on native I/O worker, may wait for native owner retirement.
 * No guest execution/pause; final clientfree itself remains nonwaiting.
 * Явный native shutdown может ждать owner retirement; clientfree не ждёт. */
void styxclientshutdown(StyxClient *client);


/* EN/RU: Owned sticky cancellation token/NULL; request is nonwaiting and thread-safe.
 * One admitted operation per token; admitted operation retains it independently.
 * Caller can free after admission; freeing alone does not request cancellation.
 * Свой sticky token/NULL; request thread-safe и без ожидания, один operation/token. */
StyxCancel *styxcancelcreate(void);


/* EN/RU: Request sticky cancellation from a retained native capability; no protocol wait.
 * Запросить sticky cancellation с retained capability; без protocol wait. */
void styxcancelrequest(StyxCancel *cancel);


/* EN/RU: Consume caller token ref; admitted work retains its own ref, no implicit cancel.
 * Поглотить caller token ref; admitted work имеет свою ref, без implicit cancel. */
void styxcancelfree(StyxCancel *cancel);


/* EN/RU: Snapshot terminal transport/framing state under connection lock;1/0.
 * Not a heartbeat or a permission check. Снимок terminal state; не heartbeat/права. */
int styxclientalive(StyxClient *client);


/* EN/RU: Copy immutable attached root kind,1 directory/0 file/-1 NULL or auth root.
 * Caller retains client; no protocol request or mutable server metadata query.
 * Копировать immutable attach root kind:1 directory/0 file/-1 invalid; без protocol request. */
int styxclientrootdirectory(StyxClient *client);


#define STYX_LINK_FAILED 1 /* Terminal transport/framing failure. */
#define STYX_LINK_CLOSED 2 /* Explicit native shutdown, not normal final owner drop. */
typedef struct StyxWatch StyxWatch; /* Owned native terminal subscription. */

#define STYX_LINK_READY 3 /* Responsive/initial source; heartbeat success after suspicion. */
#define STYX_LINK_SUSPECT 4 /* Sent probe has missed a response window, connection retained. */
#define STYX_LINK_STALLED 5 /* Probe cannot finish sending within a send window. */
#define STYX_LINK_TIMEOUT 6 /* Heartbeat policy exhausted; does not prove remote death. */


/* EN/RU: Copied local connection observation, no permission or remote object identity.
 * Копия local observation соединения, без permissions/remote object identity. */
typedef struct StyxLinkEvent
{
    unsigned long long generation; /* Monotonic local observation version, initially1. */
    int state; /* STYX_LINK_*; timeout is policy failure rather than proof of death. */
} StyxLinkEvent;


/* EN/RU: Native nonwaiting observation outside source/client/engine locks; no guest,
 * provider/transport I/O or target wait. Nonblocking native readiness is allowed.
 * Native observation вне locks; без guest/provider/transport I/O/target wait;
 * nonblocking native readiness разрешён. */
typedef void (*StyxLinkReceive)(void *state, const StyxLinkEvent *event);


/* EN/RU: Own state only on success; persistent watch/NULL, immediate current snapshot.
 * Caller pins client; ordered generations fence replay racing with native delivery.
 * Detach with clientunwatch; release follows all admitted callbacks, no guest calls.
 * Persistent watch/NULL принимает state при успехе; snapshot немедленно, generations ordered. */
StyxWatch *styxclientobserve(StyxClient *client, StyxLinkReceive receive, void *state, void (*release)(void *));


/* EN/RU: Copy local latest observation without I/O;1/0, output untouched on invalid input.
 * Копировать latest local observation без I/O;1/0, invalid не меняет output. */
int styxclientlink(StyxClient *client, StyxLinkEvent *output);


/* EN/RU: Nonwaiting native terminal delivery outside source/client/engine locks.
 * May copy an event/unsubscribe, never perform provider/transport I/O, guest calls or wait for a target.
 * Nonblocking native readiness notification is allowed.
 * Native terminal callback вне locks; без provider/transport I/O/guest calls/target wait;
 * native readiness signal разрешён. */
typedef void (*StyxTerminal)(void *state, int reason);


/* EN/RU: Adopt state only on success, owned watch/NULL; terminal state replays once.
 * At most256 live watcher nodes/client. Retain client while registering. State
 * outlives already admitted callbacks; release runs after final unsubscribe/flight.
 * Принять state только при успехе; свой watch/NULL, sticky terminal replay. */
StyxWatch *styxclientwatch(StyxClient *client, StyxTerminal notify, void *state, void (*release)(void *));


/* EN/RU: Consume watch without waiting; an already admitted callback may finish.
 * Поглотить watch без ожидания; admitted callback может завершиться позже. */
void styxclientunwatch(StyxWatch *watch);


/* EN/RU: Drop connection reference after callers retire; live fids pin transport.
 * Final release closes transport once; server retires root on disconnect. NULL allowed.
 * Поглотить ссылку; live fids удерживают transport; NULL допустим. */
void styxclientfree(StyxClient *client);


/* EN/RU: Walk canonical relative path, dot for root; owned independent fid/NULL.
 * Reject absolute/empty/dotdot/embedded dot components before wire I/O. Multiple
 * MAXWELEM batches preserve the same new fid. Calls serialized by connection mutex.
 * Пройти canonical relative path; свой fid/NULL, без выхода выше root. */
StyxFid *styxclientwalk(StyxClient *client, const char *relative);


/* EN/RU: Walk from a borrowed pinned unopened fid; dot clones, owned independent result/NULL.
 * Serialize base open/clunk against this call; concurrent walks are allowed. File descendants
 * and unsafe paths refuse before wire effects. No fallback to the original attach pathname.
 * Пройти от pinned unopened fid; dot клонирует, свой result/NULL; без fallback к attach pathname. */
StyxFid *styxclientwalkfrom(StyxFid *base, const char *relative);


/* EN/RU: Open unopened fid in mode0/1/2 without create/truncate;1/0.
 * Caller serializes a fid's data calls. Ropen iounit limits every later chunk.
 * Открыть fid без create/truncate;1/0, iounit ограничивает chunk. */
int styxclientopen(StyxFid *fid, int mode);


/* EN/RU: Query current remote fid metadata;1/0, output changes only on success.
 * Tstat не открывает путь заново;1/0, output только при успехе. */
int styxclientstat(StyxFid *fid, StyxMeta *output);


/* EN/RU: One actual Tread/Twrite chunk, signed64 offset range; exact confirmed
 * prefix, no auto retry. Uncertain data effects poison further data calls on fid.
 * Один Tread/Twrite chunk; точный prefix, без retry; uncertain effects запрещают I/O. */
void styxclientread(StyxFid *fid, unsigned long long offset, void *bytes, size_t size, StyxTransfer *result);
void styxclientwrite(StyxFid *fid, unsigned long long offset, const void *bytes, size_t size, StyxTransfer *result);


/* EN/RU: Cancellable chunk requires mountfd client; same precise-prefix contract.
 * Original acknowledgement before Rflush wins; otherwise cancelled is set, with
 * uncertain effects if original bytes were sent. Caller serializes the fid.
 * Cancellable chunk требует mountfd; original acknowledgement не теряется. */
void styxclientreadcancel(StyxFid *fid, unsigned long long offset, void *bytes, size_t size, StyxCancel *cancel, StyxTransfer *result);
void styxclientwritecancel(StyxFid *fid, unsigned long long offset, const void *bytes, size_t size, StyxCancel *cancel, StyxTransfer *result);


/* EN/RU: Local capability snapshot;1 if live fd owner has at least two RPC slots,0 otherwise.
 * Local capability snapshot;1 для live fd owner минимум с двумя RPC slots. */
int styxclientwatchable(StyxClient *client);


/* EN/RU: Long-lived native read without ordinary request deadline; explicit token required.
 * Same precision/flush fences as readcancel. Heartbeat/terminal/cancel still operate.
 * Caller pins/serializes fid and token; requires watchable client, uses one RPC slot.
 * Долгий native read без обычного deadline; explicit token, heartbeat/cancel сохраняются. */
void styxclientreadwatch(StyxFid *, unsigned long long, void *, size_t, StyxCancel *, StyxTransfer *);


/* EN/RU: Decode one complete original stat record; bytes borrowed,1/0 and consumed.
 * Reject malformed/truncated records and unsafe basename, no partial output.
 * Разобрать одну полную stat-запись;1/0 и consumed, без unsafe basename. */
int styxclientrecord(const void *bytes, size_t size, StyxMeta *output, size_t *consumed);


/* EN/RU: Consume one fid, clunk if transport usable, then drop its connection pin.
 * Поглотить fid, clunk если transport пригоден; NULL допустим. */
void styxclientclunk(StyxFid *fid);
#endif
