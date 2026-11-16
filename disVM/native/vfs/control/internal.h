/* Copyright (c) 2026 bhgv
 * EN: Private live mount control ownership and serialized native preparation.
 * RU: Private владение live mount control и сериализация native preparation.
 */
#ifndef STYX_MOUNT_CONTROL_INTERNAL_H
#define STYX_MOUNT_CONTROL_INTERNAL_H
#include "../mount.h"
#include "../feed/internal.h"


/* EN/RU: Mount and explicit native owners pin control, never the provider itself.
 * Mount и явные native owners удерживают control, но не сам provider. */
struct StyxMountCtl
{
    pthread_mutex_t lock; /* Protect revisions, ownership and current source. */
    size_t references; /* Mount plus explicit native capabilities. */
    StyxMountInfo info; /* Copy-only status. */
    unsigned access; /* Immutable mount access ceiling. */
    StyxClient *client; /* Independent connection pin, dropped on provider close. */
    DVM_Changes *journal; /* Independent journal pin, dropped on provider close. */
    StyxFeed *retiring; /* Owned detached predecessor until replacement/close; preserves RPC quota fence. */
    StyxFeed *feed; /* Current owned source or NULL. */
};


/* EN/RU: Borrow client/journal and retain independent pins; owned result/NULL, no I/O.
 * Borrow client/journal и удержать свои pins; свой result/NULL, без I/O. */
StyxMountCtl *styxmountctlcreate(StyxClient *, DVM_Changes *, unsigned access);


/* EN/RU: Permanently close control on provider retirement, detach source and drop pins without join.
 * Навсегда закрыть control при provider retirement, detach source/drop pins без join. */
void styxmountctlclose(StyxMountCtl *control);
#endif
