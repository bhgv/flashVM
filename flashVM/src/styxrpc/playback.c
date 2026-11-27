/* Copyright (c) 2026 bhgv
 * EN: Stage a complete SWF and advance its real timeline from owner control files.
 * RU: Подготовка полного SWF и управление его шкалой из файлов владельца.
 */
#include "flashvm/styxrpc_playback.h"
#include "movie_load.h"
#include "playback_clock.h"
#include "../standard/movieclip_constants.h"
#include "flashvm/swf.h"
#include "flashvm/gc.h"
#include "flashvm/movieclip.h"
#include "flashvm/async.h"
#include "../standard/async_worker.h"
#include "worker_vfs.h"
#include "flashvm/standard.h"
#include "flashvm/styxrpc_filesystem.h"
#include "playback_internal.h"
#include <stdlib.h>
#include <string.h>

#define PLAYBACK_FIRST_TICK 1U /* Start/restart enter the first available timeline tick. */
#define PLAYBACK_FIXED8_RATE_UNIT 256.0 /* SWF header frame-rate denominator. */
#define PLAYBACK_FALLBACK_FPS 12.0 /* A zero header rate uses twelve frames per second. */
#define PLAYBACK_SECONDS_MS 1000.0 /* Milliseconds per second. */

/* [GC:] EN: Drop deferred roots before the VM and free the retained callback storage.
 * RU: Снять отложенные корни до освобождения VM, затем освободить колбэки. */
static void dispose_movie(FVM_Context *context, FVM_StyxRpcMovies *movies, FVM_StyxRpcPlayerServices *script)
{
    fvm_styxrpc_movies_finish(movies, 0);
    fvm_styxrpc_player_disarm(script);
    fvm_context_destroy(context);
    free(script);
    free(movies);
}


/* [GC:] EN: Prepare a separate context before changing the active movie.
 * RU: Подготовить отдельный контекст до изменения текущего фильма. */
static FVM_Result prepare(FVM_StyxRpcPlayback *playback, const void *bytes, size_t size)
{
    FVM_Context *context = NULL;
    FVM_Object *root = NULL;
    FVM_StyxRpcMovies *movies = NULL;
    FVM_StyxRpcPlayerServices *script = NULL;
    FVM_SWFInfo info;
    FVM_Result status = fvm_context_create(playback->spec.runtime, &context);
    if (status == FVM_OK)
    {
        status = fvm_load_memory(context, bytes, size);
    }
    if (status == FVM_OK)
    {
        info.struct_size = sizeof(info);
        status = fvm_swf_info(context, &info);
    }
    if (status == FVM_OK)
    {
        status = fvm_standard_install(context);
    }
    if (status == FVM_OK)
    {
        status = fvm_async_install(context);
    }
    if (status == FVM_OK && playback->spec.session)
    {
        status = fvm_styxrpc_filesystem_mount(context, "/", playback->spec.session);
        if (status == FVM_OK)
        {
            status = fvm_filesystem_install(context, "fs");
        }
        if (status == FVM_OK)
        {
            status = fvm_styxrpc_player_services(context, playback->spec.session, playback->slot, &script);
        }
        if (status == FVM_OK && playback->message_server)
        {
            status = fvm_styxrpc_player_enable_messages(context, script, playback->message_server);
        }
    }
    if (status == FVM_OK)
    {
        status = fvm_execution_set_mode(context, playback->spec.mode, 0);
    }
    if (status == FVM_OK && playback->spec.setup)
    {
        status = playback->spec.setup(playback->spec.host, context);
    }
    if (status == FVM_OK)
    {
        status = fvm_movieclip_load_swf(context, bytes, size, "_root", &root);
    }
    if (status == FVM_OK)
    {
        status = fvm_gc_pin(context, root);
    }
    if (status == FVM_OK && playback->spec.session)
    {
        movies = calloc(1, sizeof(*movies));
        if (!movies)
        {
            status = FVM_OUT_OF_MEMORY;
        }
        else
        {
            movies->context = context;
            movies->root = root;
            movies->session = playback->spec.session;
            movies->named = playback->named;
            movies->refs = playback->refs;
            movies->binding = fvm_styxrpc_player_binding(script);
            if (playback->movie_scope && playback->message_server)
            {
                movies->caller_pid = playback->movie_scope->pid;
                movies->argument_bus = playback->movie_scope->bus;
                movies->reverse = playback->message_server;
            }
            movies->limit = playback->spec.upload_limit;
            if (movies->limit > FVM_SWF_DEFAULT_LIMIT)
            {
                movies->limit = FVM_SWF_DEFAULT_LIMIT;
            }
            status = fvm_styxrpc_movies_bind(movies);
            if (status == FVM_OK)
            {
                status = fvm_styxrpc_os_install(movies);
            }
            if (status == FVM_OK)
            {
                FVM_AsyncWorkerSetup setup;
                setup.install = fvm_styxrpc_worker_vfs;
                setup.state = playback;
                fvm_async_worker_setup(context, &setup);
            }
        }
    }
    if (status == FVM_OK)
    {
        status = fvm_styxrpc_player_activate(playback->slot, script);
    }
    if (status != FVM_OK)
    {
        dispose_movie(context, movies, script);
        return status;
    }

    dispose_movie(playback->context, playback->movies, playback->script);
    playback->movies = movies;
    playback->script = script;
    playback->context = context;
    playback->root = root;
    playback->debug.attached = 0;
    playback->period = PLAYBACK_SECONDS_MS / (info.frame_rate_fixed8 ?
        (double)info.frame_rate_fixed8 / PLAYBACK_FIXED8_RATE_UNIT : PLAYBACK_FALLBACK_FPS);
    playback->ticks = 0;
    playback->state = PLAYBACK_READY;
    return FVM_OK;
}


/* [ASINC] EN: Consume the complete SWF and replace the current movie on success.
 * RU: Принять полный SWF и заменить текущий фильм при успехе. */
static int adopt_movie(FVM_StyxRpcPlayback *playback, unsigned char *bytes, size_t size)
{
    playback->last = prepare(playback, bytes, size);
    if (playback->last != FVM_OK)
    {
        free(bytes);
        return 0;
    }
    free(playback->swf);
    playback->swf = bytes;
    playback->size = size;
    return 1;
}


/* [ASINC] EN: Copy the borrowed completed upload on the playback owner.
 * RU: Скопировать заимствованную завершённую загрузку у владельца плеера. */
static int upload(void *data, const void *bytes, size_t size)
{
    FVM_StyxRpcPlayback *playback = data;
    unsigned char *copy;
    if (playback->awaiting_session)
    {
        return 0;
    }
    copy = malloc(size);
    if (!copy)
    {
        playback->last = FVM_OUT_OF_MEMORY;
        return 0;
    }
    memcpy(copy, bytes, size);
    return adopt_movie(playback, copy, size);
}


/* EN/RU: See playback_internal.h. / См. playback_internal.h. */
int fvm_styxrpc_playback_start_local(FVM_StyxRpcPlayback *playback, unsigned char *bytes, size_t size)
{
    if (!bytes || !size || size > playback->spec.upload_limit || playback->awaiting_session)
    {
        free(bytes);
        return 0;
    }
    if (!adopt_movie(playback, bytes, size))
    {
        return 0;
    }
    /* EN: The clock starts AS after ordinary wire dispatch is enabled.
     * RU: Часы запускают AS после начала обычной обработки соединения. */
    playback->state = PLAYBACK_RUNNING;
    ++playback->schedule;
    return 1;
}


/* [ASINC] EN/RU: See flashvm/styxrpc_playback.h. / См. flashvm/styxrpc_playback.h. */
FVM_Result fvm_styxrpc_playback_tick(FVM_StyxRpcPlayback *playback, unsigned int ticks)
{
    if (!playback)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (playback->state != PLAYBACK_RUNNING || !ticks)
    {
        return FVM_OK;
    }
    playback->debug.ticking = 1;
    if (!playback->movies || ticks > FVM_MOVIECLIP_MAX_TICKS_PER_CALL)
    {
        playback->last = fvm_movieclip_advance(playback->context, playback->root, ticks);
        if (playback->last == FVM_OK)
        {
            playback->ticks += ticks;
        }
    }
    else
    {
        unsigned int index;
        for (index = 0; index < ticks; ++index)
        {
            playback->last = fvm_movieclip_advance(playback->context, playback->root, PLAYBACK_FIRST_TICK);
            fvm_styxrpc_movies_finish(playback->movies, playback->last == FVM_OK);
            if (playback->last != FVM_OK)
            {
                break;
            }
            ++playback->ticks;
        }
    }
    if (playback->last != FVM_OK)
    {
        playback->state = PLAYBACK_FAILED;
    }
    playback->debug.ticking = 0;
    return playback->last;
}


/* [ASINC] EN: Apply a recognized command between owner activations.
 * RU: Применить распознанную команду между активациями владельца. */
static int command(void *data, StyxRpcControlCommand what)
{
    FVM_StyxRpcPlayback *playback = data;
    if (!playback->context || playback->debug.stopped)
    {
        return 0;
    }
    switch (what)
    {
    case STYXRPC_CONTROL_PAUSE:
        if (playback->state == PLAYBACK_FAILED)
        {
            return 0;
        }
        playback->state = PLAYBACK_PAUSED;
        return 1;
    case STYXRPC_CONTROL_DEBUG:
        playback->last = fvm_execution_set_mode(playback->context, FVM_EXECUTION_INTERPRETER, 0);
        if (playback->last != FVM_OK)
        {
            return 0;
        }
        playback->state = PLAYBACK_PAUSED;
        if (playback->spec.debug)
        {
            playback->last = playback->spec.debug(playback->spec.host, playback->context);
            if (playback->last != FVM_OK)
            {
                return 0;
            }
        }
        playback->last = fvm_styxrpc_debug_attach(playback);
        if (playback->last != FVM_OK)
        {
            return 0;
        }
        playback->state = PLAYBACK_DEBUG;
        return 1;
    case STYXRPC_CONTROL_RESTART:
        playback->last = prepare(playback, playback->swf, playback->size);
        if (playback->last != FVM_OK)
        {
            return 0;
        }
        break;
    case STYXRPC_CONTROL_START:
        if (playback->state == PLAYBACK_RUNNING)
        {
            return 1;
        }
        if (playback->state == PLAYBACK_FAILED)
        {
            return 0;
        }
        break;
    default:
        return 0;
    }
    playback->state = PLAYBACK_RUNNING;
    ++playback->schedule;
    return playback->defer_command_tick || fvm_styxrpc_playback_tick(playback, PLAYBACK_FIRST_TICK) == FVM_OK;
}


/* EN/RU: See playback_clock.h. / См. playback_clock.h. */
int fvm_styxrpc_playback_clock(FVM_StyxRpcPlayback *playback, double *period, unsigned long *schedule)
{
    if (playback->state != PLAYBACK_RUNNING)
    {
        return 0;
    }
    *period = playback->period;
    *schedule = playback->schedule;
    return 1;
}


/* EN/RU: See flashvm/styxrpc_playback.h. / См. flashvm/styxrpc_playback.h. */
FVM_Result fvm_styxrpc_playback_create(const FVM_StyxRpcPlaybackSpec *spec, FVM_StyxRpcPlayback **out)
{
    FVM_StyxRpcPlayback *playback;
    StyxRpcControlSpec control;
    if (!spec || !spec->runtime || !spec->upload_limit || !out ||
        (spec->mode != FVM_EXECUTION_MANUAL && spec->mode != FVM_EXECUTION_INTERPRETER &&
         spec->mode != FVM_EXECUTION_JIT))
    {
        return FVM_INVALID_ARGUMENT;
    }
    playback = calloc(1, sizeof(*playback));
    if (!playback)
    {
        return FVM_OUT_OF_MEMORY;
    }
    playback->spec = *spec;
    control.context = playback;
    control.upload_limit = spec->upload_limit;
    control.upload = upload;
    control.command = command;
    control.status = fvm_styxrpc_playback_snapshot;
    if (styxrpc_control_create(&control, &playback->control) != STYXRPC_OK)
    {
        free(playback);
        return FVM_INVALID_ARGUMENT;
    }
    playback->slot = styxrpc_module_slot_create(playback->control);
    if (!playback->slot)
    {
        styxrpc_control_destroy(playback->control);
        free(playback);
        return FVM_OUT_OF_MEMORY;
    }
    *out = playback;
    return FVM_OK;
}


/* EN/RU: See flashvm/styxrpc_playback.h. / См. flashvm/styxrpc_playback.h. */
StyxRpcControl *fvm_styxrpc_playback_control(FVM_StyxRpcPlayback *playback)
{
    return playback ? playback->control : NULL;
}


/* EN/RU: See flashvm/styxrpc_playback.h. / См. flashvm/styxrpc_playback.h. */
StyxRpcModuleSlot *fvm_styxrpc_playback_slot(FVM_StyxRpcPlayback *playback)
{
    return playback ? playback->slot : NULL;
}


/* EN/RU: See playback_clock.h. / См. playback_clock.h. */
int fvm_styxrpc_playback_scene(FVM_StyxRpcPlayback *playback, FVM_Context **context, FVM_Object **root)
{
    if (!playback || !playback->root || !context || !root)
    {
        return 0;
    }
    *context = playback->context;
    *root = playback->root;
    return 1;
}


/* EN/RU: See playback_internal.h. / См. playback_internal.h. */
void fvm_styxrpc_playback_retire_movie(FVM_StyxRpcPlayback *playback)
{
    if (playback && playback->context)
    {
        fvm_styxrpc_player_activate(playback->slot, NULL);
        dispose_movie(playback->context, playback->movies, playback->script);
        playback->context = NULL;
        playback->root = NULL;
        playback->movies = NULL;
        playback->script = NULL;
    }
}


/* EN/RU: See flashvm/styxrpc_playback.h. / См. flashvm/styxrpc_playback.h. */
void fvm_styxrpc_playback_destroy(FVM_StyxRpcPlayback *playback)
{
    if (playback)
    {
        fvm_styxrpc_playback_retire_movie(playback);
        styxrpc_module_slot_destroy(playback->slot);
        styxrpc_control_destroy(playback->control);
        free(playback->swf);
        free(playback);
    }
}
