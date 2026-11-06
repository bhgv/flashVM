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
#include "branch_value.h"
#include "shift_value.h"
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
unsigned long dvm_runtime_shared_byte_steps; /* Shared BYTE arithmetic/comparisons. */
unsigned long dvm_runtime_shared_long_steps; /* Shared LONG arithmetic/comparisons. */
unsigned long dvm_runtime_shared_integer_branches; /* WORD branches completed inside shared traces. */
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


static int integer_division(int opcode)
{
    return opcode == IDIVB || opcode == IDIVW || opcode == IDIVL ||
        opcode == IMODB || opcode == IMODW || opcode == IMODL;
}


typedef struct IntegerStep
{
    int opcode;                  /* Audited, nonthrowing integer opcode. */
    size_t width;                /* Native value width. */
    size_t source_width;         /* Shift counts always occupy one WORD. */
    int shift;
    int family;                  /* BYTE, WORD or LONG for diagnostics. */
    const void *source;          /* Borrowed frame scalar or rooted instruction literal. */
    const void *middle;          /* Borrowed frame scalar or copied immediate. */
    WORD immediate;              /* Decoded signed 16-bit middle value. */
    int immediate_middle;        /* Restore the retained REG.t convention. */
    void *destination;           /* Frame result or rooted branch literal cell. */
    Inst *target;                /* Verified same-module target for an integer branch. */
} IntegerStep;


/* Check a bytecode address against the retained module's complete Inst span. */
static int batch_pc(const Module *module, const Inst *pc)
{
    uintptr_t address = (uintptr_t)pc;
    uintptr_t base = (uintptr_t)module->prog;
    return address >= base && (address - base) % sizeof(Inst) == 0 &&
        (address - base) / sizeof(Inst) < (size_t)module->nprog;
}


/* Decode only direct scalar operands and same-module integer branch targets.
 * A failed check ends the prefix and leaves the normal decoder authoritative. */
static int decode_integer_step(const Module *module, const Inst *pc, const Type *frame,
    const Type *module_type, IntegerStep *step)
{
    Inst instruction;
    int source_mode, middle_mode, destination_mode, branch = 0;
    if (!batch_pc(module, pc))
    {
        return 0;
    }
    memcpy(&instruction, pc, sizeof(instruction));
    source_mode = USRC(instruction.add);
    middle_mode = instruction.add & ARM;
    destination_mode = UDST(instruction.add);
    step->opcode = instruction.op;
    step->target = NULL;
    switch (instruction.op)
    {
    case IBEQB: case IBNEB: case IBLTB: case IBLEB: case IBGTB: case IBGEB:
        branch = 1;
        /* fall through */
    case IADDB: case ISUBB: case IMULB: case IANDB: case IORB: case IXORB:
    case IDIVB: case IMODB:
    case ISHLB: case ISHRB:
        step->width = sizeof(BYTE);
        step->family = 1;
        break;
    case IBEQW: case IBNEW: case IBLTW: case IBLEW: case IBGTW: case IBGEW:
        branch = 1;
        /* fall through */
    case IADDW: case ISUBW: case IMULW: case IANDW: case IORW: case IXORW:
    case IDIVW: case IMODW:
    case ISHLW: case ISHRW: case ILSRW:
        step->width = sizeof(WORD);
        step->family = 0;
        break;
    case IBEQL: case IBNEL: case IBLTL: case IBLEL: case IBGTL: case IBGEL:
        branch = 1;
        /* fall through */
    case IADDL: case ISUBL: case IMULL: case IANDL: case IORL: case IXORL:
    case IDIVL: case IMODL:
    case ISHLL: case ISHRL: case ILSRL:
        step->width = sizeof(LONG);
        step->family = 2;
        break;
    case IJMP:
        if (source_mode != AXXX || destination_mode != AIMM || middle_mode != AXNON ||
            !batch_pc(module, instruction.d.ins))
        {
            return 0;
        }
        step->source = NULL;
        step->destination = (void *)&pc->d;
        step->middle = step->destination;
        step->target = instruction.d.ins;
        step->immediate_middle = 0;
        return 1;
    default:
        return 0;
    }
    step->shift = step->opcode == ISHLB || step->opcode == ISHRB ||
        step->opcode == ISHLW || step->opcode == ISHRW || step->opcode == ILSRW ||
        step->opcode == ISHLL || step->opcode == ISHRL || step->opcode == ILSRL;
    step->source_width = step->shift ? sizeof(WORD) : step->width;
    if (branch)
    {
        if (destination_mode != AIMM || middle_mode == AXNON || !batch_pc(module, instruction.d.ins))
        {
            return 0;
        }
        step->destination = (void *)&pc->d;
        step->target = instruction.d.ins;
    }
    else
    {
        if (destination_mode != AFP || instruction.d.ind < 0 || instruction.d.ind >= frame->size)
        {
            return 0;
        }
        step->destination = DVM_R.FP + instruction.d.ind;
        if (!frame_scalar(step->destination, frame, step->width))
        {
            return 0;
        }
    }
    if (source_mode == AFP && instruction.s.ind >= 0 && instruction.s.ind < frame->size)
    {
        step->source = DVM_R.FP + instruction.s.ind;
        if (!frame_scalar(step->source, frame, step->source_width))
        {
            return 0;
        }
    }
    else if (source_mode == AMP && module_type && instruction.s.ind >= 0 &&
        instruction.s.ind < module_type->size)
    {
        step->source = DVM_R.MP + instruction.s.ind;
        if (!scalar_span(step->source, module_type, step->source_width, (uintptr_t)DVM_R.MP, 0))
        {
            return 0;
        }
    }
    else if (source_mode == AIMM && step->source_width <= sizeof(WORD))
    {
        step->source = &pc->s;
    }
    else
    {
        return 0;
    }
    step->immediate_middle = middle_mode == AXIMM;
    if (step->immediate_middle)
    {
        if (step->width > sizeof(WORD))
        {
            return 0;
        }
        step->immediate = instruction.reg < 32768 ? instruction.reg : (int)instruction.reg - 65536;
        step->middle = &step->immediate;
        return 1;
    }
    if (middle_mode == AXNON)
    {
        step->middle = step->destination;
        return 1;
    }
    if (middle_mode == AXINM && module_type && instruction.reg < module_type->size)
    {
        step->middle = DVM_R.MP + instruction.reg;
        return scalar_span(step->middle, module_type, step->width, (uintptr_t)DVM_R.MP, 0);
    }
    if (middle_mode != AXINF || instruction.reg >= frame->size)
    {
        return 0;
    }
    step->middle = DVM_R.FP + instruction.reg;
    return frame_scalar(step->middle, frame, step->width);
}


/* Execute a bounded private integer trace in one shared interval. The suspended
 * guest retains its frame and module. No throwing callbacks run in the trace. */
int dvm_runtime_integer_batch(const Module *module, const Inst *pc, int index)
{
    IntegerStep step, last;
    Type *frame;
    const Type *module_type;
    DVM_CellDomain *domain;
    DVM_HeapOwner *owner;
    const Inst *cursor = pc;
    int completed = 0, branches = 0, released, limit;
    unsigned byte_steps = 0, long_steps = 0;
    WORD middle_temp = DVM_R.t;
    const void *saved_source = DVM_R.s;
    if (!dvm_runtime_shared_integer || dvm_runtime_parallel_add != 2 ||
        module->cstep || DVM_R.IC < 2 || !(frame = current_frame_type()))
    {
        return 0;
    }
    module_type = module_data_type(&DVM_R);
    if (!decode_integer_step(module, pc, frame, module_type, &step) ||
        (!step.target && (index >= module->nprog - 1 ||
         !decode_integer_step(module, pc + 1, frame, module_type, &step))))
    {
        return 0;
    }
    domain = dvm_managed_cell_domain();
    if (!domain)
    {
        return 0;
    }
    limit = DVM_R.IC < 16 ? DVM_R.IC : 16;
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
    while (completed < limit && decode_integer_step(module, cursor, frame, module_type, &step))
    {
        if (step.opcode == IJMP)
        {
            cursor = step.target;
            last = step;
            completed++;
            branches++;
            continue;
        }
        DVM_TripleAccess access = {0};
        WORD count = 0;
        size_t destination_size = step.target ? sizeof(Inst *) : step.width;
        if (dvm_small_range_access_begin(domain, step.source, step.source_width,
            step.middle, step.width, step.destination, destination_size, &access) != DVM_CELL_ACQUIRED)
        {
            break;
        }
        /* Leave the failing instruction to normal dispatch after exclusive reentry. */
        if ((integer_division(step.opcode) && dvm_runtime_divisor_zero(step.opcode, step.source)) ||
            (step.shift && !dvm_runtime_shift_count(step.opcode, step.source, &count)))
        {
            if (!dvm_small_range_access_end(&access))
            {
                abort();
            }
            break;
        }
        if (step.target)
        {
            branches++;
            cursor = dvm_runtime_branch_predicate(step.opcode, step.source, step.middle) ?
                step.target : cursor + 1;
        }
        else if (step.shift)
        {
            dvm_runtime_shift_value(step.opcode, step.middle, count, step.destination);
            cursor++;
        }
        else
        {
            dvm_runtime_arithmetic_value(step.opcode, step.middle, step.source, step.destination);
            cursor++;
        }
        if (!dvm_small_range_access_end(&access))
        {
            abort();
        }
        if (step.immediate_middle)
        {
            middle_temp = step.immediate;
        }
        byte_steps += step.family == 1;
        long_steps += step.family == 2;
        saved_source = step.source;
        last = step;
        completed++;
    }
    if (!dvm_native_owner_leave_shared(owner))
    {
        abort();
    }
    dvm_runtime_gc_native_enter(released);
    if (completed)
    {
        DVM_R.t = middle_temp;
        DVM_R.s = (void *)saved_source;
        DVM_R.m = last.immediate_middle ? (void *)&DVM_R.t : (void *)last.middle;
        DVM_R.d = last.destination;
        DVM_R.PC = (Inst *)cursor;
        DVM_R.IC -= completed;
        dvm_runtime_shared_integer_commits += completed;
        dvm_runtime_shared_integer_branches += branches;
        dvm_runtime_shared_byte_steps += byte_steps;
        dvm_runtime_shared_long_steps += long_steps;
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
    division = integer_division(opcode);
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
