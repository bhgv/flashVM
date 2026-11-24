/* Copyright (c) 2026 bhgv
 * EN: Owner-thread SWF playback exposed through libstyxrpc control files.
 * RU: Выполнение SWF в потоке владельца через файлы управления libstyxrpc.
 */
#ifndef FLASHVM_STYXRPC_PLAYBACK_H
#define FLASHVM_STYXRPC_PLAYBACK_H
#include "flashvm/execution_mode.h"
#include "styxrpc/control.h"

#ifdef __cplusplus
extern "C" {
#endif

struct StyxClient;
typedef struct FVM_StyxRpcPlayback FVM_StyxRpcPlayback; /* Owns the current movie and its control tree. */

typedef struct FVM_StyxRpcPlaybackSpec
{
    FVM_Runtime *runtime;           /* Borrowed runtime for each fresh movie context. */
    struct StyxClient *session;    /* Optional attached Dis VFS root; borrowed through destroy. */
    uint32_t upload_limit;          /* Maximum accepted SWF size in bytes. */
    FVM_ExecutionMode mode;        /* Execution policy of a newly prepared image. */
    void *host;                     /* Borrowed argument shared by optional host callbacks. */
    FVM_Result (*setup)(void *, FVM_Context *); /* Install application bindings in a fresh context. */
    FVM_Result (*debug)(void *, FVM_Context *); /* Attach the host debugger at the paused owner boundary. */
    int (*statistics)(void *, char *, size_t, size_t *); /* Append counted lines of machine/runtime telemetry. */
} FVM_StyxRpcPlaybackSpec;


/* EN: Copy settings and create an empty playback owner, preserving out on error.
 * RU: Скопировать настройки и создать пустой плеер, сохраняя out при ошибке. */
FVM_Result fvm_styxrpc_playback_create(const FVM_StyxRpcPlaybackSpec *spec, FVM_StyxRpcPlayback **out);


/* EN: Borrow the control tree context until destruction of the playback owner.
 * RU: Заимствовать контекст дерева управления до освобождения плеера. */
StyxRpcControl *fvm_styxrpc_playback_control(FVM_StyxRpcPlayback *playback);


/* [ASINC] EN: Advance running playback on the same owner thread as the Styx server.
 * RU: Продвинуть работающий плеер в том же потоке владельца, где обслуживается Styx. */
FVM_Result fvm_styxrpc_playback_tick(FVM_StyxRpcPlayback *playback, unsigned int ticks);


/* EN: Release playback on the owner thread after closing the server and all its fids.
 * RU: Освободить плеер в потоке владельца после закрытия сервера и всех его fid. */
void fvm_styxrpc_playback_destroy(FVM_StyxRpcPlayback *playback);

#ifdef __cplusplus
}
#endif
#endif
