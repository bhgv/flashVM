/* Bounded 9P2000 message adapter for native resource files; transport remains host-owned. */
#ifndef FLASHVM_R9P_H
#define FLASHVM_R9P_H                 /* Guard the public C89 server adapter. */
#include "rfs.h"
#define FVM_R9P_MSIZE 4096U           /* Maximum frame and required caller output capacity. */
#define FVM_R9P_MIN_MSIZE 256U        /* Minimum accepted negotiated frame capacity. */
#define FVM_R9P_FIDS FVM_RFS_FILES    /* Per-connection fid and pending-read bound. */
#define FVM_R9P_WELEM 16U             /* Standard maximum walk components in one message. */
#define FVM_R9P_NOTAG 65535U          /* Reserved version tag, never an ordinary request tag. */
#define FVM_R9P_NOFID FVM_SRECORD_U32 /* Reserved unused authentication fid. */
#ifdef __cplusplus
extern "C"
{
#endif
typedef struct FVM_R9P FVM_R9P; /* Opaque native connection; no sockets or VM pointers. */


/* Allocate fixed server state borrowing an authenticated FS through destroy. The host
     * serializes all calls, assigns one FS per connection and owns authentication and ordered
     * reliable I/O. The FS must not be used/destroyed independently while the adapter owns its open
     * descriptors. */
FVM_API FVM_RResult fvm_r9p_create(FVM_RFS *, FVM_R9P **);


/* Process exactly one frame, accepting a trailing next frame without consuming it. Output
     * capacity must be >=MSIZE before any mutation; buffers/outputs must not overlap. Incomplete
     * input PENDING, framing failure INVALID/LIMIT, duplicate pending tag CONFLICT:
     * outputs/consumed unchanged. A valid request returns OK, consumed=frame size and used=reply
     * size, or used=0 for a retained pending Tread. Protocol/service failures are Rerror replies,
     * not native call errors. No VM pump. The host retains/sends every produced reply in order,
     * including before later Rflush replies. */
FVM_API FVM_RResult fvm_r9p_request(FVM_R9P *, const void *, size_t, void *, size_t capacity,
                                    size_t *used, size_t *consumed);


/* Poll retained reads without blocking; returns one ready reply and releases its tag on OK.
     * PENDING leaves output/used untouched. Same MSIZE capacity and ordering requirements as
     * request. Tflush suppresses only an unproduced pending read; it cannot undo an accepted
     * operation write. */
FVM_API FVM_RResult fvm_r9p_poll(FVM_R9P *, void *, size_t capacity, size_t *used);


/* Quiesce transport and owner callbacks first. Closes fids, discards pending reads and frees
     * state. BUSY may have closed earlier fids; retry cleanup after running handlers finish. On
     * success pointer becomes NULL; borrowed FS and dispatcher remain host-owned and must be
     * destroyed afterwards. */
FVM_API FVM_RResult fvm_r9p_destroy(FVM_R9P **);
#ifdef __cplusplus
}
#endif
#endif
