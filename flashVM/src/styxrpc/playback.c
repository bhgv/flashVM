/* Copyright (c) 2026 bhgv
 * EN: Stage a complete SWF and advance its real timeline from owner control files.
 * RU: Подготовка полного SWF и управление его шкалой из файлов владельца.
 */
#define _POSIX_C_SOURCE 200809L /* Expose optional POSIX owner CPU and monotonic clocks. */
#include "flashvm/styxrpc_playback.h"
#include "movie_load.h"
#include "playback_clock.h"
#include "../standard/movieclip_constants.h"
#include "flashvm/swf.h"
#include "flashvm/gc.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/styxrpc_filesystem.h"
#include "session_methods.h"
#include "../../ports/styxrpc_stats_port.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PLAYBACK_FIRST_TICK 1U /* Start/restart enter the first available timeline tick. */
#define PLAYBACK_FIXED8_RATE_UNIT 256.0 /* SWF header frame-rate denominator. */
#define PLAYBACK_FALLBACK_FPS 12.0 /* A zero header rate uses twelve frames per second. */
#define PLAYBACK_SECONDS_MS 1000.0 /* Milliseconds per second. */

typedef enum PlaybackState
{
    PLAYBACK_EMPTY,   /* No upload has passed validation. */
    PLAYBACK_READY,   /* Prepared movie at its initial timeline position. */
    PLAYBACK_RUNNING, /* Subsequent owner ticks advance the movie. */
    PLAYBACK_PAUSED,  /* Owner ticks leave the timeline unchanged. */
    PLAYBACK_DEBUG,   /* Paused with the interpreter and optional debugger configured. */
    PLAYBACK_FAILED   /* A timeline operation failed; restart can rebuild the image. */
} PlaybackState;

struct FVM_StyxRpcPlayback
{
    FVM_StyxRpcPlaybackSpec spec; /* Copied settings and borrowed host services. */
    StyxRpcControl *control;     /* Owns .upload and .state node callbacks. */
    FVM_Context *context;         /* Current movie context, NULL until a successful upload. */
    FVM_StyxRpcSession *script;  /* Owned per-movie AS session and launch tickets. */
    FVM_StyxRpcMovies *movies;   /* Owned deferred SWF loader for the current paired context. */
    FVM_Object *root;             /* [GC:] Root clip pinned in the current context. */
    unsigned char *swf;           /* Owned copy used by restart. */
    size_t size;                  /* Length of the retained SWF, in bytes. */
    PlaybackState state;         /* State reported to the controller. */
    FVM_Result last;              /* Last timeline or control operation status. */
    double period;                /* Milliseconds per frame from the uploaded SWF. */
    unsigned long schedule;        /* Changes when start establishes a new frame deadline. */
    unsigned long ticks;          /* Successfully advanced ticks for the current movie. */
    double last_wall;             /* Last available monotonic sample in seconds. */
    double last_cpu;              /* Last available owner CPU sample in seconds. */
    FVM_StyxRpcProcessStats process; /* Previous kernel sample for matching CPU deltas by thread identity. */
};


/* [GC:] EN: Drop deferred roots before the VM and free the retained callback storage.
 * RU: Снять отложенные корни до освобождения VM, затем освободить колбэки. */
static void dispose_movie(FVM_Context *context, FVM_StyxRpcMovies *movies, FVM_StyxRpcSession *script)
{
    fvm_styxrpc_movies_finish(movies, 0);
    fvm_styxrpc_session_close(script);
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
    FVM_StyxRpcSession *script = NULL;
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
    if (status == FVM_OK && playback->spec.session)
    {
        status = fvm_styxrpc_filesystem_mount(context, "/", playback->spec.session);
        if (status == FVM_OK)
        {
            status = fvm_filesystem_install(context, "fs");
        }
        if (status == FVM_OK)
        {
            status = fvm_styxrpc_install_session(context, playback->spec.session, &script);
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
            movies->limit = playback->spec.upload_limit;
            if (movies->limit > FVM_SWF_DEFAULT_LIMIT)
            {
                movies->limit = FVM_SWF_DEFAULT_LIMIT;
            }
            status = fvm_styxrpc_movies_bind(movies);
        }
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
    playback->period = PLAYBACK_SECONDS_MS / (info.frame_rate_fixed8 ?
        (double)info.frame_rate_fixed8 / PLAYBACK_FIXED8_RATE_UNIT : PLAYBACK_FALLBACK_FPS);
    playback->ticks = 0;
    playback->state = PLAYBACK_READY;
    return FVM_OK;
}


/* [ASINC] EN: Replace the movie only after validating a complete SWF.
 * RU: Заменить фильм только после проверки полного SWF. */
static int upload(void *data, const void *bytes, size_t size)
{
    FVM_StyxRpcPlayback *playback = data;
    unsigned char *copy = malloc(size);
    if (!copy)
    {
        playback->last = FVM_OUT_OF_MEMORY;
        return 0;
    }
    memcpy(copy, bytes, size);
    playback->last = prepare(playback, copy, size);
    if (playback->last != FVM_OK)
    {
        free(copy);
        return 0;
    }
    free(playback->swf);
    playback->swf = copy;
    playback->size = size;
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
    return playback->last;
}


/* [ASINC] EN: Apply a recognized command between owner activations.
 * RU: Применить распознанную команду между активациями владельца. */
static int command(void *data, StyxRpcControlCommand what)
{
    FVM_StyxRpcPlayback *playback = data;
    if (!playback->context)
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
    return fvm_styxrpc_playback_tick(playback, PLAYBACK_FIRST_TICK) == FVM_OK;
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


/* EN: Format CPU use for this owner thread since the previous sample when clocks exist.
 * RU: Вычислить загрузку потока владельца с прошлого снимка при наличии часов. */
static int owner_cpu(FVM_StyxRpcPlayback *playback, double *percent)
{
#if defined(CLOCK_MONOTONIC) && defined(CLOCK_THREAD_CPUTIME_ID)
    struct timespec wall_time, cpu_time;
    double wall, cpu;
    int measured;
    if (clock_gettime(CLOCK_MONOTONIC, &wall_time) || clock_gettime(CLOCK_THREAD_CPUTIME_ID, &cpu_time))
    {
        return 0;
    }
    wall = (double)wall_time.tv_sec + (double)wall_time.tv_nsec / 1000000000.0;
    cpu = (double)cpu_time.tv_sec + (double)cpu_time.tv_nsec / 1000000000.0;
    measured = playback->last_wall > 0 && wall > playback->last_wall && cpu >= playback->last_cpu;
    if (measured)
    {
        *percent = (cpu - playback->last_cpu) / (wall - playback->last_wall) * 100.0;
    }
    playback->last_wall = wall;
    playback->last_cpu = cpu;
    return measured;
#else
    (void)playback;
    (void)percent;
    return 0;
#endif
}


/* EN: Match the prior kernel row by thread ID and creation time and report a rate.
 * RU: Найти предыдущую запись ядра по ID и времени создания и вычислить загрузку. */
static void process_cpu(const FVM_StyxRpcProcessStats *prior, const FVM_StyxRpcThreadSample *row,
    double elapsed, char *bytes, size_t capacity)
{
    unsigned i;
    strcpy(bytes, "unavailable");
    if (elapsed <= 0)
    {
        return;
    }
    for (i = 0; i < prior->used; ++i)
    {
        const FVM_StyxRpcThreadSample *old = prior->rows + i;
        if (old->id == row->id && old->created == row->created && old->cpu <= row->cpu)
        {
            snprintf(bytes, capacity, "%.2f", (row->cpu - old->cpu) / elapsed * 100.0);
            return;
        }
    }
}


/* EN: Append measured process metrics while preserving space for application telemetry.
 * RU: Дописать измерения процесса, оставив место для статистики приложения. */
static size_t append_process(FVM_StyxRpcPlayback *playback, char *bytes, size_t capacity)
{
    FVM_StyxRpcProcessStats current;
    unsigned i;
    size_t used;
    int count;
    if (!fvm_styxrpc_process_stats(&current))
    {
        count = snprintf(bytes, capacity, "process.threads=unavailable\nprocess.rss_bytes=unavailable\n");
        return count >= 0 && (size_t)count < capacity ? (size_t)count : 0;
    }
    count = snprintf(bytes, capacity, "process.threads=%u\nprocess.rss_bytes=%lu\nprocess.cpu_rows=%u\n",
        current.count, current.rss, current.used);
    if (count < 0 || (size_t)count >= capacity)
    {
        return 0;
    }
    used = (size_t)count;
    for (i = 0; i < current.used; ++i)
    {
        char cpu[32];
        process_cpu(&playback->process, current.rows + i, current.time - playback->process.time, cpu, sizeof(cpu));
        count = snprintf(bytes + used, capacity - used, "process.thread.%lu.cpu_percent=%s\n", current.rows[i].id, cpu);
        if (count < 0 || (size_t)count >= capacity - used)
        {
            break;
        }
        used += (size_t)count;
    }
    playback->process = current;
    return used;
}


/* EN: Copy a bounded status with measured VM and owner statistics, plus host extensions.
 * RU: Скопировать ограниченный снимок измерений VM и потока, дополнив данными хоста. */
static int snapshot(void *data, char *bytes, size_t capacity, size_t *used)
{
    static const char *const states[] = {"empty", "ready", "running", "paused", "debug", "failed"};
    FVM_StyxRpcPlayback *playback = data;
    FVM_GCStats gc;
    FVM_GCStringStats strings;
    FVM_ExecutionStats execution;
    double cpu;
    char load[32];
    int length;
    size_t used_bytes, extra = 0;
    memset(&gc, 0, sizeof(gc));
    memset(&strings, 0, sizeof(strings));
    memset(&execution, 0, sizeof(execution));
    gc.struct_size = sizeof(gc);
    execution.struct_size = sizeof(execution);
    if (playback->context &&
        (fvm_gc_stats(playback->context, &gc) != FVM_OK ||
         fvm_gc_string_stats(playback->context, &strings) != FVM_OK ||
         fvm_execution_get_stats(playback->context, &execution) != FVM_OK))
    {
        return 0;
    }
    strcpy(load, "unavailable");
    if (owner_cpu(playback, &cpu))
    {
        snprintf(load, sizeof(load), "%.2f", cpu);
    }
    length = snprintf(bytes, capacity,
        "state=%s\nlast_result=%d\nticks=%lu\nthreads.owner=1\nthread.owner.cpu_percent=%s\n"
        "swf_bytes=%lu\nstring_bytes=%lu\nlive_objects=%lu\npinned_objects=%lu\ngc_pending=%lu\n"
        "interpreter_entries=%lu\nnative_entries=%lu\ncompiled_bodies=%lu\n"
        "movies_loaded=%lu\nmovie_load_errors=%lu\nlast_movie_result=%d\n",
        states[playback->state], (int)playback->last, playback->ticks, load,
        (unsigned long)playback->size, (unsigned long)strings.allocated_bytes,
        gc.live_objects, gc.pinned_objects, gc.pending_allocations,
        execution.interpreter_entries, execution.native_entries, execution.compiled_bodies,
        playback->movies ? playback->movies->loaded : 0UL,
        playback->movies ? playback->movies->failed : 0UL,
        playback->movies ? (int)playback->movies->last : FVM_OK);
    if (length < 0 || (size_t)length >= capacity)
    {
        return 0;
    }
    used_bytes = (size_t)length;
    used_bytes += append_process(playback, bytes + used_bytes, capacity - used_bytes);
    if (playback->spec.statistics)
    {
        size_t remaining = capacity - used_bytes;
        if (!playback->spec.statistics(playback->spec.host, bytes + used_bytes, remaining, &extra) || extra > remaining)
        {
            return 0;
        }
    }
    *used = used_bytes + extra;
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
    control.status = snapshot;
    if (styxrpc_control_create(&control, &playback->control) != STYXRPC_OK)
    {
        free(playback);
        return FVM_INVALID_ARGUMENT;
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
void fvm_styxrpc_playback_destroy(FVM_StyxRpcPlayback *playback)
{
    if (playback)
    {
        styxrpc_control_destroy(playback->control);
        dispose_movie(playback->context, playback->movies, playback->script);
        free(playback->swf);
        free(playback);
    }
}
