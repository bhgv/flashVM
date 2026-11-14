/* Copyright (c) 2026 bhgv
 * EN: Protect native compilation and publish immutable code to parallel residents.
 * RU: Защитить native-компиляцию и публиковать неизменный код параллельным потокам.
 */
#include "lib9.h"
#include "interp.h"
#include "jit_publication.h"
#include "jit_quarantine.h"
#include <pthread.h>

static pthread_mutex_t builder_mutex = PTHREAD_MUTEX_INITIALIZER; /* Compilation metadata/code only, never execution. */


/* [GC:] EN: Enter the ordinary native builder mutex; no parameters or ownership.
 * RU: Войти в обычный mutex native-строителя; без параметров/передачи владения. */
void dvm_jit_build_lock(void)
{
    pthread_mutex_lock(&builder_mutex);
}


/* [GC:] EN: Leave builder mutex after protected compilation and cleanup finish.
 * RU: Снять mutex строителя после защищённой компиляции и очистки. */
void dvm_jit_build_unlock(void)
{
    pthread_mutex_unlock(&builder_mutex);
}


/* [GC:] EN: Acquire the published table of caller-retained module; borrow only.
 * RU: Прочитать опубликованную таблицу удерживаемого module; только заимствование. */
Jit *dvm_jit_load(Module *module)
{
    return __atomic_load_n(&module->jit, __ATOMIC_ACQUIRE);
}


/* [GC:] EN: Release-publish complete owned code to caller-retained module.
 * Caller holds builder mutex and has completed executable permission setup.
 * RU: Опубликовать готовый code в удерживаемый module с release; вызывающий
 * держит mutex строителя и уже подготовил права исполняемой памяти. */
void dvm_jit_publish(Module *module, Jit *code)
{
    code->generation = dvm_jit_new_generation();
    __atomic_store_n(&module->jit, code, __ATOMIC_RELEASE);
}
