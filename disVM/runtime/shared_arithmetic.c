/* Claim all binary integer operands together before capture/publication. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "raise.h"
#include "shared_access.h"
#include "shared_arithmetic.h"
#include "arithmetic_value.h"
#include "managed_owner.h"
#include "disvm/native_process.h"
#include "disvm/native_owner.h"
#include "runqueue.h"
#include "runtime_gc.h"
#include <fenv.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>


int dvm_runtime_shared_integer; /* Dedicated guests can publish under SHARED. */
unsigned long dvm_runtime_shared_integer_batches; /* Completed multi-instruction shared intervals. */
unsigned long dvm_runtime_shared_integer_commits; /* Counted after exclusive return. */
int dvm_runtime_parallel_add; /* Startup-only leased arithmetic selection, 1 for ADDW or 2 for all. */


/* Find the current live frame descriptor within the rooted stack extent.
 * The first frame stores its Type in the extent header. Borrow under ACTIVE. */
static Type *frame_type(const REG *registers)
{
    uintptr_t base = (uintptr_t)registers->EX;
    uintptr_t frame = (uintptr_t)registers->FP;
    uintptr_t top = (uintptr_t)registers->SP;
    Type *type;

    if (!base || registers->EX == H || frame < base || top < frame ||
        (uintptr_t)registers->TS < top || top - frame < sizeof(Frame))
    {
        return NULL;
    }
    type = ((Frame *)frame)->t;
    if (!type && frame - base == offsetof(Stkext, reg.tos.fu))
    {
        type = ((Stkext *)base)->reg.TR;
    }
    if (!type || type == H || type->size < (int)sizeof(Frame) ||
        (size_t)type->size > top - frame || type->np < 0 ||
        (size_t)type->np > ((size_t)type->size / sizeof(void *) + 7) / 8)
    {
        return NULL;
    }
    return type;
}


static Type *current_frame_type(void)
{
    return frame_type(&DVM_R);
}


/* Check a borrowed scalar span against the frame extent and pointer map.
 * The published process stack retains the frame throughout its lease. */
static int scalar_span(const void *cell, const Type *type, size_t width,
    uintptr_t base, size_t minimum)
{
    uintptr_t address = (uintptr_t)cell;
    size_t offset, slot, last;

    if (address < base || width > (size_t)type->size)
    {
        return 0;
    }
    offset = address - base;
    if (offset < minimum || offset > (size_t)type->size - width)
    {
        return 0;
    }
    last = (offset + width - 1) / sizeof(void *);
    for (slot = offset / sizeof(void *); slot <= last; slot++)
    {
        if (slot / 8 < (size_t)type->np && (type->map[slot / 8] & (0x80U >> (slot % 8))))
        {
            return 0;
        }
    }
    return 1;
}


static int frame_scalar(const void *cell, const Type *type, size_t width)
{
    return scalar_span(cell, type, width, (uintptr_t)DVM_R.FP, NREG * IBY2WD);
}


/* The suspended Prog roots its current Modlink. Ordinary Dis links own
 * their fixed heap-backed MP. Dynamic modules can supply a native MP. */
static const Type *module_data_type(const REG *registers)
{
    const Modlink *link = registers->M;
    const Type *type;
    if (!link || link == H || !link->m || link->m == H || link->m->rt == DYNMOD ||
        !registers->MP || registers->MP == H || link->MP != registers->MP)
    {
        return NULL;
    }
    type = D2H(registers->MP)->t;
    if (!type || type == H || type->size < (int)sizeof(void *) || type->np < 0 ||
        (size_t)type->np > ((size_t)type->size / sizeof(void *) + 7) / 8)
    {
        return NULL;
    }
    return type;
}

/* Verify nonempty byte spans in the active frame or the rooted module data. */
int dvm_runtime_rooted_scalar(const void *cell, size_t size)
{
    const Type *type = current_frame_type();
    if (!size)
    {
        return 0;
    }
    if (type && frame_scalar(cell, type, size))
    {
        return 1;
    }
    type = module_data_type(&DVM_R);
    return type && scalar_span(cell, type, size, (uintptr_t)DVM_R.MP, 0);
}


/* A raw move count must be independent of module changes across suspension. */
int dvm_runtime_private_copy_count(const void *cell)
{
    const Type *type = current_frame_type();
    return cell == &DVM_R.t || (type && frame_scalar(cell, type, sizeof(WORD)));
}


static int mapped_pointer(const void *cell, const void *base, const Type *type, size_t minimum)
{
    uintptr_t address = (uintptr_t)cell;
    uintptr_t start = (uintptr_t)base;
    size_t offset, slot;
    if (!type || address < start)
    {
        return 0;
    }
    offset = address - start;
    if (offset < minimum || offset % sizeof(void *) ||
        offset > (size_t)type->size - sizeof(void *))
    {
        return 0;
    }
    slot = offset / sizeof(void *);
    return slot / 8 < (size_t)type->np &&
        (type->map[slot / 8] & (0x80U >> (slot % 8))) != 0;
}

int dvm_runtime_frame_pointer(const void *cell)
{
    return mapped_pointer(cell, DVM_R.FP, current_frame_type(), NREG * IBY2WD);
}

int dvm_runtime_module_pointer(const void *cell)
{
    return mapped_pointer(cell, DVM_R.MP, module_data_type(&DVM_R), 0);
}


/* Admit mixed-width cells of an armed experimental operation.
 * The active dispatch and typed frame supply the borrowed span lifetime. */
static int parallel_frame_spans(int opcode, const void *first, size_t first_size,
    const void *second, size_t second_size, const void *third, size_t third_size)
{
    Type *type;

    if (dvm_native_execution()->arithmetic_retry_opcode != opcode || !up->prog)
    {
        return 0;
    }
    type = current_frame_type();
    return type && frame_scalar(first, type, first_size) &&
        (!second_size || frame_scalar(second, type, second_size)) &&
        (!third_size || frame_scalar(third, type, third_size));
}


/* Verify that the entire record agrees with the live frame's pointer map. */
static int mapped_record(const void *cell, const Type *record, const Type *frame,
    uintptr_t base, size_t minimum)
{
    uintptr_t address = (uintptr_t)cell;
    size_t offset, slot, frame_slot;

    if (!frame || address < base || record->size <= 0 ||
        record->size % sizeof(void *) || record->size > frame->size)
    {
        return 0;
    }
    offset = address - base;
    if (offset < minimum || offset % sizeof(void *) ||
        offset > (size_t)(frame->size - record->size))
    {
        return 0;
    }
    for (slot = 0; slot < (size_t)record->size / sizeof(void *); slot++)
    {
        frame_slot = offset / sizeof(void *) + slot;
        int mapped = slot / 8 < (size_t)record->np &&
            (record->map[slot / 8] & (0x80U >> (slot % 8)));
        int frame_mapped = frame_slot / 8 < (size_t)frame->np &&
            (frame->map[frame_slot / 8] & (0x80U >> (frame_slot % 8)));
        if (mapped != frame_mapped)
        {
            return 0;
        }
    }
    return 1;
}


int dvm_runtime_frame_record_for(const REG *registers, const void *cell, const Type *record)
{
    return mapped_record(cell, record, frame_type(registers),
        (uintptr_t)registers->FP, NREG * IBY2WD);
}

int dvm_runtime_module_record(const void *cell, const Type *record)
{
    return mapped_record(cell, record, module_data_type(&DVM_R), (uintptr_t)DVM_R.MP, 0);
}

int dvm_runtime_frame_record(const void *cell, const Type *record)
{
    return dvm_runtime_frame_record_for(&DVM_R, cell, record);
}


/* Check a record in the frame or heap-backed module rooted by registers.
 * The caller keeps the corresponding guest alive through the transfer. */
int dvm_runtime_rooted_record(const REG *registers, const void *cell, const Type *record)
{
    if (!record->np && record->size > 0 && registers == &DVM_R)
    {
        const Type *frame = frame_type(registers);
        if (frame && frame_scalar(cell, frame, (size_t)record->size))
        {
            return 1;
        }
    }
    return dvm_runtime_frame_record_for(registers, cell, record) ||
        mapped_record(cell, record, module_data_type(registers), (uintptr_t)registers->MP, 0);
}


/* A slice index must belong to the armed instruction or its private frame. */
int dvm_runtime_slice_index_stable(const void *cell)
{
    return up->prog && dvm_native_execution()->arithmetic_retry_opcode == ISLICELA &&
        (cell == &DVM_R.t ||
            parallel_frame_spans(ISLICELA, cell, sizeof(WORD), NULL, 0, NULL, 0));
}


/* Gate the shared frame check on the full integer experiment. */
int dvm_runtime_parallel_scalar_spans(int opcode, const void *first, size_t first_size,
    const void *second, size_t second_size, const void *third, size_t third_size)
{
    return dvm_runtime_parallel_add == 2 &&
        parallel_frame_spans(opcode, first, first_size, second, second_size, third, third_size);
}


int dvm_runtime_parallel_real; /* Startup-only flag for private FP evaluation. */


/* Preserve this guest's FP state across native lease admission. */
int dvm_runtime_parallel_real_leave(int opcode, const void *first, size_t first_size,
    const void *second, size_t second_size, const void *third, size_t third_size)
{
    fenv_t environment;
    int released;

    if (!dvm_runtime_parallel_real ||
        !parallel_frame_spans(opcode, first, first_size, second, second_size, third, third_size))
    {
        return 0;
    }
    if (fegetenv(&environment))
    {
        return 0;
    }
    released = dvm_runtime_gc_native_try_leave();
    if (fesetenv(&environment))
    {
        abort();
    }
    return released > 0;
}


/* Preserve FP flags produced on private bytes through native reentry. */
void dvm_runtime_parallel_real_enter(int released)
{
    if (released)
    {
        fenv_t environment;

        if (fegetenv(&environment))
        {
            abort();
        }
        dvm_runtime_gc_native_enter(released);
        if (fesetenv(&environment))
        {
            abort();
        }
    }
}


/* Revalidate the private snapshots under the new union. All data is
 * borrowed through the active guest lease. No errors are thrown here. */
static const char *commit_arithmetic(int opcode, const void *first, const void *second,
    void *output, size_t width, int division, const unsigned char *left,
    const unsigned char *right, const unsigned char *result)
{
    if (memcmp(first, left, width) || memcmp(second, right, width))
    {
        if (division && dvm_runtime_divisor_zero(opcode, second))
        {
            return exZdiv;
        }
        dvm_runtime_arithmetic_value(opcode, first, second, output);
    }
    else
    {
        memmove(output, result, width);
    }
    return NULL;
}


/* Compute from private bytes, then revalidate and publish. Dedicated guests
 * can do both in a shared interval. Error delivery remains exclusive. */
static const char *leased_arithmetic(int opcode, const void *first, const void *second,
    void *output, size_t width, int division, DVM_TripleAccess *access, int *retryable)
{
    unsigned char left[sizeof(ULONG)], right[sizeof(ULONG)], result[sizeof(ULONG)];
    DVM_CellDomain *domain;
    DVM_HeapOwner *owner = dvm_native_owner_current();
    int released;
    DVM_CellResult claim;
    const char *problem;

    memmove(left, first, width);
    memmove(right, second, width);
    if (!dvm_small_range_access_end(access))
    {
        abort();
    }
    domain = dvm_runtime_shared_integer ? dvm_managed_cell_domain() : NULL;
    released = dvm_runtime_gc_native_try_leave();
    if (released > 0 && domain)
    {
        if (!dvm_native_owner_enter_shared(owner))
        {
            abort();
        }
        dvm_runtime_arithmetic_value(opcode, left, right, result);
        claim = dvm_small_range_access_begin(domain, first, width, second, width,
            output, width, access);
        problem = NULL;
        if (claim == DVM_CELL_ACQUIRED)
        {
            problem = commit_arithmetic(opcode, first, second, output, width, division, left, right, result);
            if (!dvm_small_range_access_end(access))
            {
                abort();
            }
        }
        if (!dvm_native_owner_leave_shared(owner))
        {
            abort();
        }
        dvm_runtime_gc_native_enter(released);
        if (claim == DVM_CELL_ACQUIRED)
        {
            if (!problem)
            {
                dvm_runtime_shared_integer_commits++;
            }
            return problem;
        }
    }
    else
    {
        dvm_runtime_arithmetic_value(opcode, left, right, result);
        if (released > 0)
        {
            dvm_runtime_gc_native_enter(released);
        }
    }

    claim = dvm_managed_small_three_begin(first, width, second, width, output, width, access);
    if (claim != DVM_CELL_ACQUIRED)
    {
        *retryable = claim == DVM_CELL_BUSY;
        return exHeap;
    }
    problem = commit_arithmetic(opcode, first, second, output, width, division, left, right, result);
    if (!dvm_small_range_access_end(access))
    {
        abort();
    }
    return problem;
}


typedef struct IntegerStep
{
    int opcode;                  /* Audited, nonthrowing WORD opcode. */
    const void *source;          /* Borrowed frame scalar or rooted instruction literal. */
    const void *middle;          /* Borrowed frame scalar or copied immediate. */
    WORD immediate;              /* Decoded signed 16-bit middle value. */
    int immediate_middle;        /* Restore the retained REG.t convention. */
    void *destination;           /* Borrowed scalar in the suspended frame. */
} IntegerStep;


/* Capture up to sixteen consecutive private WORD operations. Unknown
 * addressing, opcodes and module boundaries end the candidate prefix. */
int dvm_runtime_integer_batch(const Module *module, const Inst *pc, int index)
{
    IntegerStep steps[16];
    Type *frame;
    DVM_CellDomain *domain;
    DVM_HeapOwner *owner;
    int count = 0, completed = 0, released;
    if (!dvm_runtime_shared_integer || dvm_runtime_parallel_add != 2 ||
        module->cstep || DVM_R.IC < 2 || !(frame = current_frame_type()))
    {
        return 0;
    }
    while (count < 16 && count < DVM_R.IC && count < module->nprog - index)
    {
        Inst instruction;
        IntegerStep *step = &steps[count];
        memcpy(&instruction, pc + count, sizeof(instruction));
        int source_mode = USRC(instruction.add);
        int middle_mode = instruction.add & ARM;
        if (UDST(instruction.add) != AFP || (source_mode != AFP && source_mode != AIMM) ||
            middle_mode == AXINM ||
            (instruction.op != IADDW && instruction.op != ISUBW && instruction.op != IMULW &&
             instruction.op != IANDW && instruction.op != IORW && instruction.op != IXORW) ||
            (source_mode == AFP && (instruction.s.ind < 0 || instruction.s.ind >= frame->size)) ||
            instruction.d.ind < 0 || instruction.d.ind >= frame->size ||
            (middle_mode == AXINF && instruction.reg >= frame->size))
        {
            break;
        }
        step->opcode = instruction.op;
        step->source = source_mode == AFP ? (const void *)(DVM_R.FP + instruction.s.ind) :
            (const void *)&pc[count].s;
        step->destination = DVM_R.FP + instruction.d.ind;
        step->immediate_middle = middle_mode == AXIMM;
        if (step->immediate_middle)
        {
            step->immediate = instruction.reg < 32768 ? instruction.reg : (int)instruction.reg - 65536;
            step->middle = &step->immediate;
        }
        else
        {
            step->middle = middle_mode == AXINF ? DVM_R.FP + instruction.reg : step->destination;
        }
        if ((source_mode == AFP && !frame_scalar(step->source, frame, sizeof(WORD))) ||
            (!step->immediate_middle && !frame_scalar(step->middle, frame, sizeof(WORD))) ||
            !frame_scalar(step->destination, frame, sizeof(WORD)))
        {
            break;
        }
        count++;
    }
    if (count < 2 || !(domain = dvm_managed_cell_domain()))
    {
        return 0;
    }
    owner = dvm_native_owner_current();
    released = dvm_runtime_gc_native_try_leave();
    if (released <= 0)
    {
        return 0;
    }
    if (!dvm_native_owner_enter_shared(owner))
    {
        abort();
    }
    while (completed < count)
    {
        DVM_TripleAccess access = {0};
        IntegerStep *step = &steps[completed];
        if (dvm_small_range_access_begin(domain, step->source, sizeof(WORD),
            step->middle, sizeof(WORD), step->destination, sizeof(WORD), &access) != DVM_CELL_ACQUIRED)
        {
            break;
        }
        dvm_runtime_arithmetic_value(step->opcode, step->source, step->middle, step->destination);
        if (!dvm_small_range_access_end(&access))
        {
            abort();
        }
        completed++;
    }
    if (!dvm_native_owner_leave_shared(owner))
    {
        abort();
    }
    dvm_runtime_gc_native_enter(released);
    if (completed)
    {
        IntegerStep *last = &steps[completed - 1];
        int step_index;
        for (step_index = completed - 1; step_index >= 0; step_index--)
        {
            if (steps[step_index].immediate_middle)
            {
                DVM_R.t = steps[step_index].immediate;
                break;
            }
        }
        DVM_R.s = (void *)last->source;
        DVM_R.m = last->immediate_middle ? (void *)&DVM_R.t : (void *)last->middle;
        DVM_R.d = last->destination;
        DVM_R.PC += completed;
        DVM_R.IC -= completed;
        dvm_runtime_shared_integer_commits += completed;
        if (completed > 1)
        {
            dvm_runtime_shared_integer_batches++;
        }
    }
    return completed;
}


/* [PERF ARITH-096] Historical generic guard cost remains in the ledger.
 * [PERF SMALL-097] Compact metadata preserves the canonical guard union; matched
 * workload comparison and remaining profile overhead: PERFORMANCE-COSTS.md. */


/* [PERF MULDIV-098] Full guarded product/division workload is recorded in
 * PERFORMANCE-COSTS.md. Divisor checks/error-path cost is not isolated. */


/* [GC:] opcode selects width/operation; first/second/output borrow live storage.
 * Reject context/opcode/geometry before reads. Return static error or NULL.
 * retryable points to caller-private storage and receives one only for a
 * fully rolled-back BUSY claim. Existing NativeWorld/root rules remain. */
const char *dvm_runtime_arithmetic_attempt(int opcode, const void *first, const void *second,
    void *output, int *retryable)
{
    DVM_TripleAccess access = {0};
    DVM_CellResult claim;
    size_t width;
    int division = 0;
    int invalid_outer;
    const char *problem = NULL;
    *retryable = 0;
    if (dvm_runtime_shared_mode() != 2)
    {
        return exHeap;
    }
    switch (opcode)
    {
    case IADDB:
    case ISUBB:
    case IANDB:
    case IORB:
    case IMULB:
    case IDIVB:
    case IMODB:
    case IXORB:
        width = sizeof(BYTE);
        break;
    case IADDW:
    case ISUBW:
    case IANDW:
    case IORW:
    case IMULW:
    case IDIVW:
    case IMODW:
    case IXORW:
        width = sizeof(UWORD);
        break;
    case IADDL:
    case ISUBL:
    case IANDL:
    case IORL:
    case IMULL:
    case IDIVL:
    case IMODL:
    case IXORL:
        width = sizeof(ULONG);
        break;
    default:
        return exHeap;
    }
    division = opcode == IDIVB || opcode == IDIVW || opcode == IDIVL ||
        opcode == IMODB || opcode == IMODW || opcode == IMODL;
    if (!second || second == H || (uintptr_t)second > (uintptr_t)-1 - width)
    {
        return exBounds;
    }
    invalid_outer = !output || output == H || (uintptr_t)output > (uintptr_t)-1 - width ||
        !first || first == H || (uintptr_t)first > (uintptr_t)-1 - width;
    if (invalid_outer && !division)
    {
        return exBounds;
    }
    /* Invalid dividend/output geometry is not payload access. For DIV/MOD,
     * claim only the valid divisor (three aliases) before choosing Zdiv/Bounds.
     * Valid operations keep all three spans locked through their single snapshot. */
    claim = dvm_managed_small_three_begin(invalid_outer ? second : first, width,
        second, width, invalid_outer ? (void *)second : output, width, &access);
    if (claim != DVM_CELL_ACQUIRED)
    {
        *retryable = claim == DVM_CELL_BUSY;
        return exHeap;
    }
    if (division && dvm_runtime_divisor_zero(opcode, second))
    {
        problem = exZdiv;
    }
    else if (invalid_outer)
    {
        problem = exBounds;
    }
    else
    {
        if (dvm_runtime_parallel_add && (opcode == IADDW || dvm_runtime_parallel_add == 2) &&
            dvm_native_execution()->arithmetic_retry_opcode == opcode && up->prog)
        {
            Type *type = current_frame_type();
            if (type && frame_scalar(first, type, width) && frame_scalar(second, type, width) &&
                frame_scalar(output, type, width))
            {
                return leased_arithmetic(opcode, first, second, output, width, division, &access, retryable);
            }
        }
        dvm_runtime_arithmetic_value(opcode, first, second, output);
    }
    if (!dvm_small_range_access_end(&access))
    {
        abort();
    }
    return problem;
}


/* Request a bounded retry for the armed opcode. NativeWorld still protects
 * the current process, whose retry count follows it across OS turns. */
int dvm_runtime_scalar_retry(int opcode)
{
    DVM_NativeExecution *execution = dvm_native_execution();
    DVM_RuntimeProcess *process;

    if (opcode <= 0 || execution->arithmetic_retry_opcode != opcode)
    {
        return 0;
    }
    process = (DVM_RuntimeProcess *)up->prog;
    if (!process || process->arithmetic_retries >= 8)
    {
        return 0;
    }
    process->arithmetic_retries++;
    execution->arithmetic_retry_opcode = -1;
    return 1;
}


/* Adapt the result for C/JIT. Operands borrow the attempt's spans. Report
 * the original diagnostic when retry cannot be scheduled. */
const char *dvm_runtime_arithmetic(int opcode, const void *first, const void *second, void *output)
{
    int retryable;
    const char *problem = dvm_runtime_arithmetic_attempt(opcode, first, second, output, &retryable);

    return retryable && dvm_runtime_scalar_retry(opcode) ? NULL : problem;
}


/* Disarm the current dispatch, preserving the retry count only on yield.
 * process borrows a live runtime Prog under NativeWorld. No allocation. */
int dvm_runtime_arithmetic_finish(Prog *process)
{
    DVM_NativeExecution *execution = dvm_native_execution();
    int retry = execution->arithmetic_retry_opcode == -1;

    execution->arithmetic_retry_opcode = 0;
    if (!retry)
    {
        ((DVM_RuntimeProcess *)process)->arithmetic_retries = 0;
    }
    return retry;
}
