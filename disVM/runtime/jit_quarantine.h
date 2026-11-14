/* Copyright (c) 2026 bhgv
 * EN: Per-loaded-code function quarantine, independent of any external debugger.
 * RU: Карантин функций загруженного code, независимо от внешнего debugger.
 */
#ifndef DVM_JIT_QUARANTINE_H
#define DVM_JIT_QUARANTINE_H
struct Module;
struct Jit;
struct REG;


/* [GC:] EN: Select code for borrowed live module/original next-PC/frame. Capture
 * scalar function bounds before any callback; NULL forces ordinary interpreter.
 * RU: Выбрать code живого module/original next-PC/frame. Снять scalar границы
 * функции до callback; NULL направляет в обычный interpreter. */
struct Jit *dvm_jit_select(struct Module *module, const void *next_pc, const void *frame);


/* EN: Record a confirmed backend defect from the current native invocation.
 * cause/opcode are copied diagnostics. Quarantine all subsequent native entries
 * in this function; return1 if recorded. Never replay the current operation.
 * RU: Учесть подтверждённый backend defect текущего native invocation. cause/opcode
 * — copied diagnostics. Карантин следующих entries всей функции; вернуть1 при записи.
 * Текущая операция не повторяется. */
int dvm_jit_report_failure(int opcode, const char *cause);


/* EN/RU: Assign unique loaded-code generation before publication; zero on exhaustion.
 * Назначить unique generation загруженного code до publication; ноль при исчерпании. */
unsigned long dvm_jit_new_generation(void);


/* EN/RU: Bracket one native callback; copied scalar provenance survives VM unwind.
 * Окаймить native callback; copied scalar provenance переживает VM unwind. */
void dvm_jit_native_begin(void);
void dvm_jit_native_end(void);


/* EN: On owner error, quarantine only an explicit compile exception originating
 * in native callback execution. Clear provenance for every error, including
 * ordinary application errors. Return1 when quarantine was recorded.
 * RU: При owner error карантин только explicit compile exception из native
 * callback. Снять provenance для любой ошибки, включая application errors.
 * Вернуть1 при записи карантина. */
int dvm_jit_native_error(const char *message);
#endif
