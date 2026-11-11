/* Native handle retirement; no VM roots, descriptor slots or callbacks in worker. */
#ifndef DVM_CLOSE_WORKER_H
#define DVM_CLOSE_WORKER_H
#include "file_port.h"
#include "filesystem_port.h"

typedef struct DVM_CloseJob DVM_CloseJob; /* Owned native reservation prepared before open. */


/* Start one native closer before admitting managed work. No arguments/VM access.
 * Return zero on allocation/thread failure after releasing partial native state. */
int dvm_close_worker_start(void);


/* Allocate a native retirement reservation before destructive file construction.
 * Return owned job or NULL; the caller discards it on failed construction. */
DVM_CloseJob *dvm_close_prepare(void);


/* Free an unused reservation (NULL permitted). Caller owns job; no handles were
 * attached, no allocation/blocking/managed access. Return nothing. */
void dvm_close_discard(DVM_CloseJob *job);


/* Transfer job and exclusively owned handles after FD pins/gate users are gone.
 * The table must already have detached them. No allocation or managed access.
 * Worker mode queues without waiting for filesystem I/O; inactive/reference mode
 * closes inline. NULL handles are allowed. job must be an unused reservation. */
void dvm_close_submit(DVM_CloseJob *job, FVM_PortFile *file, FVM_PortDirectory *directory);


/* After all producers/collectors stop, drain native jobs and join. No heap access
 * is needed; join can wait for genuine native close. No forced I/O cancellation. */
void dvm_close_worker_stop(void);


/* EN: Print a native snapshot, normally after stop (pending must then be zero).
 * Caller guarantees service start/stop do not overlap this call; producers may.
 * No arguments/result or heap access.
 * RU: Вывести снимок, обычно после stop (тогда pending обязан быть нулём).
 * Вызов не совмещается со start/stop службы; производители могут работать.
 * Без параметров/результата и доступа к куче. */
void dvm_close_statistics(void);
#endif
