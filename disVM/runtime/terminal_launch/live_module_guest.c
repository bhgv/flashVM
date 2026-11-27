/* Copyright (c) 2026 bhgv
 * [GC:] EN: Start one permanent Dis owner for each admitted live load.
 * RU: Запустить одного постоянного владельца Dis на принятую загрузку.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "kernel.h"
#include "live_modules_private.h"
#include "client.h"
#include "live_module_owner_image.h"
#include "../vfs/guest.h"
#include "../vfs/cancel.h"
#include "../vfs/native_wait.h"
#include "../vfs/launch/runtime.h"
#include "runtime_gc.h"
#include "sys_capture.h"
#include "vfsnative_decl.h"
#include <stddef.h>
#include <time.h>
#include <string.h>
#include <sys/socket.h>


/* EN: Record the exact resident PID before it can submit its profile.
 * RU: Записать точный PID владельца до отправки им своего профиля. */
static void started(void *state, int pid)
{
    LiveModule *entry = state;
    LiveModules *session = dvm_live_modules_get();
    pthread_mutex_lock(&session->lock);
    entry->pid = pid;
    pthread_mutex_unlock(&session->lock);
}


/* EN: Revoke an exited owner's route and wake any requester that is still waiting.
 * RU: Отозвать маршрут вышедшего владельца и пробудить ожидающих клиентов. */
static void retired(void *state, int status)
{
    (void)status;
    LiveModule *entry = state;
    LiveModules *session = dvm_live_modules_get();
    pthread_mutex_lock(&session->lock);
    entry->phase = LIVE_FAILED;
    entry->worker = 0;
    if (entry->breaker >= 0)
    {
        shutdown(entry->breaker, SHUT_RDWR);
    }
    pthread_cond_broadcast(&session->changed);
    pthread_mutex_unlock(&session->lock);
}


typedef struct LiveStartup
{
    DVM_LaunchHandoff handoff; /* Owned namespace until successful adoption. */
    Module *bootstrap;         /* Owned compiled worker through launch. */
    DVM_LaunchCompletion completion; /* Copied callbacks for this resident. */
} LiveStartup;


/* [GC:] EN: Decode the shipped worker and launch it with a private namespace.
 * RU: Разобрать поставляемый образ и запустить его в частном пространстве имён. */
static int launch(void *state)
{
    LiveStartup *job = state;
    Dir metadata = {0};
    uchar image[sizeof(live_module_owner_image)];
    memcpy(image, live_module_owner_image, sizeof(image));
    job->bootstrap = parsemod("$StyxLiveModuleOwner", image, sizeof(image), &metadata);
    return job->bootstrap && dvm_runtime_launch_child(job->bootstrap, &job->handoff, &job->completion);
}


/* EN: Stop the broker if its dedicated listener exits.
 * RU: Остановить брокер, если его отдельный приёмник завершился. */
static void listener_retired(void *state, int status)
{
    (void)state;
    (void)status;
    dvm_live_modules_stop();
}


/* [GC:] EN: Adopt a private group and start one independent guest with the given arguments.
 * RU: Принять частное пространство и запустить гостя с указанными аргументами. */
static int start_worker(LiveModules *session, char *arguments, const DVM_LaunchCompletion *completion)
{
    LiveStartup job = {.completion = *completion};
    DVM_LaunchHandoff *handoff = &job.handoff;
    handoff->group = dvm_namespace_copy(session->group);
    handoff->local_role = DVM_ORIGIN_PROVIDER;
    strcpy(handoff->description.module, "styx-live-module-owner");
    handoff->arguments = (unsigned char *)arguments;
    handoff->description.argument_bytes = strlen(arguments);
    int created = handoff->group && kprotect(launch, &job);
    if (job.bootstrap)
    {
        unload(job.bootstrap);
    }
    dvm_namespace_release(handoff->group);
    return created;
}


/* EN: Record the only resident allowed to dequeue requests.
 * RU: Записать единственного владельца, которому разрешено брать запросы. */
static void listener_started(void *state, int pid)
{
    LiveModules *session = state;
    pthread_mutex_lock(&session->lock);
    session->listener = pid;
    pthread_mutex_unlock(&session->lock);
}


/* EN/RU: See live_modules.h. / См. live_modules.h. */
int dvm_live_modules_boot(void)
{
    LiveModules *session = dvm_live_modules_get();
    char arguments[] = "live-listen";
    const DVM_LaunchCompletion completion = {.report = listener_retired,
        .state = session, .started = listener_started};
    return !session || start_worker(session, arguments, &completion);
}


/* EN: Take one queued request or wait outside managed admission until signalled.
 * RU: Взять ожидающий запрос или ждать сигнала вне допуска к управляемой куче. */
static LiveModule *take(LiveModules *session, int pid)
{
    LiveModule *entry = NULL;
    pthread_mutex_lock(&session->lock);
    while (!session->stopped && pid == session->listener && !dvm_cancel_requested(dvm_cancel_current()))
    {
        LiveModule retired_entry = {0};
        for (size_t i = 0; i < DVM_LIVE_MODULE_LIMIT; ++i)
        {
            LiveModule *candidate = &session->entries[i];
            if (candidate->phase == LIVE_FAILED && !candidate->leased && !candidate->worker)
            {
                retired_entry = *candidate;
                memset(candidate, 0, sizeof(*candidate));
                break;
            }
            if (candidate->phase == LIVE_QUEUED)
            {
                entry = candidate;
                entry->phase = LIVE_BOOTING;
                entry->worker = 1;
                break;
            }
        }
        if (retired_entry.generation)
        {
            pthread_mutex_unlock(&session->lock);
            dvm_live_module_release(&retired_entry);
            pthread_mutex_lock(&session->lock);
            continue;
        }
        if (entry)
        {
            break;
        }
        struct timespec deadline;
        if (!styxclientalive(session->client) || clock_gettime(CLOCK_MONOTONIC, &deadline))
        {
            break;
        }
        ++deadline.tv_sec;
        pthread_cond_timedwait(&session->changed, &session->lock, &deadline);
    }
    pthread_mutex_unlock(&session->lock);
    return entry;
}


/* [GC:] EN: Run the listener on its permanent owner, starting a child per request.
 * RU: Выполнять приёмник на закреплённом владельце, создавая потомка на запрос. */
void dvm_live_modules_listen(int pid)
{
    LiveModules *session = dvm_live_modules_get();
    if (!session)
    {
        return;
    }
    for (;;)
    {
        DVM_VfsWait *wait;
        int released = dvm_vfs_path_enter(&wait);
        if (released < 0)
        {
            return;
        }
        LiveModule *entry = take(session, pid);
        int cancelled = dvm_vfs_wait_end(wait);
        dvm_runtime_gc_native_enter(released);
        if (cancelled || !entry)
        {
            return;
        }
        char arguments[sizeof(entry->path) + sizeof("live-load\t")];
        strcpy(arguments, "live-load\t");
        strcat(arguments, entry->path);
        const DVM_LaunchCompletion completion = {.report = retired, .state = entry, .started = started};
        if (!start_worker(session, arguments, &completion))
        {
            retired(entry, 1);
        }
    }
}
