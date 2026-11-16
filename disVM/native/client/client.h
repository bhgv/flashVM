/* Copyright (c) 2026 bhgv
 * EN: Scalar native 9P client boundary, using original Inferno codecs internally.
 * RU: Скалярная native-граница 9P client с исходными кодеками Inferno внутри.
 */
#ifndef STYX_CLIENT_H
#define STYX_CLIENT_H
#include "file.h"

typedef struct StyxClient StyxClient; /* Owned connection reference. */
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


/* EN/RU: Snapshot terminal transport/framing state under connection lock;1/0.
 * Not a heartbeat or a permission check. Снимок terminal state; не heartbeat/права. */
int styxclientalive(StyxClient *client);


/* EN/RU: Drop connection reference after callers retire; live fids pin transport.
 * Final release closes transport once; server retires root on disconnect. NULL allowed.
 * Поглотить ссылку; live fids удерживают transport; NULL допустим. */
void styxclientfree(StyxClient *client);


/* EN/RU: Walk canonical relative path, dot for root; owned independent fid/NULL.
 * Reject absolute/empty/dotdot/embedded dot components before wire I/O. Multiple
 * MAXWELEM batches preserve the same new fid. Calls serialized by connection mutex.
 * Пройти canonical relative path; свой fid/NULL, без выхода выше root. */
StyxFid *styxclientwalk(StyxClient *client, const char *relative);


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


/* EN/RU: Decode one complete original stat record; bytes borrowed,1/0 and consumed.
 * Reject malformed/truncated records and unsafe basename, no partial output.
 * Разобрать одну полную stat-запись;1/0 и consumed, без unsafe basename. */
int styxclientrecord(const void *bytes, size_t size, StyxMeta *output, size_t *consumed);


/* EN/RU: Consume one fid, clunk if transport usable, then drop its connection pin.
 * Поглотить fid, clunk если transport пригоден; NULL допустим. */
void styxclientclunk(StyxFid *fid);
#endif
