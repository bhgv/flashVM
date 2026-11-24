/* Copyright (c) 2026 bhgv
 * EN: Owner-only playback timing for the native Styx player.
 * RU: Время исполнения плеера Styx в потоке владельца.
 */
#ifndef FVM_STYXRPC_PLAYBACK_CLOCK_H
#define FVM_STYXRPC_PLAYBACK_CLOCK_H
#include "flashvm/styxrpc_playback.h"


/* EN: Return 1 and copy the running period in milliseconds and deadline revision.
 * Return 0 without writing outputs while idle or paused. Call between dispatches.
 * RU: Вернуть 1 и записать период в мс и ревизию срока при исполнении.
 * В паузе и без фильма вернуть 0 без записи. Вызывать между запросами. */
int fvm_styxrpc_playback_clock(FVM_StyxRpcPlayback *playback, double *period, unsigned long *schedule);
#endif
