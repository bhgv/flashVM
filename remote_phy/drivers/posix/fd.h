/* Copyright (c) 2026 bhgv
 * EN: Explicit adoption of an already granted nonblocking POSIX carrier descriptor.
 * RU: Явное принятие уже выданного nonblocking POSIX carrier descriptor.
 */
#ifndef REMOTE_PHY_POSIX_FD_H
#define REMOTE_PHY_POSIX_FD_H
#include "../../phy.h"
#ifdef __cplusplus
extern "C" {
#endif


/* EN/RU: Consume fd on every outcome; owned physical adapter/NULL, no open/dial/authentication.
 * Caller explicitly asserts carrier capabilities; fd alone does not prove reliable framing.
 * Supports O_RDWR sockets and byte devices; configures NONBLOCK/CLOEXEC, preserves other flags.
 * Host configures serial/bus details before adoption; wake uses a private native pipe.
 * Принять fd всегда; свой adapter/NULL, без open/dial/auth. Caller явно задаёт capabilities;
 * O_RDWR socket/byte device, NONBLOCK/CLOEXEC; serial/bus configuration делает host до adoption. */
RemotePhy *phyposixfd(int fd, unsigned capabilities);
#ifdef __cplusplus
}
#endif
#endif
