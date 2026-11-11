/* Copyright (c) 2026 bhgv
 * EN: Hold one genuine native close while observing detached jobs and GC progress.
 * RU: Задерживать настоящее native close и наблюдать отделённые jobs и прогресс GC.
 */
#include "lib9.h"
#include "dat.h"
#include "interp.h"
#include "runtime_gc.h"
#include "close_worker.h"
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static pthread_mutex_t sync = PTHREAD_MUTEX_INITIALIZER; /* Protect test observations only. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Submitted-job notifications. */
static FVM_PortFile *first; /* Borrowed native handle until its genuine close completes. */
static FVM_PortFile *handles[64]; /* Borrowed identities of the 64 detached handles. */
static FVM_PortDirectory *directories[64]; /* Alternative native directory identities. */
static int closing[64]; /* Each tracked handle may enter genuine close only once. */
static unsigned registered; /* Handle identities published before the real submit. */
static unsigned submitted; /* Real queue publications observed after their return. */
static unsigned completed; /* Real closes of the 64 tracked native handles. */
static unsigned checkpoints; /* Parent's observed producer retirement boundaries. */
static int armed; /* Enabled only by the managed fixture after loader cleanup. */
static int held; /* First close reached the inherited release pipe. */

void __real_dvm_close_submit(DVM_CloseJob *, FVM_PortFile *, FVM_PortDirectory *);
void __real_fvm_port_file_close(FVM_PortFile *);
void __real_fvm_port_directory_close(FVM_PortDirectory *);
void __real_Sys_werrstr(void *);
void __real_dvm_close_worker_stop(void);
Channel *__real_dvm_runtime_channel_new(void *, Type *, WORD, void (*)(void), int);


/* EN: Require condition or terminate with borrowed message, without managed access.
 * RU: Проверить condition либо завершить с заимствованным message без доступа к VM. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL close backlog: %s\n", message);
        exit(12);
    }
}


/* EN: Observe production ownership transfer of job/file/directory, preserving all
 * inputs. Never retain the queue mutex or any VM lock across the real publication.
 * RU: Наблюдать настоящую передачу job/file/directory без изменения аргументов.
 * Не удерживать mutex очереди или VM при настоящей публикации. */
void __wrap_dvm_close_submit(DVM_CloseJob *job, FVM_PortFile *file, FVM_PortDirectory *directory)
{
    pthread_mutex_lock(&sync);
    if (!armed)
    {
        pthread_mutex_unlock(&sync);
        __real_dvm_close_submit(job, file, directory);
        return;
    }
    require((file != NULL) != (directory != NULL), "each fixture job owns one real handle");
    require(registered < 64, "bounded fixture handle observations");
    handles[registered] = file;
    directories[registered++] = directory;
    if (!first)
    {
        first = file;
    }
    pthread_mutex_unlock(&sync);
    __real_dvm_close_submit(job, file, directory);
    pthread_mutex_lock(&sync);
    submitted++;
    require(submitted <= 64, "exactly 64 managed native close jobs");
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&sync);
}


/* EN: Claim one borrowed native handle identity in FIFO order before real close.
 * Return its observation index or -1 for untracked/null handles; never read payload.
 * RU: Отметить заимствованный native handle в порядке FIFO перед close. Вернуть
 * индекс наблюдения либо -1 для чужого/null handle; payload не читается. */
static int claim_close(FVM_PortFile *file, FVM_PortDirectory *directory)
{
    pthread_mutex_lock(&sync);
    unsigned index;
    for (index = 0; index < registered; index++)
    {
        if ((file && handles[index] == file) || (directory && directories[index] == directory))
        {
            require(!closing[index] && index == completed, "native handles close once in FIFO order");
            closing[index] = 1;
            break;
        }
    }
    int result = index < registered ? (int)index : -1;
    pthread_mutex_unlock(&sync);
    return result;
}


/* EN: Account for a returned genuine close of index, ignoring untracked -1.
 * RU: Учесть вернувшийся настоящий close для index, пропуская чужой индекс -1. */
static void finish_close(int index)
{
    if (index >= 0)
    {
        pthread_mutex_lock(&sync);
        completed++;
        pthread_mutex_unlock(&sync);
    }
}


/* EN: Hold only the first tracked close on the native service thread; file is
 * exclusively owned by its real job. Forward every port close exactly once.
 * No managed heap admission, pointers, callbacks or scheduler locks are involved.
 * RU: Задержать только первое закрытие в native-службе; file принадлежит job.
 * Выполнить каждый настоящий close ровно один раз. Здесь нет допуска в кучу,
 * управляемых указателей, callbacks или мьютексов планировщика. */
void __wrap_fvm_port_file_close(FVM_PortFile *file)
{
    int index = claim_close(file, NULL);
    pthread_mutex_lock(&sync);
    int pause = file && file == first && !held;
    if (pause)
    {
        held = 1;
    }
    pthread_mutex_unlock(&sync);
    if (pause)
    {
        fprintf(stderr, "CLOSE_BACKLOG_HELD\n");
        const char *descriptor = getenv("DVM_CLOSE_BACKLOG_RELEASE");
        require(descriptor != NULL, "native release pipe is inherited");
        char permit;
        ssize_t count;
        do
        {
            count = read(atoi(descriptor), &permit, 1);
        } while (count < 0 && errno == EINTR);
        require(count == 1 && permit == '1', "release follows the complete backlog snapshot");
    }
    __real_fvm_port_file_close(file);
    finish_close(index);
}


/* EN: Forward the real close of borrowed directory, auditing exact FIFO identity.
 * RU: Выполнить настоящий close заимствованного directory, проверив его место FIFO. */
void __wrap_fvm_port_directory_close(FVM_PortDirectory *directory)
{
    int index = claim_close(NULL, directory);
    __real_fvm_port_directory_close(directory);
    finish_close(index);
}


/* EN: Forward real werrstr on borrowed frame, publish native roots and wait for
 * this producer's real detached job. The host releases close only after job 64.
 * RU: Выполнить настоящий werrstr на frame, опубликовать корни и ждать отделённую
 * job этого производителя. Хост разрешит close только после job 64. */
void __wrap_Sys_werrstr(void *frame)
{
    __real_Sys_werrstr(frame);
    int native = dvm_runtime_gc_native_try_leave();
    require(native > 0, "checkpoint publishes genuine parent frame roots");
    pthread_mutex_lock(&sync);
    unsigned expected = ++checkpoints;
    while (submitted < expected)
    {
        pthread_cond_wait(&changed, &sync);
    }
    if (expected == 64)
    {
        require(submitted == 64 && completed == 0, "all detached jobs remain pending");
        fprintf(stderr, "CLOSE_BACKLOG_READY submitted=64 completed=0\n");
    }
    pthread_mutex_unlock(&sync);
    if (expected == 64)
    {
        dvm_close_statistics();
    }
    dvm_runtime_gc_native_enter(native);
}


/* EN: Preserve real shutdown/drain, then require every queued handle closed once.
 * No arguments/result or VM access; producers have stopped under the real contract.
 * RU: Выполнить настоящее завершение/дренирование и проверить все закрытия.
 * Без аргументов/результата или доступа к VM; производители уже остановлены. */
void __wrap_dvm_close_worker_stop(void)
{
    __real_dvm_close_worker_stop();
    pthread_mutex_lock(&sync);
    require(held && checkpoints == 64 && registered == 64 && submitted == 64 && completed == 64,
        "native close shutdown drains every queued handle exactly once");
    unsigned directory_count = 0;
    unsigned index;
    for (index = 0; index < registered; index++)
    {
        directory_count += directories[index] != NULL;
    }
    pthread_mutex_unlock(&sync);
    fprintf(stderr, "PASS native close backlog submitted=64 completed=64 files=%u directories=%u\n",
        64 - directory_count, directory_count);
}


/* EN: Capacity 105 arms observations after loader cleanup. All arguments are
 * borrowed constructor inputs, forwarded unchanged; return the genuine channel.
 * RU: Вместимость 105 включает наблюдение после очистки загрузчика. Все аргументы
 * заимствованы и передаются без изменений; вернуть настоящий канал.
 */
Channel *__wrap_dvm_runtime_channel_new(void *destination, Type *type, WORD capacity,
    void (*mover)(void), int guarded)
{
    if (capacity == 105)
    {
        pthread_mutex_lock(&sync);
        require(!armed, "activate the native close observer exactly once");
        armed = 1;
        pthread_mutex_unlock(&sync);
    }
    return __real_dvm_runtime_channel_new(destination, type, capacity, mover, guarded);
}
