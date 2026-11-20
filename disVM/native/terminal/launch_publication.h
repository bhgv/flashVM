/* Copyright (c) 2026 bhgv
 * EN: Publish an owner command as independent submit and completion file capabilities.
 * RU: Публикация команды владельца отдельными файловыми полномочиями записи и результата.
 */
#ifndef STYX_LAUNCH_PUBLICATION_H
#define STYX_LAUNCH_PUBLICATION_H
#include "launch_process.h"
#include "provider.h"
#include <pthread.h>

typedef struct StyxLaunchPublication
{
    pthread_mutex_t lock;       /* Serializes the one-shot process and cached status. */
    StyxLaunchProcess process;  /* Fixed command configured by the owner. */
} StyxLaunchPublication;


/* EN/RU: Initialize unused caller storage and borrow command configuration through submission.
 * Инициализировать свободное состояние вызывающего, заимствовать настройку команды до запуска. */
int styxlaunchpublicationinit(StyxLaunchPublication *publication, unsigned long tag,
    const char *directory, char *const *argv);


/* EN/RU: Return an owned root factory for submit (0) or completion (1).
 * Вернуть владеющую фабрику корневого файла запроса (0) или результата (1). */
DVM_Provider *styxlaunchpublicationfile(StyxLaunchPublication *publication, int completion);


/* EN/RU: Retire publication after all providers and opens close. Reap any child, possibly blocking.
 * Освободить публикацию после закрытия провайдеров и файлов, дождавшись дочернего процесса. */
int styxlaunchpublicationfinish(StyxLaunchPublication *publication, int *status);

#endif
