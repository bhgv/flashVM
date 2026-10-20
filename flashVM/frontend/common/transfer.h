/* C89 background byte-transfer jobs; VM/SDL UI remain exclusively on the owner thread. */
#ifndef FVM_DESKTOP_TRANSFER_H
#define FVM_DESKTOP_TRANSFER_H
#include "source.h"
#include "../../ports/network/socket.h"
#define FVM_DESKTOP_TRANSFER_PENDING 0U /* Worker still owns mutable input/result storage. */
#define FVM_DESKTOP_TRANSFER_DONE 1U    /* Complete immutable bytes/provenance are available. */
#define FVM_DESKTOP_TRANSFER_FAILED                                                                \
    2U /* Provider/validation/sink failed; no payload is exposed.                                  \
        */
#define FVM_DESKTOP_TRANSFER_CANCELLED 3U /* Cancellation won before terminal publication. */
#define FVM_DESKTOP_TRANSFER_TIMED_OUT 4U /* Cooperative deadline expired before publication. */
#define FVM_DESKTOP_TRANSFER_MAX_TIMEOUT_MS 2147483647UL /* At most half a 32-bit tick period. */
#define FVM_DESKTOP_TRANSFER_MAX_BYTES 67108864UL /* Independent decoded-byte bound per job. */
#define FVM_DESKTOP_TRANSFER_INITIAL 4096U        /* Initial accumulation allocation. */
#define FVM_DESKTOP_TRANSFER_MAX_JOBS 64          /* Maximum live handles in this SDK instance. */
#ifdef __cplusplus
extern "C"
{
#endif
    /* Provider executes on a worker. It must be thread-safe and must not enter VM/SDL UI.
     * Strings/control/sink are borrowed through return; no callback may throw across C.
     * Cancellation cannot interrupt arbitrary provider/DNS/user code: release always joins. */
    typedef FVM_Result (*FVM_DesktopTransferFetch)(const char *reference, const char *base,
                                                   FVM_DesktopWrite write, void *write_userdata,
                                                   char *resolved, size_t capacity,
                                                   const FVM_NetControl *control, void *userdata);
    /* Initialize before use. Caller-owned userdata (including nested strings) must outlive release.
     */
    typedef struct FVM_DesktopTransferOptions
    {
        FVM_DesktopTransferFetch fetch; /* Required worker-safe provider. */
        void *userdata;                 /* Borrowed through worker completion and release. */
        size_t max_bytes; /* 1..MAX_BYTES; empty successful resources are permitted. */
    } FVM_DesktopTransferOptions;
    /* Opaque owner handle; release on its creating thread, never from a provider callback. */
    typedef struct FVM_DesktopTransfer FVM_DesktopTransfer;
    /* Snapshot metadata, with immutable payload only after DONE; no VM objects cross threads. */
    typedef struct FVM_DesktopTransferView
    {
        unsigned int state;   /* One of TRANSFER_PENDING/DONE/FAILED/CANCELLED. */
        FVM_Result result;    /* NOT_FOUND while pending, exact provider/sink result on failure. */
        const void *bytes;    /* Borrowed through release, null unless DONE with a nonempty body. */
        size_t size;          /* Delivered body size when DONE, zero otherwise. */
        const char *location; /* Borrowed canonical provenance when DONE, null otherwise. */
        size_t received; /* Progress bytes accumulated so far, including a later failed/cancelled
                            body. */
    } FVM_DesktopTransferView;
    /* Initialize defaults with no provider; 1 success, 0 null, no native startup. */
    FVM_DESKTOP_API int fvm_desktop_transfer_options_init(FVM_DesktopTransferOptions *out);
    /* Copy reference/base, reserve a bounded handle and launch one worker. Null/invalid input
     * returns INVALID_ARGUMENT, native allocation/thread failure OUT_OF_MEMORY, quota ERROR.
     * On failure *out is unchanged. No files/network/VM run on the calling thread. */
    FVM_DESKTOP_API FVM_Result fvm_desktop_transfer_begin(const char *reference, const char *base,
                                                          const FVM_DesktopTransferOptions *options,
                                                          FVM_DesktopTransfer **out);
    /* Additive deadline variant. Zero disables timeout; values above MAX_TIMEOUT_MS fail
     * without changing *out. Time starts before worker launch and includes provider work.
     * Deadline requests cooperative cancellation; DNS/user code may delay terminal publication
     * and release. TIMED_OUT has ERROR/no payload, retaining received progress. This is not
     * a hard realtime preemption guarantee. First explicit cancellation/deadline reason wins. */
    FVM_DESKTOP_API FVM_Result fvm_desktop_transfer_begin_timed(
        const char *reference, const char *base, const FVM_DesktopTransferOptions *options,
        unsigned long timeout_ms, FVM_DesktopTransfer **out);
    /* Copy a synchronized snapshot; null returns INVALID_ARGUMENT without modifying output. */
    FVM_DESKTOP_API FVM_Result fvm_desktop_transfer_poll(FVM_DesktopTransfer *job,
                                                         FVM_DesktopTransferView *out);
    /* Request cancellation before terminal publication. 1 accepted, 0 terminal/null.
     * Cancel does not free storage, close another thread's socket, join or revoke a DONE view. */
    FVM_DESKTOP_API int fvm_desktop_transfer_cancel(FVM_DesktopTransfer *job);
    /* Cancel and join, then release every owned byte/mutex/thread; null is OK. Can block
     * until a noncooperative provider or DNS returns. Wrong thread gives INVALID_ARGUMENT.
     * Do not concurrently release/poll/cancel; never unload the SDK before release returns. */
    FVM_DESKTOP_API FVM_Result fvm_desktop_transfer_release(FVM_DesktopTransfer *job);
    /* Live handles, including terminal ones not yet released, in this SDK instance. */
    FVM_DESKTOP_API unsigned int fvm_desktop_transfer_active(void);
#ifdef __cplusplus
}
#endif
#endif
