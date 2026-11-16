/* Copyright (c) 2026 bhgv
 * EN: Versioned native authority to replace a live mount's event subscription.
 * RU: Версионированное native право замены подписки событий работающего mount.
 */
#ifndef STYX_MOUNT_CONTROL_H
#define STYX_MOUNT_CONTROL_H
#include "feed.h"
typedef struct StyxMountCtl StyxMountCtl; /* Owned native authority; caller pins one reference across every call. */


/* EN/RU: Copy-only status; version starts at1, successful changes increment without wrap.
 * Копируемый status; версия начинается с1, успешные изменения увеличивают её без wrap. */
typedef struct StyxMountInfo
{
    unsigned long long version; /* Compare-and-swap revision of configured subscription. */
    int closed; /* Provider retired permanently; no connection retained by this capability. */
    int changing; /* One native preparation in progress; another set is refused. */
    int configured; /* Current feed exists, including EOF/failed feeds. */
} StyxMountInfo;


/* EN/RU: Retain a borrowed live capability;1/0, no I/O or callbacks.
 * Удержать borrowed capability;1/0, без I/O/callbacks. */
int styxmountctlretain(StyxMountCtl *control);


/* EN/RU: Consume owner reference; mount keeps its independent owner until retirement.
 * Поглотить owner reference; mount сохраняет своего owner до retirement. */
void styxmountctlfree(StyxMountCtl *control);


/* EN/RU: Copy status under native mutex;1/0, invalid leaves output unchanged.
 * Копировать status под native mutex;1/0, invalid не меняет output. */
int styxmountctlinfo(StyxMountCtl *control, StyxMountInfo *output);


/* EN/RU: Return owned current feed/NULL and optionally copied version, atomically; no I/O.
 * Old feed capabilities retain their identity. Caller pins control during call.
 * Вернуть свой current feed/NULL и optional version атомарно; старые capabilities не переназначаются. */
StyxFeed *styxmountctlfeed(StyxMountCtl *control, unsigned long long *version);


/* EN/RU: Native I/O prepares endpoint, then commits iff expected version still matches.
 * Return new version/0; config.state adopted only on success; optional output cleared first.
 * Preparation can block on RPC; commit never waits for old read/decoder. Explicit clear/close
 * may invalidate preparation. No callback under control lock. No guest invocation.
 * Native I/O готовит endpoint, commit по expected; new version/0, state принят только при успехе. */
unsigned long long styxmountctlset(StyxMountCtl *control, unsigned long long expected,
                                 const StyxFeedConfig *config, StyxFeed **output);


/* EN/RU: Nonwaiting cancellation/CAS detach, new version/0; can invalidate concurrent preparation.
 * Nonwaiting cancel/CAS detach, new version/0; может отменить commit concurrent preparation. */
unsigned long long styxmountctlclear(StyxMountCtl *control, unsigned long long expected);
#endif
