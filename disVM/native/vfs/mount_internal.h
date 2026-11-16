/* Copyright (c) 2026 bhgv
 * EN: Private VFS-side mount ownership, opened cursors and event routing.
 * RU: Private VFS-side mount ownership, открытые cursors и event routing.
 */
#ifndef STYX_MOUNT_INTERNAL_H
#define STYX_MOUNT_INTERNAL_H
#include "mount.h"
#include "channel_sync.h"

typedef struct StyxRootLife StyxRootLife; /* Owned detached native root-fid cleanup context. */


/* EN/RU: Provider factory lifetime pins one connection and its native event source.
 * Lifetime provider factory удерживает connection и native event source. */
struct Mount
{
    StyxRootLife *rootlife; /* Owned nonwaiting cleanup authority; worker owns captured root fid. */
    StyxFid *root; /* Borrowed immutable unopened root under rootlife; NULL for attach fid1. */
    StyxClient *client; /* Owned connection; provider wrappers pin this factory. */
    StyxMountCtl *control; /* Owned subscription authority, permanently closed on factory retirement. */
    StyxWatch *watch; /* Owned native terminal watch; its context pins only the journal. */
    int file_root; /* Immutable attached file root forbids every descendant, even with a misbehaving peer. */
    unsigned access; /* Fixed native host ceiling. */
};


/* EN/RU: Provider's open wrapper pins factory while this cursor owns its remote fid.
 * Open wrapper удерживает factory, а cursor владеет remote fid. */
struct Mounted
{
    struct Mount *mount; /* Borrowed factory, pinned by provider's open wrapper. */
    StyxFid *fid; /* Owned independent remote open. */
    unsigned long long position; /* Confirmed byte cursor, never guessed on failure. */
    int directory; /* Directory list uses raw stat records, not byte read callbacks. */
    int failed; /* Directory cursor aborted; cannot silently become EOF. */
    unsigned char *records; /* Owned directory read frame, or NULL for files. */
    size_t size; /* Confirmed buffered directory bytes. */
    size_t used; /* Already published complete record bytes. */
};


/* EN/RU: Subscribe native connection health with an independent source cursor and retained journal.
 * Подписаться на native connection health с независимым cursor и retained journal. */
StyxWatch *styxmountwatch(StyxClient *client, DVM_Changes *journal);


/* EN/RU: Copy scalar metadata, mask the host ceiling; never infer effective access.
 * Скопировать scalar metadata с host ceiling, без вывода effective access. */
void styxmountmeta(struct Mount *mount, const StyxMeta *meta, FVM_FileInfo *out);


/* EN/RU: Independent opened cursor callbacks, used only through provider lifetime pins.
 * Callbacks открытого cursor только под provider lifetime pins. */
extern const DVM_VfsOps styxmountedops;


/* EN/RU: Walk relative to this mount's captured root, never a retired original path.
 * Пройти относительно captured root этого mount, не по прежнему pathname. */
StyxFid *styxmountwalk(struct Mount *mount, const char *relative);


/* EN/RU: Capture callback returns fresh independently rooted provider, preserving fixed access.
 * Capture callback возвращает fresh independently rooted provider с прежним access ceiling. */
DVM_Provider *styxmountcapture(void *state, const char *relative);


/* EN/RU: Consume root/client owners always; construct fresh provider with optional own health watch.
 * Поглотить root/client owners всегда; fresh provider с optional own health watch. */
DVM_Provider *styxmountroot(StyxClient *client, StyxFid *root, unsigned access, int directory, int watch);


/* EN/RU: Adopt root on success only; detached native worker owns cleanup, owned control/NULL.
 * Принять root только при успехе; detached native worker владеет cleanup, свой control/NULL. */
StyxRootLife *styxmountrootlife(StyxFid *root);


/* EN/RU: Consume cleanup control nonwaiting; native worker clunks fid and frees its own context.
 * Поглотить cleanup control без ожидания; native worker clunk fid и освобождает context. */
void styxmountrootretire(StyxRootLife *life);
#endif
