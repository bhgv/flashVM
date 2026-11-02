/* Internal formatted output diagnostics; no managed/native ABI is exported. */
#ifndef DVM_FORMATTED_SYS_H
#define DVM_FORMATTED_SYS_H

/* Report owner-private completed calls, released waits and remaining FD roots.
 * Call only after managed return/cleanup, never from a UI or collector thread. */
void dvm_formatted_statistics(void);
#endif
