/* Copyright (c) 2026 bhgv
 * EN: Native VFS provider backed by an already connected original-codec 9P client.
 * RU: Native VFS provider над подключённым original-codec 9P client.
 */
#ifndef STYX_MOUNT_H
#define STYX_MOUNT_H
#include "client.h"
#include "provider.h"


/* EN/RU: Consume owned client on every outcome; owned ordinary provider/NULL.
 * Fixed access ceiling uses FVM_ACCESS_*; no automatic dialing/authentication.
 * Provider refs/opens pin connection; factory access remains UNSUPPORTED because
 * standard9P has no side-effect-free effective-access RPC. Final operations decide.
 * Принять client всегда; свой provider/NULL, ceiling не даёт server permissions. */
DVM_Provider *styxmountprovider(StyxClient *client, unsigned access);
#endif
