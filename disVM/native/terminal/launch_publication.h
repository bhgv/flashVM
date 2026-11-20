/* Copyright (c) 2026 bhgv
 * EN: Publish an owner command as independent submit and completion file capabilities.
 * RU: Публикация команды владельца отдельными файловыми полномочиями записи и результата.
 */
#ifndef STYX_LAUNCH_PUBLICATION_H
#define STYX_LAUNCH_PUBLICATION_H
#include "launch_process.h"
#include "provider.h"
#include "flashvm/sorder.h"
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


/* EN/RU: Route a complete first write by the reserved launch tag. Return 1 for
 * admission, -1 for a refused launch, 0 to leave the write to the Unibus stream.
 * Направить полную первую запись по зарезервированному тегу запуска.
 * Вернуть 1 при приёме, -1 при отказе запуска, 0 для обработки потоком Unibus. */
int styxlaunchpublicationdispatch(StyxLaunchPublication *publication, uint64_t offset,
    const void *bytes, size_t size);


/* EN/RU: Encode a cached or reaped exit status as one ordered Unibus record.
 * Return 1 for output, 0 pending, -1 on invalid input, wait or encoding error.
 * Закодировать код выхода в упорядоченную запись Unibus.
 * Вернуть 1 при записи, 0 в ожидании, -1 при ошибке. */
int styxlaunchpublicationresultrecord(StyxLaunchPublication *publication, int wait,
    const FVM_SOrdinal *ordinal, unsigned long tag, void *bytes, size_t capacity, size_t *used);


/* EN/RU: Retire publication after all providers and opens close. Reap any child, possibly blocking.
 * Освободить публикацию после закрытия провайдеров и файлов, дождавшись дочернего процесса. */
int styxlaunchpublicationfinish(StyxLaunchPublication *publication, int *status);

#endif
