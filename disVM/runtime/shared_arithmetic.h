/* Hosted three-span integer arithmetic, no managed ownership transfer. */
#ifndef DVM_SHARED_ARITHMETIC_H
#define DVM_SHARED_ARITHMETIC_H
#include <stddef.h>

extern int dvm_runtime_shared_integer;
extern int dvm_runtime_guest_unbounded;
extern unsigned long dvm_runtime_shared_extended_steps;
extern unsigned long dvm_runtime_shared_integer_commits;
extern unsigned long dvm_runtime_shared_integer_batches;
extern unsigned long dvm_runtime_shared_calls;
extern unsigned long dvm_runtime_shared_returns;
extern unsigned long dvm_runtime_shared_module_calls;
extern unsigned long dvm_runtime_shared_module_returns;
extern unsigned long dvm_runtime_shared_integer_branches;
extern unsigned long dvm_runtime_shared_byte_steps;
extern unsigned long dvm_runtime_shared_long_steps;
struct Module;
struct Inst;
int dvm_runtime_integer_batch(const struct Module *module, const struct Inst *pc, int index);
extern int dvm_runtime_parallel_add; /* Startup-only frame-local integer experiment. */


/* [GC:] opcode selects ADD/SUB/MUL/DIV/MOD/AND/OR/XOR B/W/L. first/second/output are borrowed
 * live cells. Exact ACTIVE worker context and valid nonempty geometry required.
 * Return NULL after publication or static error before effects. One ordered union
 * guards capture/evaluation/publication. DIV/MOD preserve zero-divisor priority
 * over invalid dividend/output geometry, with errors returned only after unlock. No mapping/root proof or VM callback. */
const char *dvm_runtime_arithmetic(int opcode, const void *first, const void *second, void *output);


/* Same borrowed operands and diagnostic as the opcode adapter. retryable is
 * required caller-private storage. It receives one only if a BUSY claim
 * rolled back without reading operands or publishing the result. */
const char *dvm_runtime_arithmetic_attempt(int opcode, const void *first, const void *second,
    void *output, int *retryable);


/* Disarm the current dispatch. process borrows the active runtime Prog.
 * Return one on pre-effect retry; otherwise clear its consecutive count. */
struct Prog;
int dvm_runtime_arithmetic_finish(struct Prog *process);
/* Verify one pointer-mapped cell of the current rooted frame under ACTIVE. */
struct Type;
struct REG;
int dvm_runtime_rooted_record(const struct REG *registers, const void *cell,
    const struct Type *record);
int dvm_runtime_frame_record_for(const struct REG *registers, const void *cell,
    const struct Type *record);
int dvm_runtime_frame_record(const void *cell, const struct Type *record);
/* EN: Verify an instruction or scalar operand of the current decoded opcode.
 * RU: Проверить инструкцию или скалярный операнд текущего опкода. */
const struct Inst *dvm_runtime_decoded_instruction(int opcode);
int dvm_runtime_shared_integer_operand(int opcode, const void *cell, size_t width);
int dvm_runtime_rooted_scalar(const void *cell, size_t size);
int dvm_runtime_private_copy_count(const void *cell);
/* EN: Test a prepared frame's private return slot.
 * RU: Проверить приватную ячейку результата подготовленного кадра. */
int dvm_runtime_prepared_return_slot(const void *cell);
int dvm_runtime_module_pointer(const void *cell);
int dvm_runtime_module_record(const void *cell, const struct Type *record);
int dvm_runtime_frame_pointer(const void *cell);
int dvm_runtime_slice_index_stable(const void *cell);
#endif
