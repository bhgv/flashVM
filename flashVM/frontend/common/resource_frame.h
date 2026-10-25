/* Bounded copied RGBA frames with explicit session-owned leases for chunked resource readers. */
#ifndef FVM_DESKTOP_RESOURCE_FRAME_H
#define FVM_DESKTOP_RESOURCE_FRAME_H /* Guard the additive C89 frame-resource API. */
#include "frame.h"
#include "flashvm/scontrol.h"
#include "flashvm/revents.h"

#define FVM_DESKTOP_RFRAME_VERSION 1UL /* Canonical S-integer metadata/request schema. */
#define FVM_DESKTOP_RFRAME_INFO 1UL    /* READ, empty request: latest metadata without a lease. */
#define FVM_DESKTOP_RFRAME_ACQUIRE 2UL /* READ, empty request: pin latest frame and return metadata. */
#define FVM_DESKTOP_RFRAME_CHUNK                                                                                       \
    3UL                                /* READ: lease ID, generation, offset, count as S integers. \
                                        */
#define FVM_DESKTOP_RFRAME_RELEASE 4UL /* READ: lease ID and generation as S integers. */
#define FVM_DESKTOP_RFRAME_PRESENTED                                                                                   \
    1UL /* Copied metadata event; never implicitly pins pixels.                                    \
         */
#define FVM_DESKTOP_RFRAME_SLOTS 3U               /* Fixed pixel buffers; capture requires a nonlatest unpinned slot. */
#define FVM_DESKTOP_RFRAME_LEASES 8U              /* Independent session-owned pins per adapter. */
#define FVM_DESKTOP_RFRAME_SOURCE 255U            /* Copied origin-prefix bytes, excluding native NUL. */
#define FVM_DESKTOP_RFRAME_SOURCE_TRUNCATED 1UL   /* Source has a longer byte suffix. */
#define FVM_DESKTOP_RFRAME_COUNTERS_TRUNCATED 2UL /* A native frame counter exceeded Uint32. */
#define FVM_DESKTOP_RFRAME_FLAGS 3UL              /* Current metadata flags. */
#define FVM_DESKTOP_RFRAME_FIELDS 17U             /* Integer fields before counted source bytes. */
#define FVM_DESKTOP_RFRAME_CHUNK_FIELDS 4U        /* Lease ID/generation, byte offset and requested count. */
#define FVM_DESKTOP_RFRAME_LEASE_FIELDS 2U        /* Lease ID and generation for release. */
#define FVM_DESKTOP_RFRAME_MAX_BYTES 67108864UL   /* 4096*4096*4 visible RGBA bytes per slot. */
#define FVM_DESKTOP_RFRAME_REGION 5UL             /* READ: lease, x/y and width/height; packed RGBA reply. */
#define FVM_DESKTOP_RFRAME_REGION_FIELDS 6U       /* Lease ID/generation, x, y, width, height. */
#ifdef __cplusplus
extern "C"
{
#endif
/* A lease is scoped to one resource incarnation and authenticated session, never an address. */
typedef struct FVM_DesktopFrameLease
{
    unsigned long id;         /* One-based bounded lease slot; zero only for unpinned metadata. */
    unsigned long generation; /* Nonzero Uint32 incarnation; exhausted slots retire permanently. */
} FVM_DesktopFrameLease;


/* Owned native metadata copy. Serialize fields explicitly; pixels use the CHUNK operation. */
typedef struct FVM_DesktopResourceFrameInfo
{
    FVM_DesktopFrameLease lease;                /* ACQUIRE pin, or zero/zero for INFO/events. */
    unsigned long sequence;                     /* Successful captures in this adapter lifetime, never wraps. */
    unsigned long format;                       /* FRAME_RGBA8, straight alpha, explicit byte order R/G/B/A. */
    unsigned long width;                        /* Logical stage width in pixels. */
    unsigned long height;                       /* Logical stage height in pixels. */
    unsigned long row_bytes;                    /* Packed width*4; source padding is not exposed. */
    unsigned long byte_count;                   /* Exact packed visible extent. */
    unsigned long movie_ticks;                  /* Observed current-root completed ticks. */
    unsigned long total_ticks;                  /* Observed run-wide completed ticks. */
    unsigned long presentations;                /* Observed current-root presentations. */
    unsigned long total_presentations;          /* Observed run-wide presentations. */
    unsigned long reason;                       /* FRAME_TICK or FRAME_REFRESH. */
    unsigned long flags;                        /* Explicit source/counter truncation flags. */
    unsigned long dropped;                      /* Capture BUSY count since create; saturates at Uint32 max. */
    char source[FVM_DESKTOP_RFRAME_SOURCE + 1]; /* Bytewise origin prefix, native NUL-terminated. */
} FVM_DesktopResourceFrameInfo;
typedef struct FVM_DesktopResourceFrame FVM_DesktopResourceFrame; /* Opaque owner-only native cache. */


/* Owner allocates exactly SLOTS pixel buffers of capacity bytes, plus fixed metadata, then
     * registers five READ operations. Capacity 1..MAX_BYTES; no SDK startup, grant or hidden
     * default allocation. Outputs only on OK; allocation/registration failure frees all partial
     * storage. Borrows dispatcher until destroy. Host quiesces calls and keeps adapter stable; it
     * owns no managed pointers. */
FVM_DESKTOP_API FVM_RResult fvm_desktop_resource_frame_create(
    FVM_RDispatch *, const char *path, size_t capacity, FVM_DesktopResourceFrame **, FVM_RHandle *);


/* Owner frame callback copies visible rows to a spare slot, publishes metadata, then commits
     * latest. No allocation/VM/I/O. Invalid geometry INVALID; configured capacity/sequence
     * exhaustion LIMIT. All slots pinned/no spare: BUSY leaves current frame and leases intact and
     * increments dropped. Other failures preserve published cache. Capture is never a network
     * delivery acknowledgement. */
FVM_DESKTOP_API FVM_RResult fvm_desktop_resource_frame_capture(FVM_DesktopResourceFrame *, const FVM_DesktopFrame *);


/* Direct frame-observer callback: userdata=adapter. OK/BUSY continue playback; BUSY
     * intentionally drops only export. INVALID maps INVALID_ARGUMENT, other failures ERROR. Host
     * may instead call capture and choose its own policy. No pointer from the callback is retained.
     */
FVM_DESKTOP_API FVM_Result fvm_desktop_resource_frame_notify(const FVM_DesktopFrame *, void *);


/* Strict decode of one metadata payload inside an existing resource event/reply; output is
     * owned and unchanged on error. Validates schema, leases, packed dimensions, flags, source and
     * exact size. Client-side, no VM/native adapter access, allocation or borrowed output. */
FVM_DESKTOP_API FVM_RResult fvm_desktop_resource_frame_read(const void *, size_t, FVM_DesktopResourceFrameInfo *);


/* Owner removes this resource's session rights and all its leases. Quiesce that transport
     * first; regrant cannot resurrect old pins. Call alongside dispatcher_disconnect on connection
     * teardown. A plain external grant removal alone does not free adapter-specific leases. */
FVM_DESKTOP_API FVM_RResult fvm_desktop_resource_frame_revoke(FVM_DesktopResourceFrame *, unsigned long session);


/* Owner quiesces callbacks/clients first. Unregister exact incarnation, invalidate pins and
     * free native buffers; BUSY preserves pointer. Queued old jobs later complete UNKNOWN.
     * Dispatcher, client files and SDK remain host-owned. Success nulls pointer, even if leases
     * were outstanding. */
FVM_DESKTOP_API FVM_RResult fvm_desktop_resource_frame_destroy(FVM_DesktopResourceFrame **);


/* Owner-only bulk export from an existing immutable lease; current session must still have
     * READ authority. Uses a committed boundary-aware media agreement, not pending offers.
     * Stream/epoch come from that agreement, object is the captured frame sequence, total is its
     * exact packed byte count. Host supplies FILE/LIVE mode and relative Uint32 time_ms.
     * Encode one agreed-size slot directly from retained pixels; no whole-image scratch, VM,
     * allocation, queueing or socket I/O. Caller owns disjoint output; used/output unchanged on
     * error. UNKNOWN stale lease, DENIED foreign/revoked access, BUSY before agreement,
     * CONFLICT before activation object, LIMIT for object/part-count/capacity quota, INVALID
     * for bad mode/index/profile. Closing the output/transport does not release the lease. */
FVM_DESKTOP_API FVM_RResult fvm_desktop_resource_frame_write_part(FVM_DesktopResourceFrame *,
                                                                  unsigned long session,
                                                                  FVM_DesktopFrameLease,
                                                                  const FVM_SControlExchange *,
                                                                  unsigned long mode,
                                                                  unsigned long time_ms,
                                                                  unsigned long part,
                                                                  void *output,
                                                                  size_t capacity,
                                                                  size_t *used);
#ifdef __cplusplus
}
#endif
#endif
