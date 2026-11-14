/* Copyright (c) 2026 bhgv
 * EN: Test-only native capability lifetime counts and one actual path-fallback omission.
 * RU: Только тестовые счётчики lifetime capabilities и один настоящий path-fallback omission.
 */
#include "resource.h"
#include "boot.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; /* Probe counters only, never production I/O. */
static unsigned long opened;                              /* Successful real kernel capability constructors. */
static unsigned long closed;                              /* Real closes after final native reference. */
static int omitted;                                       /* Atomic once-only benign fallback substitution. */
FVM_Result __real_fvm_port_anchor_open(const char *, FVM_PortAnchor **);
FVM_Result __real_fvm_port_anchor_open_at(FVM_PortAnchor *, const char *, FVM_PortAnchor **);
void __real_fvm_port_anchor_close(FVM_PortAnchor *);
void __real_dvm_namespace_boot_shutdown(void);
FVM_Result __real_dvm_resource_open(DVM_ResourcePlan *, const char *, FVM_PortFile **, FVM_PortDirectory **);


/* EN/RU: Count successful real native opens; return unchanged status.
 * Считать успешные native opens; вернуть неизменённый status. */
static FVM_Result count(FVM_Result status)
{
    if (status == FVM_OK)
    {
        pthread_mutex_lock(&mutex);
        opened++;
        pthread_mutex_unlock(&mutex);
    }
    return status;
}


/* EN/RU: Preserve path constructor and owned output; only scalar counter changes.
 * Сохранить path constructor/output; меняется только scalar counter. */
FVM_Result __wrap_fvm_port_anchor_open(const char *path, FVM_PortAnchor **out)
{
    return count(__real_fvm_port_anchor_open(path, out));
}


/* EN/RU: Preserve relative constructor; no synthetic handle or namespace mutation.
 * Сохранить relative constructor; без synthetic handles/namespace mutations. */
FVM_Result __wrap_fvm_port_anchor_open_at(FVM_PortAnchor *parent, const char *path, FVM_PortAnchor **out)
{
    return count(__real_fvm_port_anchor_open_at(parent, path, out));
}


/* EN/RU: Count actual consumed non-NULL handles, never keep their addresses.
 * Считать поглощённые non-NULL handles, не сохранять их адреса. */
void __wrap_fvm_port_anchor_close(FVM_PortAnchor *anchor)
{
    __real_fvm_port_anchor_close(anchor);
    if (anchor)
    {
        pthread_mutex_lock(&mutex);
        closed++;
        pthread_mutex_unlock(&mutex);
    }
}


/* EN: CONTROL1 reopens one old display pathname, reading the real decoy byte.
 * A fresh unregistered temporary avoids copying any live cleanup record.
 * RU: CONTROL1 переоткрывает один display path, читая настоящий decoy byte.
 * Новый unregistered temporary не копирует живой cleanup record. */
FVM_Result
__wrap_dvm_resource_open(DVM_ResourcePlan *plan, const char *mode, FVM_PortFile **file, FVM_PortDirectory **directory)
{
    int expected = 0;
    const char *suffix = "/source-a/token";
    size_t length = strlen(plan->names.native), tail = strlen(suffix);
    if (CONTROL == 1 && plan->anchor && length >= tail && !strcmp(plan->names.native + length - tail, suffix) &&
        __atomic_compare_exchange_n(&omitted, &expected, 1, 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED))
    {
        DVM_ResourcePlan fallback;
        memset(&fallback, 0, sizeof(fallback));
        fallback.names = plan->names;
        return __real_dvm_resource_open(&fallback, mode, file, directory);
    }
    return __real_dvm_resource_open(plan, mode, file, directory);
}


/* EN/RU: After joined shutdown require every real capability closed exactly once.
 * После joined shutdown требовать закрытия всех capabilities ровно один раз. */
void __wrap_dvm_namespace_boot_shutdown(void)
{
    __real_dvm_namespace_boot_shutdown();
    pthread_mutex_lock(&mutex);
    if (opened <= 3 || opened != closed)
    {
        fprintf(stderr, "FAIL anchor lifetime opened=%lu closed=%lu\n", opened, closed);
        exit(23);
    }
    fprintf(stderr, "ANCHORLIFETIME opened=%lu closed=%lu live=0\n", opened, closed);
    pthread_mutex_unlock(&mutex);
}
