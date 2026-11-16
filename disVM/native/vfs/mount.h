/* Copyright (c) 2026 bhgv
 * EN: Native VFS provider backed by an already connected original-codec 9P client.
 * RU: Native VFS provider над подключённым original-codec 9P client.
 */
#ifndef STYX_MOUNT_H
#define STYX_MOUNT_H
#include "client.h"
#include "provider.h"
#include "feed.h"


/* EN/RU: Consume owned client on every outcome; owned ordinary provider/NULL.
 * Fixed access ceiling uses FVM_ACCESS_*; no automatic dialing/authentication.
 * Provider refs/opens pin connection; factory access remains UNSUPPORTED because
 * standard9P has no side-effect-free effective-access RPC. Final operations decide.
 * Принять client всегда; свой provider/NULL, ceiling не даёт server permissions. */
DVM_Provider *styxmountprovider(StyxClient *client, unsigned access);


/* EN/RU: Native I/O constructor; consumes client always, adopts config.state only on success.
 * Explicit relative endpoint/decoder, fd-owner with at least two RPC slots required.
 * Optional control receives its own feed ref; provider owns another and stops it on destroy.
 * No path probing, guest loop or wire-format default. Failure clears control.
 * Native I/O constructor: client принять всегда, config.state только при успехе; control свой ref. */
DVM_Provider *styxmountproviderfeed(StyxClient *client, unsigned access, const StyxFeedConfig *config, StyxFeed **control);
#endif
