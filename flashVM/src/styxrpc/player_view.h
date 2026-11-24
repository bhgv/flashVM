/* Copyright (c) 2026 bhgv
 * EN: Optional paired-player presentation on the VM and Styx owner thread.
 * RU: Необязательная отрисовка парного плеера в потоке владельца VM и Styx.
 */
#ifndef FLASHVM_STYXRPC_PLAYER_VIEW_H
#define FLASHVM_STYXRPC_PLAYER_VIEW_H
#include "flashvm/styxrpc_playback.h"

typedef struct FVM_PairedView FVM_PairedView; /* Owned native surface and optional SDL window. */

/* EN: Verify requested build features and allocate a view, or return NULL.
 * RU: Проверить функции сборки и выделить представление либо вернуть NULL. */
FVM_PairedView *fvm_paired_view_create(int windowed, const char *capture);

/* EN: Poll input and present at the owner deadline. Clamp wait_ms for pending GUI work.
 * Return 1 to continue, 0 on window close and -1 on failure. NULL is headless.
 * RU: Обработать ввод и отрисовать по сроку у владельца. Ограничить wait_ms для GUI.
 * Вернуть 1 для продолжения, 0 при закрытии и -1 при отказе. NULL означает режим без окна. */
int fvm_paired_view_step(FVM_PairedView *view, FVM_StyxRpcPlayback *playback, int *wait_ms);

/* EN: Write the last movie to the configured PPM destination, or return one if disabled.
 * RU: Записать последний фильм в заданный PPM либо вернуть один, если снимок выключен. */
int fvm_paired_view_capture(FVM_PairedView *view, FVM_StyxRpcPlayback *playback);

/* EN: Release the owned view on the player thread. NULL is harmless.
 * RU: Освободить своё представление в потоке плеера. NULL допустим. */
void fvm_paired_view_destroy(FVM_PairedView *view);

#endif
