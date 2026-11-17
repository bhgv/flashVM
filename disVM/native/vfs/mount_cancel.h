/* Copyright (c) 2026 bhgv
 * EN: Private source-scope to single-use native Styx cancellation binding.
 * RU: Private binding source scope в single-use native Styx cancellation.
 */
#ifndef STYX_MOUNT_CANCEL_H
#define STYX_MOUNT_CANCEL_H
#include "client.h"
#include "cancel.h"


/* EN/RU: One synchronous RPC owns a tap; its native token outlives admitted signals.
 * Один synchronous RPC владеет tap; native token переживает admitted signals. */
typedef struct StyxMountCancel
{
    DVM_CancelTap *tap; /* Owned native source subscription, NULL without active scope. */
    StyxCancel *token; /* Borrowed from tap-owned callback context through RPC completion. */
} StyxMountCancel;


/* EN/RU: Initialize output;1 success/no scope,0 setup refusal before RPC effects.
 * Borrow client/current scope; callbacks retain only native token, no provider/guest.
 * Инициализировать output;1 успех/no scope,0 отказ до RPC effects; только native token. */
int styxmountcancel(StyxClient *client, StyxMountCancel *output);


/* EN/RU: Consume binding after RPC completes, without waiting for admitted callbacks.
 * Поглотить binding после RPC без ожидания admitted callbacks. */
void styxmountcancelend(StyxMountCancel *binding);


/* EN/RU: Diagnostic stat respects current cancellation; failed output stays untouched.
 * Diagnostic stat учитывает current cancellation; output при отказе не меняется. */
int styxmountstat(StyxClient *client, StyxFid *fid, StyxMeta *output);

/* EN/RU: Walk one pinned root-relative path under current native scope; owned fid/NULL.
 * Пройти root-relative path под current native scope; свой fid/NULL, base optional/unopened. */
StyxFid *styxmountwalkcancel(StyxClient *client, StyxFid *base, const char *path);


/* EN/RU: Open borrowed serialized fid; original acknowledgement1/0, no implicit retry.
 * Открыть borrowed serialized fid; original acknowledgement1/0, без retry. */
int styxmountopen(StyxClient *client, StyxFid *fid, int mode);


/* EN/RU: Consume owned fid under scope; setup refusal uses independent native cleanup.
 * Поглотить fid под scope; setup refusal использует independent native cleanup. */
void styxmountclunk(StyxClient *client, StyxFid *fid);


/* EN/RU: Scoped lookup with per-operation absence output, owned fid/NULL; missing required.
 * Scoped lookup с локальным missing, свой fid/NULL, без общего last-error state. */
StyxFid *styxmountlookup(StyxClient *client, StyxFid *base, const char *path, int *missing);
#endif
