/* Private unpublished native handle construction; no managed values cross it. */
#ifndef DVM_OPEN_WAIT_H
#define DVM_OPEN_WAIT_H
#include "file_port.h"
#include "filesystem_port.h"


/* Snapshot borrowed UTF-8 path before optionally releasing the active heap.
 * mode is an immutable native r/w/wx literal. file/directory are native caller
 * cells receiving one owned handle or NULL. No descriptor-table access occurs
 * outside the heap. Return a port status; caller publishes or closes the handle.
 * Allocation failure performs no filesystem operation. No managed allocation. */
FVM_Result dvm_open_wait(const char *path, const char *mode, int asynchronous,
    FVM_PortFile **file, FVM_PortDirectory **directory);


/* Emit aggregate open attempts/releases and staging after all owners join.
 * No parameters/return, allocation or managed access; requires heap exclusion. */
void dvm_open_statistics(void);
#endif
