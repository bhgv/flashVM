/* Copyright (c) 2026 bhgv
 * EN: Native VFS provider backed by an already connected original-codec 9P client.
 * RU: Native VFS provider над подключённым original-codec 9P client.
 */
#ifndef STYX_MOUNT_H
#define STYX_MOUNT_H
#include "client.h"
#include "provider.h"
#include "control.h"


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


/* EN/RU: Consume client always; owned provider/NULL, required output receives native mount authority.
 * Optional initial policy adopts state only on success. Ordinary fids survive later feed changes.
 * Принять client всегда; свой provider/NULL и обязательный native control; initial policy optional. */
DVM_Provider *styxmountprovidercontrol(StyxClient *client, unsigned access,
                                      const StyxFeedConfig *initial, StyxMountCtl **output);


/* EN/RU: Native I/O captures one relative root fid before publication; consume client always.
 * Return owned provider/NULL; later opens/walks stay relative to that fid after rename/rebind.
 * No prepared tree snapshot, implicit reconnect or guessed event endpoint is introduced.
 * Native I/O захватывает relative root fid; client принят всегда, свой provider/NULL. */
DVM_Provider *styxmountproviderat(StyxClient *client, const char *relative, unsigned access);

/* EN: Bind one advertised file to a trusted local launch resolver. The resolver
 * returns an independently owned address on demand. It may not use managed
 * state. The mount consumes client and resolver state on every outcome; release
 * may be NULL and otherwise must be nonblocking. Captures retain the association.
 * The remote node is pinned and compared by native identity before each resolve.
 * RU: Связать объявленный файл с доверенным native resolver. Он выдаёт отдельный
 * owned address. Client и state потребляются при любом исходе. Release может
 * быть NULL, иначе не блокирует. Capture сохраняет привязку. Удалённый объект
 * удерживается и сверяется по identity перед каждым разрешением. */
DVM_Provider *styxmountproviderlaunch(StyxClient *client, const char *relative_root,
    unsigned access, const char *advertised_file, struct DVM_LaunchAddress *(*resolve)(void *),
    void *state, void (*release)(void *));

typedef struct StyxLaunchOffer
{
    const char *path; /* Borrowed path relative to the mount root. */
    struct DVM_LaunchAddress *(*resolve)(void *); /* Native address factory. */
    void *state; /* Passed to resolve and release. */
    void (*release)(void *); /* Optional nonblocking destructor. */
} StyxLaunchOffer;

/* EN: Pin distinct advertised files on one client. Consume the client and every
 * offer state, including on refusal. Borrow the array and paths during setup.
 * RU: Закрепить разные файлы на одном client. Принять client и state каждой
 * записи даже при отказе. Массив и пути заимствуются на время настройки. */
DVM_Provider *styxmountproviderlaunches(StyxClient *client, const char *root, unsigned access,
    const StyxLaunchOffer *offers, size_t count);
#endif
