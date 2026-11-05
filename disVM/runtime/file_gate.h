/* Native per-descriptor FIFO admission for files/directories; never stores or follows managed pointers. */
#ifndef DVM_FILE_GATE_H
#define DVM_FILE_GATE_H
#include <stddef.h>

typedef struct DVM_FileGate DVM_FileGate; /* Owned native gate, lifetime follows its file slot. */


/* Allocate an empty native gate before opening a file. Returns NULL on failure.
 * Caller owns the gate; creation/close/statistics run under runtime heap exclusion. */
DVM_FileGate *dvm_file_gate_create(void);


/* Caller-owned record, kept live from reservation through admission and
 * completion or cancellation. Fields are private to the gate. */
typedef struct DVM_FileTicket
{
    DVM_FileGate *gate; /* Owning gate while queued. */
    struct DVM_FileTicket *previous; /* Earlier ticket, or NULL at the head. */
    struct DVM_FileTicket *next; /* Later ticket, or NULL at the tail. */
} DVM_FileTicket;


/* Append a ticket while ACTIVE, preserving dispatch order. NULL is stdio. */
void dvm_file_gate_reserve(DVM_FileGate *gate, DVM_FileTicket *ticket);


/* Remove a queued ticket, including a non-head ticket. Return zero for an
 * admitted, completed, or foreign ticket. No heap ownership is needed. */
int dvm_file_gate_cancel(DVM_FileGate *gate, DVM_FileTicket *ticket);


/* Reserve, release the heap if allowed, and admit in FIFO order.
 * Return -1 on resource refusal or -2 on cancellation, with the heap restored. */
int dvm_file_gate_try_release(DVM_FileGate *gate, DVM_FileTicket *ticket);


/* Wait outside heap ownership for admission. Return zero for cancellation
 * or invalid ownership. The mutex is released before I/O. */
int dvm_file_gate_enter(DVM_FileGate *gate, DVM_FileTicket *ticket);


/* Admit without waiting. A contended reservation is removed atomically.
 * Requires a bound owner. Return one on admission or zero on refusal. */
int dvm_file_gate_enter_now(DVM_FileGate *gate, DVM_FileTicket *ticket);


/* Complete the admitted ticket before heap reentry. Return zero for an
 * invalid ticket or owner. NULL gate succeeds for stdio. */
int dvm_file_gate_leave(DVM_FileGate *gate, DVM_FileTicket *ticket);


/* Destroy only an empty gate after its file becomes unreachable. Return zero
 * if any operation remains reserved/admitted; storage stays live on refusal.
 * Requires runtime heap exclusion and no future borrowers. NULL succeeds. */
int dvm_file_gate_destroy(DVM_FileGate *gate);


/* Copy native queue state under its mutex. Optional output pointers are borrowed;
 * pending includes the admitted operation, waiting counts contended admissions.
 * Safe during native work; never traverse a managed object. NULL returns zeros. */
void dvm_file_gate_snapshot(DVM_FileGate *gate, size_t *pending, size_t *waiting);


/* Report native gate lifetime/operation totals after managed workers join.
 * Requires heap exclusion; takes only short native mutexes, no OS I/O waits. */
void dvm_file_gate_statistics(void);


/* Borrow the descriptor's gate under heap exclusion, while its FD is rooted.
 * NULL rejects stdio/invalid slots; open directories have their own gate. No ownership transfer occurs. */
DVM_FileGate *dvm_runtime_file_gate(int descriptor);
#endif
