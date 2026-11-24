/* Copyright (c) 2026 bhgv
 * EN: Owner callbacks exposed as counted upload and text state files.
 * RU: Колбэки владельца в файлах загрузки с длиной и текстового состояния.
 */
#ifndef STYXRPC_CONTROL_H
#define STYXRPC_CONTROL_H
#include "styxrpc/serve.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STYXRPC_UPLOAD_PREFIX 4U /* Unsigned big-endian payload length in the write stream. */
#define STYXRPC_STATE_LIMIT 4096U /* Maximum bytes in an immutable per-fid status snapshot. */

typedef enum StyxRpcControlCommand
{
    STYXRPC_CONTROL_START,   /* Start or resume the current image. */
    STYXRPC_CONTROL_PAUSE,   /* Pause execution at the owner boundary. */
    STYXRPC_CONTROL_RESTART, /* Replace current state with a fresh execution of the same image. */
    STYXRPC_CONTROL_DEBUG    /* Enter the owner's debug mode. */
} StyxRpcControlCommand;

typedef struct StyxRpcControlSpec
{
    void *context;             /* Owner state borrowed until every server is retired. */
    uint32_t upload_limit;     /* Maximum byte length of one payload. */
    int (*upload)(void *, const void *, size_t);                  /* Borrow complete payload and return 1 on commit. */
    int (*command)(void *, StyxRpcControlCommand);                /* Apply one exact recognized word. */
    int (*status)(void *, char *, size_t, size_t *);              /* Copy a text snapshot, returning 1 on success. */
} StyxRpcControlSpec;

typedef struct StyxRpcControl StyxRpcControl; /* Owns descriptor and serializes a single staged upload. */


/* EN: Copy the callbacks into an owner-local control tree, preserving out on failure.
 * RU: Скопировать колбэки в дерево владельца, сохраняя out при ошибке. */
StyxRpcStatus styxrpc_control_create(const StyxRpcControlSpec *spec, StyxRpcControl **out);


/* EN: Destroy only after all using servers have closed their fids. NULL is accepted.
 * RU: Освободить после закрытия fid во всех зависимых серверах. Допускается NULL. */
void styxrpc_control_destroy(StyxRpcControl *control);

extern const StyxRpcTree styxrpc_control_tree; /* Publish .upload and .state directly at the root. */

#ifdef __cplusplus
}
#endif
#endif
