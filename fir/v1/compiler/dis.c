/* Direct NC64 Dis emission for verified straight-line FIR i32 programs.
 * This backend does not invoke the temporary Limbo compiler or a Dis assembler. */
#include "fir.h"
#include <string.h>

enum
{
    DIS_MAGIC = 819264, /* Explicit experimental eight-byte native-cell target. */
    DIS_MOVW = 45,      /* Existing Dis instruction numbers, not new opcodes. */
    DIS_ADDW = 58, /* Native-word addition; followed by explicit i32 normalization. */
    DIS_SUBW = 61, /* Native-word subtraction with middle minus source ordering. */
    DIS_MULW = 64, /* Native-word multiplication; followed by explicit i32 normalization. */
    DIS_ANDW = 72, /* Retain the low 32 result bits. */
    DIS_XORW = 76, /* Bias the sign bit before signed-i32 normalization. */
    DIS_RET = 12, /* Existing Dis return, including normal entry-process termination. */
    DIS_MP = 0,          /* Module-relative byte offset. */
    DIS_FP = 1,          /* Frame-relative byte offset. */
    DIS_NONE = 3,        /* Absent operand. */
    DIS_MIDDLE_FP = 128, /* Middle operand is a frame-relative byte offset. */
    DIS_FRAME = 32,      /* Four native pointer-sized frame header fields. */
    DIS_CELL = 8,        /* Native WORD and LONG both occupy eight bytes in NC64. */
    DIS_BIG_DATA = 8     /* Existing DEFL initializer, explicitly eight-byte big-endian payload. */
};


/* Write one format byte to borrowed output. ferror is checked by the public entry. */
static void octet(FILE *output, unsigned value)
{
    fputc((int)(value & 255U), output);
}


/* Encode a bounded nonnegative Dis operand; all callers preflight the module limits.
 * Values here fit signed 30-bit operands; no implementation-defined signed shifts are needed. */
static void operand(FILE *output, unsigned value)
{
    if (value <= 63U)
    {
        octet(output, value);
    }
    else if (value <= 8191U)
    {
        octet(output, 128U | (value >> 8));
        octet(output, value);
    }
    else
    {
        octet(output, 192U | (value >> 24));
        octet(output, value >> 16);
        octet(output, value >> 8);
        octet(output, value);
    }
}


/* Emit one source/destination instruction with an optional frame middle operand.
 * Every offset is in NC64 bytes. output is borrowed; this function owns no heap or parser state. */
static void instruction(FILE *output,
                        unsigned opcode,
                        unsigned source_kind,
                        unsigned source,
                        unsigned destination_kind,
                        unsigned destination,
                        int middle)
{
    unsigned mode = (source_kind << 3) | destination_kind;
    if (middle >= 0)
    {
        mode |= DIS_MIDDLE_FP;
    }
    octet(output, opcode);
    octet(output, mode);
    if (middle >= 0)
    {
        operand(output, (unsigned)middle);
    }
    if (source_kind != DIS_NONE)
    {
        operand(output, source);
    }
    if (destination_kind != DIS_NONE)
    {
        operand(output, destination);
    }
}


/* Return the frame byte offset of one verifier-admitted SSA value. */
static unsigned slot(int value)
{
    return DIS_FRAME + (unsigned)value * DIS_CELL;
}


/* Write a signed/native integer as portable big-endian DEFL bytes.
 * Conversion to uint64_t preserves the defined two's-complement file representation. */
static void integer(FILE *output, int64_t value)
{
    uint64_t bits = (uint64_t)value;
    int shift;
    for (shift = 56; shift >= 0; shift -= 8)
    {
        octet(output, (unsigned)(bits >> shift));
    }
}


/* Validate all backend-specific restrictions before writing even the magic.
 * module is borrowed after FIR verification; counts receive bounded instruction/trace extents.
 * Unsupported FIR is diagnosed, never silently reinterpreted with different numeric semantics. */
static int supported(FirModule *module, unsigned *count, unsigned *traces, char *error, size_t capacity)
{
    int index;
    *count = 0;
    *traces = 0;
    if (module->block_count != 1 || module->source_semantics == FIR_AVM1_NUMERIC)
    {
        if (error && capacity)
        {
            snprintf(error, capacity, "Dis i32 backend requires one block and non-AVM1 semantics");
        }
        return 0;
    }
    for (index = 0; index < module->count; index++)
    {
        FirInstruction *current = &module->instructions[index];
        if (current->removed)
        {
            continue;
        }
        switch (current->op)
        {
        case O_I32:
        case O_COPY:
        case O_ADD:
        case O_SUB:
        case O_MUL:
            if (current->type != T_I32)
            {
                goto unsupported;
            }
            *count += current->op == O_ADD || current->op == O_SUB || current->op == O_MUL ? 4U : 1U;
            break;
        case O_TRACE:
            if (module->values[current->a].type != T_I32)
            {
                goto unsupported;
            }
            (*traces)++;
            (*count)++;
            break;
        case O_RETURN:
            (*count)++;
            break;
        default:
        unsupported:
            if (error && capacity)
            {
                snprintf(error, capacity, "line %d: operation/type is outside Dis i32 backend coverage", current->line);
            }
            return 0;
        }
    }
    return 1;
}


/* Emit existing NC64 Dis directly from caller-owned FIR, preserving i32 wrap arithmetic.
 * output/module are borrowed; no native runtime/SDK is required. Verification or coverage errors
 * leave output untouched. I/O errors can leave a prefix; CLI staging protects prior destination files.
 * Trace statements store ordered results in the first module-data cells for debugger inspection.
 * See fir.h and doc/DIS_BACKEND.md for the deliberately bounded entry/module contract. */
int fir_dis(FILE *output, FirModule *module, char *error, size_t capacity)
{
    unsigned count, traces, data_size, frame_size, mask, sign, constants;
    unsigned trace = 0;
    int index;
    if (!error)
    {
        capacity = 0;
    }
    if (!output || !module)
    {
        if (error && capacity)
        {
            snprintf(error, capacity, "invalid Dis backend arguments");
        }
        return 0;
    }
    if (!fir_verify(module, error, capacity) || !supported(module, &count, &traces, error, capacity))
    {
        return 0;
    }
    mask = traces * DIS_CELL;
    sign = mask + DIS_CELL;
    constants = sign + DIS_CELL;
    data_size = constants + (unsigned)module->value_count * DIS_CELL;
    frame_size = slot(module->value_count);
    operand(output, DIS_MAGIC);
    operand(output, 0); /* No runtime flags, signed payload, imports or exception table. */
    operand(output, frame_size + 4096U);
    operand(output, count);
    operand(output, data_size);
    operand(output, 2); /* Descriptor zero is module data; descriptor one is the entry frame. */
    operand(output, 0); /* No public function exports: this is an executable numeric harness. */
    operand(output, 0); /* Entry instruction zero. */
    operand(output, 1); /* Entry frame descriptor one. */
    for (index = 0; index < module->count; index++)
    {
        FirInstruction *current = &module->instructions[index];
        unsigned destination = current->result < 0 ? 0U : slot(current->result);
        unsigned opcode;
        if (current->removed)
        {
            continue;
        }
        switch (current->op)
        {
        case O_I32:
            instruction(
                output, DIS_MOVW, DIS_MP, constants + (unsigned)current->result * DIS_CELL, DIS_FP, destination, -1);
            break;
        case O_COPY:
            instruction(output, DIS_MOVW, DIS_FP, slot(current->a), DIS_FP, destination, -1);
            break;
        case O_ADD:
        case O_SUB:
        case O_MUL:
            opcode = current->op == O_ADD ? DIS_ADDW : current->op == O_SUB ? DIS_SUBW : DIS_MULW;
            instruction(output, opcode, DIS_FP, slot(current->b), DIS_FP, destination, (int)slot(current->a));
            /* NC64 words are wider than FIR i32. Normalize every arithmetic result, including
             * intermediate values: ((value & 0xffffffff) ^ 0x80000000) - 0x80000000. */
            instruction(output, DIS_ANDW, DIS_MP, mask, DIS_FP, destination, (int)destination);
            instruction(output, DIS_XORW, DIS_MP, sign, DIS_FP, destination, (int)destination);
            instruction(output, DIS_SUBW, DIS_MP, sign, DIS_FP, destination, (int)destination);
            break;
        case O_TRACE:
            instruction(output, DIS_MOVW, DIS_FP, slot(current->a), DIS_MP, trace++ * DIS_CELL, -1);
            break;
        default: /* Preflight admits only the remaining O_RETURN. */
            instruction(output, DIS_RET, DIS_NONE, 0, DIS_NONE, 0, -1);
            break;
        }
    }
    operand(output, 0);
    operand(output, data_size);
    operand(output, 0); /* Scalar-only module data has no GC pointer-map bytes. */
    operand(output, 1);
    operand(output, frame_size);
    operand(output, 0);               /* Scalar locals; frame-header roots are managed by the Dis stack. */
    octet(output, DIS_BIG_DATA << 4); /* Extended-count existing DEFL record. */
    operand(output, data_size / DIS_CELL);
    operand(output, 0);
    for (index = 0; index < (int)traces; index++)
    {
        integer(output, 0);
    }
    integer(output, INT64_C(4294967295));
    integer(output, INT64_C(2147483648));
    for (index = 0; index < module->value_count; index++)
    {
        FirInstruction *definition = &module->instructions[module->values[index].definition];
        integer(output, definition->op == O_I32 ? definition->integer : 0);
    }
    octet(output, 0); /* End initializers. */
    fwrite(module->module, 1, strlen(module->module) + 1, output);
    octet(output, 0); /* Empty source path; full SBL debug generation remains separate work. */
    if (ferror(output))
    {
        if (error && capacity)
        {
            snprintf(error, capacity, "Dis output write failed");
        }
        return 0;
    }
    return 1;
}
