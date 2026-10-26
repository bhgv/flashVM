/* FIR v1 semantic effects, independent of the concrete AVM1 lowering.
 * Pure literals represent logical values, not observable allocator calls.
 * External/dynamic operations are conservative until a narrower contract exists. */
#include "fir.h"

unsigned fir_effects(FirOp op)
{
    switch (op)
    {
    case O_TO_INT:
    case O_BIG_LT:
    case O_BIG_EQ:
    case O_BIG_ADD:
    case O_BIG_SUB:
    case O_BIG_MUL:
    case O_BIG_AND:
    case O_BIG_OR:
    case O_BIG_XOR:
    case O_BIG_NOT:
    case O_BIG_NEG:
    case O_I64:
    case O_I32:
    case O_F64:
    case O_BOOL:
    case O_STRING:
    case O_COPY:
    case O_ADD:
    case O_SUB:
    case O_MUL:
    case O_LT:
    case O_EQ:
    case O_FADD:
    case O_FSUB:
    case O_FMUL:
    case O_FDIV:
    case O_IF:
    case O_JUMP:
    case O_RETURN:
        return 0;
    case O_BIG_SHL:
    case O_BIG_SHR:
    case O_BIG_DIV:
    case O_BIG_MOD:
        return FIR_MAY_THROW;
    case O_ASADD:
    case O_NUMBER:
    case O_CHANNEL:
    case O_RECEIVE:
    case O_TRACE:
    case O_SEND:
    case O_CLOSE:
    default:
        return FIR_ALL_EFFECTS;
    }
}
