/* Copyright (c) 2026 bhgv
 * EN: Host-only FD FIFO admission retaining cancellation through provider data I/O.
 * RU: Host-only FD FIFO admission сохраняет cancellation во время provider data I/O.
 */
#ifndef DVM_VFS_NATIVE_WAIT_H
#define DVM_VFS_NATIVE_WAIT_H
#include "file_gate.h"

typedef struct DVM_VfsWait DVM_VfsWait; /* Owned native wait record, no managed pointers. */


/* EN/RU: Caller is ACTIVE with rooted FD/gate/ticket. Reserve FIFO, register copied
 * PID and cancel scope, then leave native admission. Return release>=0 and owned
 * wait, or -1/-2 with heap restored and NULL wait. Only interruptible data handles.
 * Caller ACTIVE, FD/gate/ticket rooted; reserve FIFO, copied PID/cancel scope;
 * release>=0 и свой wait либо -1/-2, heap восстановлен/wait NULL. */
int dvm_vfs_gate_enter(DVM_FileGate *gate, DVM_FileTicket *ticket, DVM_VfsWait **wait);


/* EN/RU: Finish after actual I/O and FIFO leave, before managed reentry; consume wait.
 * Return1 if cancellation/shutdown won before unlink,0 otherwise/NULL. Preserve
 * acknowledged effects; caller must not start another readn chunk after cancellation.
 * Завершить после I/O и FIFO leave, до managed reentry; поглотить wait;
 * сохранить acknowledged effects при гонке; вернуть1 при cancellation,0 иначе/NULL. */
int dvm_vfs_wait_end(DVM_VfsWait *wait);
#endif
