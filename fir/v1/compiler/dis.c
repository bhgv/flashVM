/* Direct NC64 Dis emission for verified scalar FIR control-flow programs.
 * This backend does not invoke the temporary Limbo compiler or a Dis assembler. */
#include "fir.h"
#include <string.h>

enum
{
    DIS_MAGIC = 819264,  /* Explicit experimental eight-byte native-cell target. */
    DIS_BLTL = 130,      /* Signed big less-than branch. */
    DIS_BEQL = 134,      /* Full-width big equality branch. */
    DIS_SHLL = 127,      /* Checked full-width left shift by a native int count. */
    DIS_SHRL = 128,      /* Checked arithmetic full-width right shift. */
    DIS_MOVL = 118,      /* Exact full-width big move; independent of host long. */
    DIS_ADDL = 119,      /* Modulo-2^64 addition. */
    DIS_SUBL = 120,      /* Modulo-2^64 subtraction and negation. */
    DIS_DIVL = 121,      /* Checked signed quotient. */
    DIS_MODL = 122,      /* Checked signed remainder. */
    DIS_MULL = 123,      /* Modulo-2^64 multiplication. */
    DIS_ANDL = 124,      /* Full-width bit intersection. */
    DIS_ORL = 125,       /* Full-width bit union. */
    DIS_XORL = 126,      /* Full-width exclusive-or and complement. */
    DIS_MOVF = 46,       /* Existing binary64 move preserves signed zero and payload bits. */
    DIS_ADDF = 59,       /* Binary64 addition. */
    DIS_SUBF = 62,       /* Binary64 middle minus source. */
    DIS_MULF = 65,       /* Binary64 multiplication. */
    DIS_DIVF = 68,       /* Binary64 middle divided by source; runtime exception policy applies. */
    DIS_MOVW = 45,       /* Existing Dis instruction numbers, not new opcodes. */
    DIS_ADDW = 58,       /* Native-word addition; followed by explicit i32 normalization. */
    DIS_SUBW = 61,       /* Native-word subtraction with middle minus source ordering. */
    DIS_DIVW = 67,       /* Native signed quotient, followed by int normalization. */
    DIS_MODW = 69,       /* Native signed remainder, followed by int normalization. */
    DIS_MULW = 64,       /* Native-word multiplication; followed by explicit i32 normalization. */
    DIS_ANDW = 72,       /* Retain the low 32 result bits. */
    DIS_XORW = 76,       /* Bias the sign bit before signed-i32 normalization. */
    DIS_RET = 12,        /* Existing Dis return, including normal entry-process termination. */
    DIS_JMP = 13,        /* Existing Dis branch to a verified program index. */
    DIS_BEQW = 93,       /* Signed/native equality used to materialize bool. */
    DIS_BNEW = 94,       /* Branch when a normalized bool differs from scratch zero. */
    DIS_BLEW = 96,       /* Constant 32 <= count rejection before int shifts. */
    DIS_BGTW = 97,       /* Constant zero > count rejection before int shifts. */
    DIS_BLTW = 95,       /* Signed/native comparison of normalized i32 values. */
    DIS_IMMEDIATE = 2,   /* Immediate scalar or loader-relocated branch program index. */
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


/* Write eight caller-supplied payload bits as portable big-endian DEFL bytes.
 * Both native integers and IEEE binary64 use the existing eight-byte initializer. */
static void bits64(FILE *output, uint64_t bits)
{
    int shift;
    for (shift = 56; shift >= 0; shift -= 8)
    {
        octet(output, (unsigned)(bits >> shift));
    }
}


/* Preflight scalar coverage and map FIR instruction/block indices to final Dis PCs.
 * module is borrowed after verification. Output arrays/counts are caller-owned and bounded
 * by FIR_LIMIT/FIR_BLOCKS. No file bytes or caller IR are modified on rejection. */
static int supported(
    FirModule *module, unsigned *count, unsigned *traces, unsigned *pcs, unsigned *blocks, char *error, size_t capacity)
{
    int index;
    *count = 1; /* Entry prologue initializes a dedicated scalar zero scratch slot. */
    *traces = 0;
    memset(blocks, 0, FIR_BLOCKS * sizeof(*blocks));
    if (module->channels)
    {
        if (error && capacity)
        {
            snprintf(error, capacity, "Dis scalar backend does not implement channel requirements");
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
        pcs[index] = *count;
        if (!blocks[current->block])
        {
            blocks[current->block] = *count;
        }
        switch (current->op)
        {
        case O_I32:
        case O_BOOL:
        case O_F64:
        case O_I64:
        case O_COPY:
        case O_TO_BIG:
            if (current->op == O_TO_BIG && module->values[current->a].type != T_I64 &&
                module->values[current->a].type != T_I32 && module->values[current->a].type != T_BOOL)
            {
                goto unsupported;
            }
            if (current->type != T_I32 && current->type != T_BOOL && current->type != T_F64 && current->type != T_I64)
            {
                goto unsupported;
            }
            (*count)++;
            break;
        case O_BOOL_LOCAL:
        case O_BOOL_STORE:
        case O_BOOL_LOAD:
            (*count)++;
            break;
        case O_I32_SHL:
        case O_I32_SHR:
            *count += 8U;
            break;
        case O_TO_INT:
            *count += 3U;
            break;
        case O_BIG_SHL:
        case O_BIG_SHR:
        case O_BIG_ADD:
        case O_BIG_SUB:
        case O_BIG_MUL:
        case O_BIG_DIV:
        case O_BIG_MOD:
        case O_BIG_AND:
        case O_BIG_OR:
        case O_BIG_XOR:
        case O_BIG_NEG:
        case O_BIG_NOT:
        case O_FADD:
        case O_FSUB:
        case O_FMUL:
        case O_FDIV:
            (*count)++;
            break;
        case O_I32_DIV:
        case O_I32_MOD:
        case O_ADD:
        case O_SUB:
        case O_MUL:
        case O_BIG_LT:
        case O_BIG_EQ:
        case O_LT:
        case O_EQ:
            *count += 4U;
            break;
        case O_TRACE:
            if (module->values[current->a].type != T_I32 && module->values[current->a].type != T_BOOL &&
                module->values[current->a].type != T_F64 && module->values[current->a].type != T_I64)
            {
                goto unsupported;
            }
            (*traces)++;
            *count += 2U; /* Capture value and set its distinct executed flag. */
            break;
        case O_IF:
            *count += 2U; /* Conditional yes edge followed by unconditional no edge. */
            break;
        case O_JUMP:
        case O_RETURN:
            (*count)++;
            break;
        default:
        unsupported:
            if (error && capacity)
            {
                snprintf(
                    error, capacity, "line %d: operation/type is outside Dis scalar backend coverage", current->line);
            }
            return 0;
        }
    }
    for (index = 0; index < module->block_count; index++)
    {
        if (!blocks[index])
        {
            if (error && capacity)
            {
                snprintf(error, capacity, "Dis scalar backend found a block without emitted instructions");
            }
            return 0;
        }
    }
    return 1;
}


/* Emit signature-checked table calls from verified IR; owned by the separate C99 table backend. */
extern int fir_dis_table(FILE *, FirModule *, char *, size_t);


/* Emit existing NC64 Dis directly from caller-owned FIR, preserving i32 wrap arithmetic.
 * output/module are borrowed; no native runtime/SDK is required. Verification or coverage errors
 * leave output untouched. I/O errors can leave a prefix; CLI staging protects prior destination files.
 * Trace statements store values followed by 0/1 executed flags, indexed by static trace point.
 * See fir.h and doc/DIS_BACKEND.md for the deliberately bounded entry/module contract. */
int fir_dis(FILE *output, FirModule *module, char *error, size_t capacity)
{
    unsigned count, traces, data_size, frame_size, mask, sign, constants;
    unsigned trace = 0;
    unsigned pcs[FIR_LIMIT];     /* Final start PC for each live FIR instruction, local to this emission. */
    unsigned blocks[FIR_BLOCKS]; /* Final start PC for each verifier-admitted CFG block. */
    unsigned scratch;            /* Extra frame word initialized once; never aliases an SSA result. */
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
    if (!fir_verify(module, error, capacity))
    {
        return 0;
    }
    if (strlen(module->module) >= 128)
    {
        snprintf(error, capacity, "Dis module name exceeds the loader's 127-byte limit");
        return 0;
    }
    for (index = 0; index < module->count; index++)
    {
        FirInstruction *in = &module->instructions[index];
        if (!in->removed &&
            (((in->op >= O_TABLE_NEW && in->op <= O_TO_TEXT) &&
              !(in->op == O_TO_BIG && (module->values[in->a].type == T_I64 || module->values[in->a].type == T_I32 ||
                                       module->values[in->a].type == T_BOOL))) ||
             (in->op >= O_TABLE_COUNT && in->op <= O_TABLE_DELETE) ||
             (in->op >= O_TABLE_RANGE && in->op <= O_REF_STORE) ||
             (in->op >= O_STRING_LEN && in->op <= O_STRING_SLICE) || in->op == O_STRING_CONCAT ||
             in->op == O_STRING_EQ || in->op == O_STRING_LT || (in->type >= T_TABLE_BIG && in->type != T_BOOL_SLOT)))
        {
            return fir_dis_table(output, module, error, capacity);
        }
    }
    if (!supported(module, &count, &traces, pcs, blocks, error, capacity))
    {
        return 0;
    }
    mask = 2U * traces * DIS_CELL;
    sign = mask + DIS_CELL;
    constants = sign + DIS_CELL;
    data_size = constants + (unsigned)module->value_count * DIS_CELL;
    scratch = slot(module->value_count);
    frame_size = scratch + DIS_CELL;
    operand(output, DIS_MAGIC);
    operand(output, 0); /* No runtime flags, signed payload, imports or exception table. */
    operand(output, frame_size + 4096U);
    operand(output, count);
    operand(output, data_size);
    operand(output, 2); /* Descriptor zero is module data; descriptor one is the entry frame. */
    operand(output, 0); /* No public function exports: this is an executable numeric harness. */
    operand(output, 0); /* Entry instruction zero. */
    operand(output, 1); /* Entry frame descriptor one. */
    instruction(output, DIS_MOVW, DIS_IMMEDIATE, 0, DIS_FP, scratch, -1);
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
        case O_BOOL_LOCAL:
            instruction(output, DIS_MOVW, DIS_IMMEDIATE, 0, DIS_FP, destination, -1);
            break;
        case O_BOOL_STORE:
            instruction(output, DIS_MOVW, DIS_FP, slot(current->b), DIS_FP, slot(current->a), -1);
            break;
        case O_BOOL_LOAD:
            instruction(output, DIS_MOVW, DIS_FP, slot(current->a), DIS_FP, destination, -1);
            break;
        case O_I32:
        case O_BOOL:
        case O_F64:
        case O_I64:
            instruction(output,
                        current->type == T_F64   ? DIS_MOVF
                        : current->type == T_I64 ? DIS_MOVL
                                                 : DIS_MOVW,
                        DIS_MP,
                        constants + (unsigned)current->result * DIS_CELL,
                        DIS_FP,
                        destination,
                        -1);
            break;
        case O_COPY:
        case O_TO_BIG:
            instruction(output,
                        current->type == T_F64   ? DIS_MOVF
                        : current->type == T_I64 ? DIS_MOVL
                                                 : DIS_MOVW,
                        DIS_FP,
                        slot(current->a),
                        DIS_FP,
                        destination,
                        -1);
            break;
        case O_I32_DIV:
        case O_I32_MOD:
        case O_ADD:
        case O_SUB:
        case O_MUL:
            opcode = current->op == O_ADD       ? DIS_ADDW
                     : current->op == O_SUB     ? DIS_SUBW
                     : current->op == O_MUL     ? DIS_MULW
                     : current->op == O_I32_DIV ? DIS_DIVW
                                                : DIS_MODW;
            instruction(output, opcode, DIS_FP, slot(current->b), DIS_FP, destination, (int)slot(current->a));
            /* NC64 words are wider than FIR i32. Normalize every arithmetic result, including
             * intermediate values: ((value & 0xffffffff) ^ 0x80000000) - 0x80000000. */
            instruction(output, DIS_ANDW, DIS_MP, mask, DIS_FP, destination, (int)destination);
            instruction(output, DIS_XORW, DIS_MP, sign, DIS_FP, destination, (int)destination);
            instruction(output, DIS_SUBW, DIS_MP, sign, DIS_FP, destination, (int)destination);
            break;
        case O_I32_SHL:
        case O_I32_SHR:
            /* FIR counts are 0..31 even on NC64. The error edge deliberately
             * executes the existing checked big shift with count 64, raising
             * the VM's normal invalid-argument exception without a new ABI. */
            instruction(output, DIS_BGTW, DIS_IMMEDIATE, 0, DIS_IMMEDIATE, pcs[index] + 7U, (int)slot(current->b));
            instruction(output, DIS_BLEW, DIS_IMMEDIATE, 32, DIS_IMMEDIATE, pcs[index] + 7U, (int)slot(current->b));
            instruction(output,
                        current->op == O_I32_SHL ? DIS_SHLL : DIS_SHRL,
                        DIS_FP,
                        slot(current->b),
                        DIS_FP,
                        destination,
                        (int)slot(current->a));
            instruction(output, DIS_ANDW, DIS_MP, mask, DIS_FP, destination, (int)destination);
            instruction(output, DIS_XORW, DIS_MP, sign, DIS_FP, destination, (int)destination);
            instruction(output, DIS_SUBW, DIS_MP, sign, DIS_FP, destination, (int)destination);
            instruction(output, DIS_JMP, DIS_NONE, 0, DIS_IMMEDIATE, pcs[index] + 8U, -1);
            instruction(output, DIS_SHLL, DIS_IMMEDIATE, 64, DIS_FP, destination, (int)slot(current->a));
            break;
        case O_TO_INT:
            instruction(output, DIS_ANDW, DIS_MP, mask, DIS_FP, destination, (int)slot(current->a));
            instruction(output, DIS_XORW, DIS_MP, sign, DIS_FP, destination, (int)destination);
            instruction(output, DIS_SUBW, DIS_MP, sign, DIS_FP, destination, (int)destination);
            break;
        case O_BIG_SHL:
        case O_BIG_SHR:
            instruction(output,
                        current->op == O_BIG_SHL ? DIS_SHLL : DIS_SHRL,
                        DIS_FP,
                        slot(current->b),
                        DIS_FP,
                        destination,
                        (int)slot(current->a));
            break;
        case O_BIG_ADD:
        case O_BIG_SUB:
        case O_BIG_MUL:
        case O_BIG_DIV:
        case O_BIG_MOD:
        case O_BIG_AND:
        case O_BIG_OR:
        case O_BIG_XOR:
            opcode = current->op == O_BIG_ADD   ? DIS_ADDL
                     : current->op == O_BIG_SUB ? DIS_SUBL
                     : current->op == O_BIG_MUL ? DIS_MULL
                     : current->op == O_BIG_DIV ? DIS_DIVL
                     : current->op == O_BIG_MOD ? DIS_MODL
                     : current->op == O_BIG_AND ? DIS_ANDL
                     : current->op == O_BIG_OR  ? DIS_ORL
                                                : DIS_XORL;
            instruction(output, opcode, DIS_FP, slot(current->b), DIS_FP, destination, (int)slot(current->a));
            break;
        case O_BIG_NEG:
            instruction(output, DIS_SUBL, DIS_FP, slot(current->a), DIS_FP, destination, (int)scratch);
            break;
        case O_BIG_NOT:
            /* The existing per-result plain constant cell holds the all-ones mask.
             * XOR is commutative, so the middle remains a normal frame operand. */
            instruction(output,
                        DIS_XORL,
                        DIS_MP,
                        constants + (unsigned)current->result * DIS_CELL,
                        DIS_FP,
                        destination,
                        (int)slot(current->a));
            break;
        case O_FADD:
        case O_FSUB:
        case O_FMUL:
        case O_FDIV:
            opcode = current->op == O_FADD   ? DIS_ADDF
                     : current->op == O_FSUB ? DIS_SUBF
                     : current->op == O_FMUL ? DIS_MULF
                                             : DIS_DIVF;
            instruction(output, opcode, DIS_FP, slot(current->b), DIS_FP, destination, (int)slot(current->a));
            break;
        case O_BIG_LT:
        case O_BIG_EQ:
        case O_LT:
        case O_EQ:
            instruction(output, DIS_MOVW, DIS_IMMEDIATE, 0, DIS_FP, destination, -1);
            instruction(output,
                        current->op == O_BIG_LT   ? DIS_BLTL
                        : current->op == O_BIG_EQ ? DIS_BEQL
                        : current->op == O_LT     ? DIS_BLTW
                                                  : DIS_BEQW,
                        DIS_FP,
                        slot(current->a),
                        DIS_IMMEDIATE,
                        pcs[index] + 3U,
                        (int)slot(current->b));
            instruction(output, DIS_JMP, DIS_NONE, 0, DIS_IMMEDIATE, pcs[index] + 4U, -1);
            instruction(output, DIS_MOVW, DIS_IMMEDIATE, 1, DIS_FP, destination, -1);
            break;
        case O_IF:
            instruction(output, DIS_BNEW, DIS_FP, slot(current->a), DIS_IMMEDIATE, blocks[current->yes], (int)scratch);
            instruction(output, DIS_JMP, DIS_NONE, 0, DIS_IMMEDIATE, blocks[current->no], -1);
            break;
        case O_JUMP:
            instruction(output, DIS_JMP, DIS_NONE, 0, DIS_IMMEDIATE, blocks[current->yes], -1);
            break;
        case O_TRACE:
            instruction(output,
                        module->values[current->a].type == T_F64   ? DIS_MOVF
                        : module->values[current->a].type == T_I64 ? DIS_MOVL
                                                                   : DIS_MOVW,
                        DIS_FP,
                        slot(current->a),
                        DIS_MP,
                        trace * DIS_CELL,
                        -1);
            instruction(output, DIS_MOVW, DIS_IMMEDIATE, 1, DIS_MP, (traces + trace) * DIS_CELL, -1);
            trace++;
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
    for (index = 0; index < (int)(2U * traces); index++)
    {
        bits64(output, 0);
    }
    bits64(output, INT64_C(4294967295));
    bits64(output, INT64_C(2147483648));
    for (index = 0; index < module->value_count; index++)
    {
        int defining = module->values[index].definition;
        FirInstruction *definition = defining < 0 ? NULL : &module->instructions[defining];
        uint64_t payload = 0; /* Exact binary64 bits or defined modulo conversion of signed integer. */
        if (definition && definition->op == O_F64)
        {
            memcpy(&payload, &definition->real, sizeof(payload));
        }
        else if (definition && definition->op == O_I64)
        {
            payload = (uint64_t)definition->big;
        }
        else if (definition && definition->op == O_BIG_NOT)
        {
            payload = UINT64_MAX;
        }
        else if (definition && (definition->op == O_I32 || definition->op == O_BOOL))
        {
            payload = (uint64_t)(int64_t)definition->integer;
        }
        bits64(output, payload);
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
