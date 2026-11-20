/* Copyright (c) 2026 bhgv
 * EN: Host-selected local process behind a one-shot Styx launch route.
 * RU: Выбранный хостом локальный процесс за одноразовым маршрутом Styx.
 */
#ifndef STYX_TERMINAL_LAUNCH_PROCESS_H
#define STYX_TERMINAL_LAUNCH_PROCESS_H
#include "launch_receive.h"
#include <sys/types.h>


/* EN/RU: Caller-owned process and one-shot receiver; serialize operations.
 * Процесс и приёмник принадлежат вызывающему; операции последовательны. */
typedef struct StyxLaunchProcess
{
    StyxLaunchReceiver receiver; /* Embedded request admission. */
    const char *directory;      /* Borrowed working directory. */
    char *const *argv;          /* Borrowed argv with absolute executable at index zero. */
    pid_t child;               /* Child awaiting reap, or zero. */
    int completed;             /* Cached completion state. */
    int status;                /* Exit code, 128+signal, or -1 on wait failure. */
} StyxLaunchProcess;


/* EN/RU: Initialize unused storage. Host pins directory/argv until submission,
 * marks private host descriptors close-on-exec, and reaps before retirement.
 * Инициализировать неиспользуемое состояние. Хост удерживает directory/argv
 * до запроса, помечает частные FD close-on-exec и дожидается завершения. */
int styxlaunchprocessinit(StyxLaunchProcess *process, unsigned long tag,
    const char *directory, char *const *argv);


/* EN/RU: Opt in before publication; the DisVM executable will receive up to limit bytes, max 32768.
 * Включить до публикации; исполняемый DisVM получит до limit байт, максимум 32768. */
int styxlaunchprocesspayload(StyxLaunchProcess *process, size_t limit);


/* EN/RU: Observe cached status or reap the owned child. Return 1 with status,
 * 0 while pending (or unstarted), -1 on observation failure. wait blocks if nonzero.
 * Получить сохранённый статус или дождаться своего процесса. Вернуть 1 со
 * статусом, 0 если результата ещё нет, -1 при ошибке. wait включает ожидание. */
int styxlaunchprocessstatus(StyxLaunchProcess *process, int wait, int *status);

#endif
