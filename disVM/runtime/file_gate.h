/* Native per-file FIFO admission; never stores or follows managed pointers. */
#ifndef DVM_FILE_GATE_H
#define DVM_FILE_GATE_H
#include <stddef.h>

typedef struct DVM_FileGate DVM_FileGate; /* Owned native gate, lifetime follows its file slot. */


/* Allocate an empty native gate before opening a file. Returns NULL on failure.
 * Caller owns the gate; creation/close/statistics run under runtime heap exclusion. */
DVM_FileGate *dvm_file_gate_create(void);


/* Reserve the next operation under ACTIVE heap ownership, before native release.
 * gate is borrowed through the operation by a rooted FD or private loader slot.
 * Returns a native FIFO ticket. NULL denotes stdio/no gate and returns zero. */
size_t dvm_file_gate_reserve(DVM_FileGate *gate);


/* Wait outside heap ownership for the reserved ticket, then admit one native
 * operation. gate/ticket must match reserve; NULL is a no-op. No managed access,
 * allocation or throwing occurs. Return zero without a bound native context,
 * one after admission. The internal mutex is NOT held during I/O. */
int dvm_file_gate_enter(DVM_FileGate *gate, size_t ticket);


/* Complete exactly the admitted operation BEFORE managed reentry. Return one
 * on success (also for NULL); zero refuses wrong ownership without advancing.
 * Only the admitted caller may finish; no managed access/allocation/throw. */
int dvm_file_gate_leave(DVM_FileGate *gate, size_t ticket);


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
 * NULL rejects stdio/directory/invalid slots. No ownership transfer occurs. */
DVM_FileGate *dvm_runtime_file_gate(int descriptor);
#endif
