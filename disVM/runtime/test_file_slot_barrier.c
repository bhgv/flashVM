/* Copyright (c) 2026 bhgv
 * EN: Align real resident slot reservations before native gate construction.
 * RU: Совместить резервации слотов потоков перед созданием нативного шлюза.
 */
#include "lib9.h"
#include "dat.h"
#include "interp.h"
#include "file_gate.h"
#include "disvm/native_context.h"
#include <pthread.h>

static pthread_mutex_t sync = PTHREAD_MUTEX_INITIALIZER; /* Private barrier mutex. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Generation predicate. */
static unsigned arrived; /* Participants in the current generation, under sync. */
static unsigned generation; /* Completed generations, under sync. */

DVM_FileGate *__real_dvm_file_gate_create(void);
void __real_dvm_managed_workers_file_call(void);
static _Thread_local int counted; /* Align only the first real file call per resident. */


/* EN: Forward real allocation after all 16 residents reach this boundary.
 * No arguments; return the actual gate. Controller/loader opens are unchanged.
 * The test extends a legal scheduling gap, without changing slot contents.
 * RU: Вызвать настоящее выделение, когда все 16 потоков достигнут границы.
 * Без аргументов; вернуть настоящий шлюз. Контроллер/загрузчик не задерживаются.
 * Тест расширяет допустимый разрыв исполнения, не меняя содержимое слотов. */
DVM_FileGate *__wrap_dvm_file_gate_create(void)
{
    Prog *process = dvm_native_context_process(dvm_native_context_current());
    if (process && process->pid > 1)
    {
        pthread_mutex_lock(&sync);
        unsigned current = generation;
        if (++arrived == 16)
        {
            arrived = 0;
            generation++;
            pthread_cond_broadcast(&changed);
        }
        while (current == generation)
        {
            pthread_cond_wait(&changed, &sync);
        }
        pthread_mutex_unlock(&sync);
    }
    return __real_dvm_file_gate_create();
}


/* EN: Align the first real file-call counter update on sixteen owned residents.
 * No VM/counter result is replaced. All callers retain their actual FD/frame.
 * RU: Совместить первое настоящее обновление счётчика у шестнадцати потоков.
 * Результат VM/счётчика не меняется; вызывающие держат свои FD/кадры. */
void __wrap_dvm_managed_workers_file_call(void)
{
    Prog *process = dvm_native_context_process(dvm_native_context_current());
    if (process && process->pid > 1 && !counted)
    {
        counted = 1;
        pthread_mutex_lock(&sync);
        unsigned current = generation;
        if (++arrived == 16)
        {
            arrived = 0;
            generation++;
            pthread_cond_broadcast(&changed);
        }
        while (current == generation)
        {
            pthread_cond_wait(&changed, &sync);
        }
        pthread_mutex_unlock(&sync);
    }
    __real_dvm_managed_workers_file_call();
}
