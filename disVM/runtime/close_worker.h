/* Copyright (c) 2026 bhgv
 * EN: Detached native handle retirement; shutdown cancellation never retains guest identity.
 * RU: Отделённое native закрытие; отмена при shutdown не удерживает guest identity.
 */
#ifndef DVM_CLOSE_WORKER_H
#define DVM_CLOSE_WORKER_H
#include "file_port.h"
#include "filesystem_port.h"
#include "vfs/handle.h"

typedef struct DVM_CloseJob DVM_CloseJob; /* Owned native reservation prepared before open. */


/* Start one native closer before admitting managed work. No arguments/VM access.
 * Return zero on allocation/thread failure after releasing partial native state.
 * Запустить native closer до managed producers;1/0, cleanup partial native state при отказе. */
int dvm_close_worker_start(void);


/* Allocate a native retirement reservation before destructive file construction.
 * Return owned job or NULL; the caller discards it on failed construction.
 * Создать native job до resource construction; свой job/NULL, discard при отказе. */
DVM_CloseJob *dvm_close_prepare(void);


/* Free an unused reservation (NULL permitted). Caller owns job; no handles were
 * attached, no allocation/blocking/managed access. Return nothing.
 * Поглотить unused reservation/NULL без allocation/blocking/managed access. */
void dvm_close_discard(DVM_CloseJob *job);


/* Transfer job and exclusively owned handles after FD pins/gate users are gone.
 * The table must already have detached them. No allocation or managed access.
 * Worker mode queues without waiting for filesystem I/O; inactive/reference mode
 * closes inline. NULL handles are allowed. job must be an unused reservation.
 * Передать job/exclusive handles после ухода borrowers; publication без allocation/VM,
 * очередь не ждёт I/O, inactive service закрывает inline. */
void dvm_close_submit(DVM_CloseJob *job, FVM_PortFile *file, FVM_PortDirectory *directory);


/* EN/RU: Move exclusive resource into preallocated job after all FD borrowers
 * retire. Clears source; no allocation or managed callback. Worker closes it.
 * Перенести resource в job после ухода FD borrowers. Очистить source; без
 * allocation/managed callback. Worker закрывает handle. Invalid ownership is fatal. */
void dvm_close_submit_resource(DVM_CloseJob *job, DVM_VfsHandle *resource);


/* After all producers/collectors stop, drain native jobs and join. No heap access
 * is needed. Cancel the service source before join, outside the queue mutex.
 * Eligible callbacks consume ownership even when pre-cancelled; legacy close can still block.
 * Без heap access; отменить service source до join вне mutex очереди; legacy close может ждать. */
void dvm_close_worker_stop(void);


/* EN: Print a native snapshot, normally after stop (pending must then be zero).
 * Caller guarantees service start/stop do not overlap this call; producers may.
 * No arguments/result or heap access.
 * RU: Вывести снимок, обычно после stop (тогда pending обязан быть нулём).
 * Вызов не совмещается со start/stop службы; производители могут работать.
 * Без параметров/результата и доступа к куче. */
void dvm_close_statistics(void);
#endif
