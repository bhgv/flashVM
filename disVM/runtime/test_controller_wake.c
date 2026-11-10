/* Force a channel wake before the controller samples the park generation. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER;
static int parking;
static int woken;
static int checked;
static int (*controller_pending)(void);

int __real_dvm_runqueue_claim(Prog *process);
int __wrap_dvm_runqueue_claim(Prog *process)
{
    if (process->pid == 2)
    {
        pthread_mutex_lock(&gate);
        while (!parking)
            pthread_cond_wait(&changed, &gate);
        pthread_mutex_unlock(&gate);
    }
    return __real_dvm_runqueue_claim(process);
}

void __real_dvm_managed_workers_wake_guest(Prog *guest);
void __wrap_dvm_managed_workers_wake_guest(Prog *guest)
{
    __real_dvm_managed_workers_wake_guest(guest);
    if (guest->pid == 1)
    {
        pthread_mutex_lock(&gate);
        if (parking && !woken)
        {
            woken = 1;
            pthread_cond_broadcast(&changed);
            while (!checked)
                pthread_cond_wait(&changed, &gate);
        }
        pthread_mutex_unlock(&gate);
    }
}

/* Keep later worker notifications out of the forced interval. */
static int check_controller_pending(void)
{
    int pending = controller_pending();
    if (!pending)
    {
        fputs("controller missed readiness published before park\n", stderr);
        exit(1);
    }
    pthread_mutex_lock(&gate);
    checked = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&gate);
    return pending;
}

int __real_dvm_managed_workers_park(int (*pending)(void));
int __wrap_dvm_managed_workers_park(int (*pending)(void))
{
    pthread_mutex_lock(&gate);
    if (!parking)
    {
        parking = 1;
        pthread_cond_broadcast(&changed);
        while (!woken)
            pthread_cond_wait(&changed, &gate);
        puts("PASS controller wake before park");
        controller_pending = pending;
        pending = check_controller_pending;
    }
    pthread_mutex_unlock(&gate);
    return __real_dvm_managed_workers_park(pending);
}
