/* Sole-VM-owner cooperative boundaries; no raw pointer crosses a VM heap. */
#ifndef DVM_TABLE_LEASES_H
#define DVM_TABLE_LEASES_H


/* Before native MCALL publication, admit a typed table call or park/retry it.
 * function/frame are borrowed validated native call metadata. Returns 1 to run,
 * 0 after removing the process from ready and rewinding its bytecode PC.
 * Only the bound executor calls this API; no native payload wait occurs here. */
int dvm_bigtable_call_ready(void (*function)(void *), void *frame);


/* Abort all leases/waits of a live process before exception unwinding or exit.
 * process is borrowed from the scheduler root list. Releases pinned roots only
 * after guard leave, wakes contenders, and never forcibly cancels foreign work. */
void dvm_bigtable_abort(Prog *process);
#endif
