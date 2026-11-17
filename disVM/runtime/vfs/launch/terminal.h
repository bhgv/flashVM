/* Copyright (c) 2026 bhgv
 * EN: Pinned FlashVM channel namespace, independent of local/remote placement and union order.
 * RU: Закреплённое пространство каналов FlashVM независимо от размещения и порядка union.
 */
#ifndef DVM_LAUNCH_TERMINAL_H
#define DVM_LAUNCH_TERMINAL_H
#include "prepare.h"
#include "movie_offer.h"


/* EN/RU: Borrowed descriptor at submission, owned provider pin inside a handoff; never serialize.
 * Receive/send are relative channel names, identical for one duplex node or distinct for a pair.
 * Descriptor borrowed при submit, handoff владеет provider pin; не сериализовать.
 * Receive/send — относительные имена одного duplex node либо пары каналов. */
typedef struct DVM_TerminalChannels
{
    DVM_Provider *provider; /* Granted anchored live channel root; no union/native fallback. */
    char receive[DVM_NS_PATH_LIMIT]; /* Provider-to-Dis direction, canonical relative path. */
    char send[DVM_NS_PATH_LIMIT]; /* Dis-to-provider direction, canonical relative path. */
    DVM_MovieOffer *offers; /* Optional trusted per-job announcement source, retained with the descriptor. */
} DVM_TerminalChannels;


/* EN/RU: Validate two borrowed canonical channel paths bounded by DVM_NS_PATH_LIMIT;1/0.
 * No provider/transport I/O, no rewriting of absolute/dotdot/empty components.
 * Проверить два borrowed canonical path с bound DVM_NS_PATH_LIMIT;1/0, без I/O/rewrite. */
int dvm_terminal_channel_paths_valid(const char *receive, const char *send);


/* EN/RU: Validate borrowed descriptor without callbacks/I/O;1 valid/0 invalid.
 * Require anchored dot and PATH cancellation; strings grant no additional rights.
 * Проверить borrowed descriptor без callbacks/I/O;1/0, anchored dot/PATH обязательны. */
int dvm_terminal_channels_valid(const DVM_TerminalChannels *channels);


/* EN/RU: Open a named channel on the pinned root into empty owned out; native-only, may block.
 * Borrow descriptor/path/mode through return; canonical relative path, mode r/w/r+/rw only.
 * Provider receives r/wo/rw respectively: existing streams, no create/truncate.
 * Control and dynamically added bulk/stream nodes use the same operation; no framing/retry.
 * Current cancellation reaches opted-in provider; pre-cancel refuses before I/O.
 * Открыть канал в empty owned out; native blocking, borrowed inputs, mode r/w/r+/rw.
 * Control/bulk/stream используют один API; без framing/retry, pre-cancel до I/O. */
FVM_Result dvm_terminal_channel_open(const DVM_TerminalChannels *channels, const char *path,
    const char *mode, DVM_VfsHandle *out);
#endif
