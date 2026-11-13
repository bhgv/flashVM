/* Copyright (c) 2026 bhgv
 * EN: Seal a real child between the parent's lifetime sample and wait admission.
 * RU: Завершить настоящий lifetime ребёнка между снимком родителя и допуском wait.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runqueue.h"
#include "runt.h"
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static pthread_mutex_t sync = PTHREAD_MUTEX_INITIALIZER;  /* Fixture predicates only. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Owner/parent handoff. */
static int child_ready;                                   /* Child reached its own retirement call, under sync. */
static int sampled;                                       /* Parent captured the live lifetime value, under sync. */
static int sealed;                                        /* Child's actual retirement CAS succeeded, under sync. */
static int result_seen;           /* Old-result control may release its registered child, under sync. */
static int target_pid;            /* Immutable after child_ready publication. */
static __thread int started;      /* Parent entered the first preflight. */
static __thread int sampling;     /* Only the parent preflight samples the selected child. */
static __thread int sample_calls; /* First lifetime load in that preflight. */
static __thread int expected_pid; /* Selected direct child, on parent owner. */
static __thread int first_parked; /* Actual first preflight result, on parent owner. */


/* EN/RU: Real native preflight borrows caller/pid through its synchronous call.
 * Настоящий preflight заимствует caller/pid до синхронного возврата. */
int __real_dvm_runtime_prepare_killwait(Prog *caller, int pid);


/* EN/RU: Real lifetime load borrows independently live process without a hold.
 * Настоящий lifetime load заимствует живой process, не выдавая hold. */
int __real_dvm_runqueue_hold_count(Prog *process);


/* EN/RU: Real retirement seals borrowed process only on its permanent owner.
 * Настоящее завершение закрывает process только на постоянном владельце. */
int __real_dvm_runqueue_begin_retire(Prog *process);


/* EN/RU: Real result borrows caller/pid and consumes a matching completion.
 * Настоящий result заимствует caller/pid и снимает согласованное завершение. */
int __real_dvm_runtime_killwait_result(Prog *caller, int pid);


/* EN: Fail condition precisely with borrowed message; terminate peers with 12.
 * RU: При ложном condition завершить партнёров кодом 12 с точным message. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL killwait seal: %s\n", message);
        fflush(stderr);
        _Exit(12);
    }
}


/* EN: Wait for borrowed fixture flag without touching managed data. Parent may
 * hold the registry mutex: child sealing needs only its atomic lifetime CAS.
 * No path holds sync while acquiring that registry mutex. Timeout is a failure.
 * RU: Ждать тестовый flag без доступа к managed-данным. Родитель может держать
 * mutex реестра: ребёнку для sealing нужен лишь atomic CAS. Под sync никто
 * не захватывает реестр. Таймаут является отказом. */
static void wait_flag(int *flag)
{
    struct timespec limit;
    require(!clock_gettime(CLOCK_REALTIME, &limit), "native clock");
    limit.tv_sec += 30;
    pthread_mutex_lock(&sync);
    while (!*flag)
    {
        int result = pthread_cond_timedwait(&changed, &sync, &limit);
        require(result == 0 || (result == ETIMEDOUT && *flag), "bounded handoff timeout");
    }
    pthread_mutex_unlock(&sync);
}


/* EN: Let the child itself seal process after the parent sampled live. The
 * child remains rooted until returning to normal retirement. CONTROL=1 keeps
 * it registered until the obsolete preflight exposes its actual result zero.
 * RU: Дать самому ребёнку закрыть process после живого снимка родителя.
 * Ребёнок укоренён до возврата в обычное завершение. CONTROL=1 удерживает его
 * в реестре до наблюдения настоящего нулевого результата старого preflight. */
int __wrap_dvm_runqueue_begin_retire(Prog *process)
{
    if (process->pid == 1)
    {
        return __real_dvm_runqueue_begin_retire(process);
    }
    require(currun() == process, "child seals on its permanent owner");
    pthread_mutex_lock(&sync);
    require(!child_ready, "one selected child");
    target_pid = process->pid;
    child_ready = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&sync);
    wait_flag(&sampled);
    int result = __real_dvm_runqueue_begin_retire(process);
    require(result == 1, "actual child lifetime seal");
    pthread_mutex_lock(&sync);
    sealed = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&sync);
    if (CONTROL == 1)
    {
        wait_flag(&result_seen);
    }
    return result;
}


/* EN: Return the real first live lifetime sample after the child has sealed.
 * process stays registered under caller's mutex. A second old admission load
 * returns the true negative value; no foreign lifetime mutation is fabricated.
 * RU: Вернуть настоящий первый живой снимок после sealing ребёнком. process
 * удерживается реестром вызывающего. Повторное старое чтение вернёт настоящий
 * отрицательный счётчик; чужая жизнь не подменяется тестом. */
int __wrap_dvm_runqueue_hold_count(Prog *process)
{
    int result = __real_dvm_runqueue_hold_count(process);
    if (sampling && process->pid == expected_pid && sample_calls++ == 0)
    {
        require(result == 0, "sampled a live unpinned child");
        pthread_mutex_lock(&sync);
        sampled = 1;
        pthread_cond_broadcast(&changed);
        pthread_mutex_unlock(&sync);
        wait_flag(&sealed);
    }
    return result;
}


/* EN: Borrow caller/pid and exercise the genuine preflight. Synchronize only its
 * first call; a resumed MCALL must consume its normal retirement completion.
 * RU: Заимствовать caller/pid и выполнить настоящий preflight. Согласовать лишь
 * первый вызов; возобновлённый MCALL снимает штатное завершение ребёнка. */
int __wrap_dvm_runtime_prepare_killwait(Prog *caller, int pid)
{
    require(caller->pid == 1 && currun() == caller, "parent owns preflight");
    if (started)
    {
        return __real_dvm_runtime_prepare_killwait(caller, pid);
    }
    started = 1;
    wait_flag(&child_ready);
    require(target_pid == pid, "selected direct child");
    expected_pid = pid;
    sampling = 1;
    int ready = __real_dvm_runtime_prepare_killwait(caller, pid);
    sampling = 0;
    first_parked = !ready;
    return ready;
}


/* EN: Observe the real caller/pid result after matching managed retirement.
 * Old control must expose NotWaiting while still registered, never Denied.
 * RU: Наблюдать настоящий result caller/pid после managed-завершения.
 * Старый контроль должен показать NotWaiting в реестре, а не Denied. */
int __wrap_dvm_runtime_killwait_result(Prog *caller, int pid)
{
    int result = __real_dvm_runtime_killwait_result(caller, pid);
    pthread_mutex_lock(&sync);
    require(sampled && sealed && target_pid == pid, "real seal preceded result");
    result_seen = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&sync);
    fprintf(stderr, "KILLWAIT_SEAL parked=%d result=%d sampled_live=1 sealed=1\n", first_parked, result);
    require(result == Processctl_Retired, "registered child returned NotWaiting");
    require(first_parked, "registered child wait was admitted");
    return result;
}
