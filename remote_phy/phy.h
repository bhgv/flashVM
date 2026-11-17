/* Copyright (c) 2026 bhgv
 * EN: Carrier-independent native transport capabilities, precise byte I/O and interruptible readiness.
 * RU: Независимые от носителя native transport capabilities, точное byte I/O и interruptible readiness.
 */
#ifndef REMOTE_PHY_H
#define REMOTE_PHY_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct RemotePhy RemotePhy; /* Owned native carrier, never a VM or wire pointer. */
#define PHY_OK 0 /* Operation succeeded, count is authoritative only with PREFIX. */
#define PHY_AGAIN 1 /* Temporary lack of progress, never EOF. */
#define PHY_EOF 2 /* Read direction ended, count must be zero. */
#define PHY_ERROR (-1) /* Permanent I/O or malformed-driver result. */
#define PHY_UNSUPPORTED (-2) /* Driver does not support the requested optional operation. */
#define PHY_PREFIX 1U /* Count is the confirmed leading extent. */
#define PHY_UNCERTAIN 2U /* Effects beyond prefix unknown; never replay automatically. */
#define PHY_READ 1U /* Wait for read readiness. */
#define PHY_WRITE 2U /* Wait for write readiness. */
#define PHY_WAKE 4U /* Local native work/cancellation interrupted the wait. */
#define PHY_HANGUP 8U /* Drain readable prefix, then observe EOF/error. */
#define PHY_FAILED 16U /* Readiness source failed. */
#define PHY_STREAM 1U /* Driver exposes a byte stream, not packet boundaries. */
#define PHY_ORDERED 2U /* Bytes retain order. */
#define PHY_RELIABLE 4U /* Loss/duplication/corruption handled below this interface or reported as failure. */
#define PHY_HALF_DUPLEX 8U /* Driver arbitrates turn-taking below logical byte I/O. */
#define PHY_PACKET 16U /* Raw message carrier; requires a suitable adaptation before 9P. */


/* EN/RU: Copied native acknowledgement; not a public protocol structure.
 * Native acknowledgement, не public protocol structure. */
typedef struct PhyTransfer
{
    size_t count; /* Confirmed prefix only. */
    int status; /* OK/AGAIN/EOF/ERROR; UNSUPPORTED only for optional operations. */
    unsigned flags; /* PREFIX and optional UNCERTAIN. */
} PhyTransfer;


/* EN/RU: Driver capabilities copied at creation; connected is local lifecycle, never a peer permission.
 * Driver capabilities copied при creation; connected — local lifecycle, не peer permission. */
typedef struct PhyInfo
{
    unsigned capabilities; /* Explicit STREAM/ORDERED/RELIABLE or PACKET and optional HALF_DUPLEX. */
    size_t mtu; /* Physical message hint,0 unknown/not applicable. */
    size_t max_read; /* Maximum callback request,0 no additional limit. */
    size_t max_write; /* Maximum callback request,0 no additional limit. */
    int connected; /* Set by successful connect/adoption, cleared by disconnect. */
} PhyInfo;


/* EN/RU: Native driver callbacks; read/write never block and borrow buffers until return.
 * Wait may block up to timeout ms (-1 infinite); wake must interrupt it, even before wait starts.
 * One owner serializes all except wake, which is thread-safe/nonwaiting until destroy.
 * connect parameters are borrowed driver-specific native data, not a URL or authentication.
 * destroy consumes state after I/O/wake callers retire; state adopted only on successful phycreate.
 * Native callbacks: read/write nonblocking; wait interruptible через sticky wake, один I/O owner.
 * connect получает borrowed native parameters; destroy после users, без guest/VM callbacks. */
typedef struct PhyOps
{
    PhyTransfer (*read)(void *, void *, size_t); /* Nonblocking read, explicit EOF. */
    PhyTransfer (*write)(void *, const void *, size_t); /* Nonblocking precise write. */
    int (*wait)(void *, unsigned, int, unsigned *); /* Requested readiness or WAKE/HANGUP/FAILED. */
    void (*wake)(void *); /* Nonwaiting sticky local readiness, no wire message. */
    int (*available)(void *, size_t *); /* Optional available-byte hint, UNSUPPORTED if unknown. */
    int (*connect)(void *, const void *, size_t); /* Optional native connection setup. */
    void (*disconnect)(void *); /* Idempotent wire close; wake remains usable until destroy. */
    void (*destroy)(void *); /* Consume native driver storage, no protocol/guest wait. */
} PhyOps;


/* EN/RU: Borrow ops/info, adopt state only on owned success; NULL invalid/OOM.
 * Borrow ops/info; state принят только при success, NULL invalid/OOM. */
RemotePhy *phycreate(const PhyOps *ops, void *state, const PhyInfo *info);


/* EN/RU: Copy local capabilities/lifecycle;1/0, owner-serialized, no driver/transport I/O.
 * Копировать local info;1/0, owner-serialized, без driver I/O. */
int phyinfo(RemotePhy *phy, PhyInfo *out);


/* EN/RU: Owner-serialized nonblocking read/write, borrowed buffers; validate precise driver results.
 * Size0 succeeds without a callback; cap hints bound a single driver request.
 * Owner-serialized read/write, buffers borrowed; size0 без callback, cap limits учитываются. */
PhyTransfer phyread(RemotePhy *phy, void *bytes, size_t size);


/* EN/RU: Owner-serialized precise write of borrowed bytes/size; no replay or splitting on failure.
 * Owner-serialized precise write borrowed bytes/size; без replay/splitting при failure. */
PhyTransfer phywrite(RemotePhy *phy, const void *bytes, size_t size);


/* EN/RU: Owner waits for requested READ/WRITE; timeout>=-1,0 polls, returns OK/ERROR and ready bits.
 * No OS descriptor in this contract; driver integrates its own bus/IRQ/event mechanism.
 * Owner ждёт READ/WRITE, timeout>=-1;0 poll; OK/ERROR и ready bits, без требования OS fd. */
int phywait(RemotePhy *phy, unsigned wanted, int timeout, unsigned *ready);


/* EN/RU: Nonwaiting native interrupt while lifetime pinned; NULL harmless, no wire traffic.
 * Native interrupt под lifetime pin; NULL harmless, без wire traffic. */
void phywake(RemotePhy *phy);


/* EN/RU: Optional local byte hint, not EOF or permission; owner-serialized, output unchanged on failure.
 * Optional byte hint, не EOF/permission; owner-serialized, output сохраняется при failure. */
int phyavailable(RemotePhy *phy, size_t *bytes);


/* EN/RU: Owner connects with borrowed driver-native parameters; OK updates local connected state.
 * Live carrier accepts only empty idempotent connect, not reconfiguration.
 * Owner connect с borrowed driver-native parameters; OK обновляет connected. */
int phyconnect(RemotePhy *phy, const void *parameters, size_t size);


/* EN/RU: Owner disconnects once; destroy/free only after every I/O/wake user retires.
 * Owner disconnect один раз; free после всех I/O/wake users. */
void phydisconnect(RemotePhy *phy);


/* EN/RU: Consume owned carrier after all I/O/wake callers retire; NULL harmless.
 * Поглотить owned carrier после всех I/O/wake callers; NULL harmless. */
void phyfree(RemotePhy *phy);


/* EN/RU: Host-selected factories; no global registry, auto-loading or guessed network authority.
 * Host-selected factories без global registry/auto-loading/authority по имени. */
typedef struct PhyDriver
{
    const char *name; /* Borrowed immutable selector, caller-defined namespace. */
    RemotePhy *(*open)(const void *, size_t); /* Borrow parameters; handle ownership defined by driver. */
} PhyDriver;


/* EN/RU: Select exactly one matching driver from borrowed table; NULL absent/ambiguous/invalid.
 * Driver-specific native parameters borrowed until its open returns; no fallback to another carrier.
 * Выбрать один driver из borrowed table; NULL absent/ambiguous/invalid, без fallback. */
RemotePhy *physelect(const PhyDriver *drivers, size_t count, const char *name,
    const void *parameters, size_t size);
#ifdef __cplusplus
}
#endif
#endif
