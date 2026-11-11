/* Copyright (c) 2026 bhgv
 * EN: Growing native PID FIFO for pending host cancellation requests.
 * RU: Растущая нативная FIFO идентификаторов запросов отмены от хоста.
 */
#ifndef DVM_HOST_CANCELLATIONS_H
#define DVM_HOST_CANCELLATIONS_H
#include <stddef.h>


/* EN: Zero-initialize before use. The caller's ledger mutex protects all fields.
 * RU: Инициализировать нулями. Все поля защищены мьютексом журнала вызывающего. */
typedef struct DVM_HostCancellations
{
    int *pids;        /* Owned native FIFO storage. */
    size_t count;     /* Pending unique positive process IDs. */
    size_t capacity;  /* Allocated slots, independent of request admission. */
} DVM_HostCancellations;


/* EN: Find pid, returning count if absent. Queue is borrowed and locked by caller.
 * RU: Найти pid, вернуть count при отсутствии. Очередь заимствована и защищена. */
size_t dvm_host_cancellations_find(const DVM_HostCancellations *queue, int pid);


/* EN: Accept positive pid once, growing storage as needed. Caller holds its lock.
 * OOM terminates with a diagnostic. Existing requests keep their FIFO order.
 * RU: Принять положительный pid без повторов с ростом памяти под блокировкой.
 * При нехватке памяти завершить с диагностикой. Порядок запросов сохраняется. */
void dvm_host_cancellations_post(DVM_HostCancellations *queue, int pid);


/* EN: Remove a valid index under caller's lock, preserving order and shrinking.
 * Invalid index terminates with a diagnostic. Queue retains remaining IDs.
 * RU: Удалить допустимый индекс под блокировкой с сохранением порядка и сжатием.
 * Неверный индекс завершает работу с диагностикой. */
void dvm_host_cancellations_remove(DVM_HostCancellations *queue, size_t index);


/* EN: Cancel pending work and release all native storage under caller's lock.
 * RU: Отменить оставшиеся запросы и освободить память под блокировкой вызывающего. */
void dvm_host_cancellations_clear(DVM_HostCancellations *queue);

#endif
