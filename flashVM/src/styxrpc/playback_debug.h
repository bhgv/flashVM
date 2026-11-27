/* Copyright (c) 2026 bhgv
 * EN: Owner-local debugger state and the .state command/status adapter.
 * RU: Состояние отладчика у владельца и адаптер команд и состояния .state.
 */
#ifndef FVM_STYXRPC_PLAYBACK_DEBUG_H
#define FVM_STYXRPC_PLAYBACK_DEBUG_H
#include "flashvm/styxrpc_playback.h"

#define PLAYBACK_DEBUG_FUNCTION_MAX 127U /* Maximum command function-name bytes. */
#define PLAYBACK_DEBUG_SNAPSHOT_MAX 1536U /* Maximum copied location and stack text. */
#define PLAYBACK_DEBUG_FRAMES_MAX 4U /* Innermost frames included in .state. */

typedef struct PlaybackDebug
{
    StyxRpcServer *server;   /* Borrowed paired server; owner steps it at a stop. */
    int attached;            /* Current context has an installed callback. */
    int ticking;             /* Timeline owner can stop without recursive dispatch. */
    int stopped;             /* Live AVM1 activation is held inside the callback. */
    int resume;              /* Controller accepted continue or step for this stop. */
    unsigned long stop;      /* Last stop ID, retained across movie replacement. */
    size_t length;           /* Copied text bytes exposed while stopped. */
    char snapshot[PLAYBACK_DEBUG_SNAPSHOT_MAX]; /* Copied labels and stack, no VM references. */
} PlaybackDebug;


/* EN: Enable debugger commands for playback, borrowing server until playback retires.
 * RU: Включить команды отладчика у playback, заимствуя server до завершения. */
void fvm_styxrpc_debug_init(FVM_StyxRpcPlayback *playback, StyxRpcServer *server);


/* EN: Install the actual AVM1 callback on the paused owner; propagate VM errors.
 * An embedding host without a registered server keeps its own debug hook.
 * RU: Установить колбэк AVM1 на паузе у владельца; вернуть ошибки VM.
 * Встраивающий хост без сервера сохраняет собственный обработчик. */
FVM_Result fvm_styxrpc_debug_attach(FVM_StyxRpcPlayback *playback);


/* EN: Append native debugger data to the current .state sample, or zero if absent.
 * RU: Дописать данные отладчика в снимок .state или вернуть ноль при отсутствии. */
size_t fvm_styxrpc_debug_sample(const PlaybackDebug *debug, char *bytes, size_t capacity);
#endif
