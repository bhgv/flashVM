/* Copyright (c) 2026 bhgv
 * EN: Native supervisor and frozen published-continuation debugging interfaces.
 * RU: Интерфейсы native supervisor и отладки frozen published continuation.
 */
#ifndef DVM_DEBUGGER_H
#define DVM_DEBUGGER_H

extern int dvm_debug_enabled; /* Immutable while VM owners exist; no execution-mode change. */
struct Prog;
struct REG;
struct Inst;
struct DVM_DiagnosticLocation;


/* EN/RU: Start private local socket/supervisor before VM startup; zero on refusal.
 * Запустить private local socket/supervisor до VM startup; ноль при отказе. */
int dvm_debug_start(const char *path);


/* EN/RU: Duplicate explicit private pipe FDs >=3; caller retains original FDs.
 * Дублировать explicit private pipe FDs >=3; originals остаются caller. */
int dvm_debug_start_fds(int input, int output);


/* EN/RU: Wake frozen continuations and join supervisor before collector shutdown.
 * Разбудить frozen continuations и завершить supervisor до collector shutdown. */
void dvm_debug_quiesce(void);


/* EN/RU: Free native registry only after all VM owners join; no managed references.
 * Удалить native registry только после join всех VM owners; без managed references. */
void dvm_debug_destroy(void);


/* EN/RU: Bind this permanent owner to pid; zero on native allocation failure.
 * Привязать permanent owner к pid; ноль при native allocation failure. */
int dvm_debug_bind(int pid);


/* EN/RU: Withdraw this owner's card before its process storage disappears.
 * Снять карточку своего owner до удаления памяти процесса. */
void dvm_debug_retire(void);


/* EN: Copy current owner instruction/REG/display location while ACTIVE. No managed
 * reference escapes. The opcode is sampled before execution, never PC-1 guessed.
 * RU: Копировать instruction/REG/display location своего owner в ACTIVE.
 * Managed references не выходят. Opcode снимается до исполнения, без PC-1 guess. */
void dvm_debug_instruction(const struct REG *registers,
                           const struct Inst *instruction,
                           const struct DVM_DiagnosticLocation *location);


/* EN/RU: Set original fault text before handler/unwind; force retained debug stop.
 * Сохранить original fault text до handler/unwind; запросить retained debug stop. */
void dvm_debug_fault(const char *message);


/* EN/RU: Mark the instruction as executing; only boundary stops permit step.
 * Отметить instruction execution; step разрешён только на boundary stop. */
void dvm_debug_instruction_enter(void);


/* EN/RU: Record an unreleasable fault honestly, without claiming retained memory.
 * Отметить unreleasable fault без ложного обещания retained memory. */
void dvm_debug_stop_refused(void);


/* EN/RU: Record continued unhandled guest errors without stopping unrelated guests.
 * Учесть continued unhandled guest errors без остановки остальных guests. */
void dvm_debug_unhandled(void);
int dvm_debug_exit_status(void);


/* EN/RU: Copy flags for this owner; pause/terminate are asynchronous native commands.
 * Читать flags своего owner; pause/terminate — асинхронные native команды. */
int dvm_debug_requested(void);
int dvm_debug_terminated(void);


/* [GC:] EN: Owner publishes registers only after NATIVE release. Its live C
 * continuation and process root keep storage alive until the matching gate.
 * RU: Owner публикует registers только после NATIVE release. Live C continuation
 * и process root удерживают память до matching gate. */
void dvm_debug_native_publish(struct Prog *process, const struct REG *registers);


/* [GC:] EN: Block NATIVE->ACTIVE while paused; release no source roots and reload
 * no registers. The supervisor may inspect the frozen publication under its lock.
 * RU: Задержать NATIVE->ACTIVE при pause; не снимать roots и не загружать REG.
 * Supervisor может исследовать frozen publication под её lock. */
void dvm_debug_native_gate(struct Prog *process);
#endif
