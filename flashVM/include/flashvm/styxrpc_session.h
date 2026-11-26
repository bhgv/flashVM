/* Copyright (c) 2026 bhgv
 * EN: ActionScript bindings for delegated Styx files and named program tickets.
 * RU: Привязки ActionScript для выданных файлов Styx и именованных запусков.
 */
#ifndef FLASHVM_STYXRPC_SESSION_H
#define FLASHVM_STYXRPC_SESSION_H
#include "value.h"
#include "styxrpc/launch.h"
#include "styxrpc/vfs_client.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FVM_STYXRPC_LAUNCH_SLOTS 8U  /* Maximum live launch fids per binding. */
#define FVM_STYXRPC_SCRIPT_BYTES 2048U /* Maximum bytes in one AS file transfer. */
#define FVM_STYXRPC_ABSOLUTE_PATH (STYXRPC_VFS_PATH_MAX + 2U) /* Leading slash and NUL. */

typedef struct FVM_StyxRpcTicket
{
    struct StyxFid *fid;         /* Owned through close or binding teardown. */
    uint32_t token;              /* Exact positive AS Number, never reused. */
    StyxRpcLaunchStatus status;  /* Last confirmed pending or terminal completion. */
} FVM_StyxRpcTicket;

typedef struct FVM_StyxRpcSession
{
    struct StyxClient *client;  /* Borrowed mounted Styx session. */
    StyxRpcProgram program;     /* Copied reverse authority selected by the host. */
    uint32_t last_token;         /* Last allocated token, including closed tickets. */
    int bound;                  /* One-shot registration, even after close. */
    FVM_StyxRpcTicket tickets[FVM_STYXRPC_LAUNCH_SLOTS]; /* Owned named launch state. */
    char cwd[FVM_STYXRPC_ABSOLUTE_PATH]; /* Canonical absolute working path for ordinary VFS calls. */
} FVM_StyxRpcSession;


/* [GC:] EN: Create a fresh AS object for a zero-initialized host binding.
 * RU: Создать новый объект AS для обнулённой привязки хоста. */
FVM_API FVM_Result fvm_styxrpc_session_bind(FVM_Context *context, FVM_StyxRpcSession *binding,
    struct StyxClient *client, const StyxRpcProgram *program, FVM_Value *object);


/* EN: Disarm the binding and close its tickets while leaving client ownership with the host.
 * RU: Отключить привязку и закрыть запуски, сохранив владение client у хоста. */
FVM_API void fvm_styxrpc_session_close(FVM_StyxRpcSession *binding);

#ifdef __cplusplus
}
#endif
#endif
