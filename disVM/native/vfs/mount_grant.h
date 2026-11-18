/* Copyright (c) 2026 bhgv
 * EN: Native channel adapters clone the actual retained Styx grant root, never an ambient attach path.
 * RU: Native channel adapters клонируют настоящий удержанный Styx grant root, не ambient attach path.
 */
#ifndef STYX_MOUNT_GRANT_H
#define STYX_MOUNT_GRANT_H
#include "mount.h"
#ifdef __cplusplus
extern "C" {
#endif


/* EN: Borrow pinned grant/client, return owned unopened root fid/NULL under current native scope.
 * Require this SDK's Styx mount factory, a bindable fixed dot, exact client identity and access
 * within its immutable local ceiling. Directory roots additionally require EXEC/search ceiling.
 * Required access is nonzero FVM_ACCESS_*; it grants no remote permission. Clone the captured root
 * fid (or immutable attached-file root), without rewalking its former pathname or probing another root.
 * Native-only adapter boundary, not a generic provider-state or guest capability export.
 * RU: Borrow pinned grant/client, свой unopened root fid/NULL. Только Styx mount factory,
 * fixed dot, exact client и access внутри ceiling; directory требует EXEC/search. Клонировать
 * captured root/immutable attached file, без обхода старого pathname либо другого root.
 */
StyxFid *styxmountgrantfid(DVM_Provider *grant, StyxClient *client, unsigned access);
#ifdef __cplusplus
}
#endif
#endif
