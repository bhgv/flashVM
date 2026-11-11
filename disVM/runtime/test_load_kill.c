/* Copyright (c) 2026 bhgv
 * EN: Observe real module open ownership across accepted resident termination.
 * RU: Наблюдать владение настоящим открытием модуля при принятом завершении потока.
 */
#include "lib9.h"
#include "dat.h"
#include "interp.h"
#include "disvm/native_context.h"
#include "file_port.h"
#include "module_registry.h"
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static pthread_mutex_t sync = PTHREAD_MUTEX_INITIALIZER; /* Native fixture observations only. */
static FVM_PortFile *handles[32]; /* Genuine victim handles, retained by their real owners. */
static int closing[32]; /* Each observed handle can close only after its open returns. */
static unsigned phases; /* Numbered real open boundaries. */
static unsigned closed; /* Returned real closes of victim handles. */
static int held_pid; /* Copied resident identity for accepted killwait observation. */
static int held; /* One real open remains blocked on the inherited release pipe. */
static _Thread_local int lookup_seen; /* This resident completed the real slow-image lookup. */
static _Thread_local int lookup_cached; /* Lookup returned its production-owned Module reference. */

FVM_Result __real_fvm_port_file_open(const char *, const char *, FVM_PortFile **);
void __real_fvm_port_file_close(FVM_PortFile *);
int __real_dvm_runtime_prepare_killwait(Prog *, int);
void __real_dvm_close_worker_stop(void);
Module *__real_dvm_module_registry_lookup(const char *);


/* EN: Require condition or terminate with borrowed message, never mutating VM state.
 * RU: Проверить condition либо завершить с заимствованным message, не меняя VM. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL loader kill: %s\n", message);
        exit(12);
    }
}


/* EN: Observe the real lookup before loader I/O without retaining extra references.
 * path is borrowed; return its genuine owned Module or NULL unchanged. Only TLS
 * booleans record whether the current slow-image request used the existing cache.
 * RU: Наблюдать настоящий lookup до I/O без добавочных ссылок. path заимствован;
 * вернуть настоящий владеющий Module либо NULL. В TLS сохраняются только флаги
 * наличия кэша у текущего slow-запроса. */
Module *__wrap_dvm_module_registry_lookup(const char *path)
{
    Module *module = __real_dvm_module_registry_lookup(path);
    Prog *process = dvm_native_context_process(dvm_native_context_current());
    const char *name = path ? strrchr(path, '/') : NULL;
    name = name ? name + 1 : path;
    if (name && process && process->pid > 1 && !strncmp(name, "slow-", 5))
    {
        lookup_seen = 1;
        lookup_cached = module != NULL;
    }
    return module;
}


/* EN: Hold the successful real open only for a spawned resident's slow image.
 * path/mode/output borrow production inputs; preserve result/handle unchanged.
 * No GC admission, live-register or managed pointer changes are made here.
 * RU: Задержать успешное открытие slow-образа только у созданного потока.
 * path/mode/output заимствованы; результат/handle не меняются. Здесь не меняются
 * допуск GC, живые регистры или управляемые указатели. */
FVM_Result __wrap_fvm_port_file_open(const char *path, const char *mode, FVM_PortFile **output)
{
    FVM_Result result = __real_fvm_port_file_open(path, mode, output);
    Prog *process = dvm_native_context_process(dvm_native_context_current());
    if (!path)
    {
        return result;
    }
    const char *name = strrchr(path, '/');
    name = name ? name + 1 : path;
    if (result != FVM_OK || !process || process->pid <= 1 || strncmp(name, "slow-", 5))
    {
        return result;
    }
    const char *warm = getenv("DVM_LOADER_WARM");
    require(warm && lookup_seen && lookup_cached == atoi(warm),
        "real lookup confirms the requested cold or cached Module ownership");
    lookup_seen = 0;
    pthread_mutex_lock(&sync);
    require(!held && phases < 32 && *output, "one genuine victim open per phase");
    handles[phases++] = *output;
    held_pid = process->pid;
    held = 1;
    fprintf(stderr, "LOADER_OPEN_HELD %u\n", phases);
    pthread_mutex_unlock(&sync);
    const char *descriptor = getenv("DVM_LOADER_RELEASE");
    require(descriptor != NULL, "native loader release pipe is inherited");
    char permit;
    ssize_t count;
    do
    {
        count = read(atoi(descriptor), &permit, 1);
    } while (count < 0 && errno == EINTR);
    require(count == 1 && permit == '1', "release follows accepted killwait");
    pthread_mutex_lock(&sync);
    held = 0;
    held_pid = 0;
    pthread_mutex_unlock(&sync);
    return result;
}


/* EN: Announce only real accepted killwait for this held resident. caller/pid
 * follow production authorization; do not replace or synthesize its reply.
 * RU: Сообщить только о настоящем принятом killwait удерживаемого потока.
 * caller/pid проходят production-проверки; ответ не подменяется и не создаётся. */
int __wrap_dvm_runtime_prepare_killwait(Prog *caller, int pid)
{
    int result = __real_dvm_runtime_prepare_killwait(caller, pid);
    if (!result)
    {
        pthread_mutex_lock(&sync);
        if (held && pid == held_pid)
        {
            fprintf(stderr, "LOADER_KILL_ACCEPTED %u\n", phases);
        }
        pthread_mutex_unlock(&sync);
    }
    return result;
}


/* EN: Reject premature victim close and forward every real port close.
 * file is borrowed from the genuine cleanup path, never dereferenced by observer.
 * RU: Запретить ранний close жертвы и выполнить настоящий port close.
 * file заимствован у настоящего cleanup, наблюдатель его не разыменовывает. */
void __wrap_fvm_port_file_close(FVM_PortFile *file)
{
    pthread_mutex_lock(&sync);
    int tracked = -1;
    unsigned index = phases;
    while (file && index)
    {
        index--;
        if (handles[index] == file && !closing[index])
        {
            require(!held || index != phases - 1, "loader handle survives its live native open");
            closing[index] = 1;
            tracked = (int)index;
            break;
        }
    }
    pthread_mutex_unlock(&sync);
    __real_fvm_port_file_close(file);
    if (tracked >= 0)
    {
        pthread_mutex_lock(&sync);
        closed++;
        pthread_mutex_unlock(&sync);
    }
}


/* EN: After real close-service drain require every victim's handle retired.
 * No arguments/result; callers have already stopped actual producers/collectors.
 * RU: После настоящего дренажа close-службы проверить handles всех жертв.
 * Без аргументов/результата; производители/сборщики уже остановлены вызывающим. */
void __wrap_dvm_close_worker_stop(void)
{
    __real_dvm_close_worker_stop();
    pthread_mutex_lock(&sync);
    require(phases == 32 && closed == 32 && !held, "all 32 loader handles retire after native return");
    pthread_mutex_unlock(&sync);
    fprintf(stderr, "PASS native loader kill opens=32 closed=32\n");
}
