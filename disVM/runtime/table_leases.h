/* Copyright (c) 2026 bhgv
 * EN: Per-table native admission, cooperative FIFO and retained lease cleanup.
 * RU: Потабличный нативный допуск, кооперативная FIFO и снятие retained lease.
 */
#ifndef DVM_TABLE_LEASES_H
#define DVM_TABLE_LEASES_H

#define DVM_TABLE_WAIT (Pbroken + 1) /* Hosted table wait state. */

/* Publish an MCALL cancellation for application by its register owner. */
int dvm_runtime_table_retry_begin(Prog *process);
void dvm_runtime_table_retry_pending(Prog *process, int pending);
void dvm_runtime_table_cancel_pc(Prog *process);


/* Before native MCALL publication, admit a typed table call or park/retry it.
 * function/frame are borrowed validated native call metadata. Returns 1 to run,
 * 0 after removing the process from ready and rewinding its bytecode PC.
 * Only the bound executor calls this API; no native payload wait occurs here.
 * A primitive reserves one implicit level of each distinct table before return.
 * Its table_apply removes those levels; an exception uses the ordinary actor abort.
 * RU: Операция занимает по одному неявному уровню различных таблиц до возврата.
 * table_apply снимает эти уровни; исключение использует обычный abort владельца. */
int dvm_bigtable_call_ready(void (*function)(void *), void *frame);


/* Remove a table wait and make the guest runnable for its exit turn.
 * Return zero for other states. Scheduling metadata uses its native mutex. */
int dvm_bigtable_wake_for_exit(Prog *process);


/* Cancel a parked or granted table retry. The next dispatch delivers a
 * normal exception at MCALL. Scheduling metadata uses its native mutex. */
int dvm_bigtable_cancel_wait(Prog *process);


/* Abort all leases/waits of a live process before exception unwinding or exit.
 * process is borrowed from the scheduler root list. Releases pinned roots only
 * after guard leave, wakes contenders, and never forcibly cancels foreign work. */
void dvm_bigtable_abort(Prog *process);
#endif
