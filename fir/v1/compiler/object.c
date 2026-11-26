/* Standalone SWO1 object emitter. Values are object-local generated variables.
 * Build everything in memory before writing: branch range failures never
 * produce a partially emitted object. No private FlashVM structures are used. */
#include "object_format.h"
#include <stdlib.h>
#include <string.h>


typedef struct
{
    unsigned char *data;
    size_t n;
    int failed;
} Bytes;


/* Append one byte, recording capacity failure without writing out of bounds. */
static void byte(Bytes *b, unsigned x)
{
    if (b->n >= FIR_OBJECT_LIMIT)
    {
        b->failed = 1;
    }
    else
    {
        b->data[b->n++] = (unsigned char)x;
    }
}


/* Append an unsigned little-endian 16-bit field. */
static void word(Bytes *b, unsigned x)
{
    byte(b, x);
    byte(b, x >> 8);
}


/* Append an unsigned little-endian 32-bit field. */
static void dword(Bytes *b, uint32_t x)
{
    word(b, x);
    word(b, x >> 16);
}


/* Push one NUL-terminated AVM1 string with a checked FIR-bounded spelling. */
static void text(Bytes *b, const char *s)
{
    byte(b, 0x96);
    word(b, (unsigned)strlen(s) + 2);
    byte(b, 0);
    do
    {
        byte(b, (unsigned char)*s);
    } while (*s++);
}


/* Push a signed i32 using the AVM1 integer constant representation. */
static void integer(Bytes *b, int32_t x)
{
    byte(b, 0x96);
    word(b, 5);
    byte(b, 7);
    dword(b, (uint32_t)x);
}


/* Push a binary64 constant using the SWF mixed-word byte layout. */
static void real(Bytes *b, double x)
{
    uint64_t bits;
    memcpy(&bits, &x, 8);
    byte(b, 0x96);
    word(b, 9);
    byte(b, 6);
    /* SWF doubles exchange the two little-endian 32-bit words. */
    dword(b, (uint32_t)(bits >> 32));
    dword(b, (uint32_t)bits);
}


/* Encode every module-name byte as two hex digits. AVM1 variable names must
 * not reinterpret an accepted module separator as a property/target path.
 * Fixed-width encoding is injective, including dots, underscores and case.
 * The dollar prefix cannot overlap legacy names beginning with __fir1_. */
static void variable(Bytes *b, const FirModule *m, int index)
{
    static const char digits[] = "0123456789abcdef";
    char name[FIR_TEXT * 2 + 40];
    const unsigned char *module = (const unsigned char *)m->module + 1;
    size_t used = 0;
    const char prefix[] = "__fir1$";
    memcpy(name, prefix, sizeof(prefix) - 1);
    used = sizeof(prefix) - 1;
    while (*module)
    {
        unsigned byte_value = *module++;
        name[used++] = digits[byte_value >> 4];
        name[used++] = digits[byte_value & 15];
    }
    /* Verified module names and SSA indices fit the spelling plus this suffix. */
    snprintf(name + used, sizeof(name) - used, "_value_%d", index);
    text(b, name);
}


/* Retrieve one SSA value from its generated object-local variable. */
static void load(Bytes *b, const FirModule *m, int index)
{
    variable(b, m, index);
    byte(b, 0x1c);
}


/* Push the ordered predicate left >= right using numeric SSA operands.
 * b owns staged bytes; m/operand identities are borrowed verified FIR values.
 * Less2 preserves unordered as undefined; comparing that result with false
 * rejects NaN without relying on the loaded player's numeric NaN equality.
 * No operand expression is reevaluated; buffer failure remains sticky in b. */
static void ordered_not_less(Bytes *b, const FirModule *m, int left, int right)
{
    load(b, m, left);
    load(b, m, right);
    byte(b, 0x48);
    byte(b, 0x96);
    word(b, 2);
    byte(b, 5);
    byte(b, 0);
    byte(b, 0x49);
}


/* Push the low unsigned 16-bit half of an i32 value. */
static void low(Bytes *b, const FirModule *m, int index)
{
    load(b, m, index);
    integer(b, 65535);
    byte(b, 0x60); /* BitAnd */
}


/* Push the high signed 16-bit half of an i32 value. */
static void high(Bytes *b, const FirModule *m, int index)
{
    load(b, m, index);
    integer(b, 16);
    byte(b, 0x64); /* signed shift right */
}


/* Full 32x32 multiplication cannot use one AVM1 double multiply: low bits
 * disappear above 2^53. Three exact 16-bit partial products preserve them. */
static void multiply(Bytes *b, const FirModule *m, int a, int c)
{
    low(b, m, a);
    low(b, m, c);
    byte(b, 0x0c);
    high(b, m, a);
    low(b, m, c);
    byte(b, 0x0c);
    high(b, m, c);
    low(b, m, a);
    byte(b, 0x0c);
    byte(b, 0x0a);
    integer(b, 16);
    byte(b, 0x63);
    byte(b, 0x0a);
    integer(b, 0);
    byte(b, 0x61);
}


/* Emit an unresolved branch and return the offset-field position. */
static size_t branch(Bytes *b, int conditional)
{
    byte(b, conditional ? 0x9d : 0x99);
    word(b, 2);
    word(b, 0);
    return b->n - 2;
}


#include "object_method.inc"


/* Lower a verified module, resolve all branch relocations, then write bytes. */
int fir_object(FILE *output, FirModule *m, char *error, size_t cap)
{
    Bytes actions = {0};
    FirAvm1Fixup fixes[FIR_LIMIT * 2];
    size_t labels[FIR_BLOCKS], end, frame = 0, frame_body = 0;
    int i, nf = 0, previous = -1, ok = 0;
    if (!fir_object_admit(m, error, cap))
    {
        return 0;
    }
    actions.data = malloc(FIR_OBJECT_LIMIT);
    if (!actions.data)
    {
        snprintf(error, cap, "out of memory");
        return 0;
    }
    if (m->avm1_frame)
    {
        frame = frame_begin(&actions, m, &frame_body);
    }
    for (i = 0; i < m->count; ++i)
    {
        const FirInstruction *in = &m->instructions[i];
        if (in->removed || in->op == O_AS_ARG)
        {
            continue;
        }
        if (previous != in->block)
        {
            labels[in->block] = actions.n;
            previous = in->block;
        }
        if (in->result >= 0)
        {
            variable(&actions, m, in->result);
        }
        switch (in->op)
        {
        case O_AS_GLOBAL_GET:
            text(&actions, in->text);
            byte(&actions, 0x1c);
            break;
        case O_AS_GLOBAL_SET:
            text(&actions, in->text);
            load(&actions, m, in->a);
            byte(&actions, 0x1d);
            break;
        case O_AS_MEMBER_GET:
            load(&actions, m, in->a);
            load(&actions, m, in->b);
            byte(&actions, FIR_AVM1_GET_MEMBER);
            break;
        case O_AS_MEMBER_PUT:
        {
            const FirInstruction *item = &m->instructions[m->values[in->b].definition];
            const FirInstruction *key = &m->instructions[m->values[item->b].definition];
            load(&actions, m, in->a);
            load(&actions, m, key->a);
            load(&actions, m, item->a);
            byte(&actions, FIR_AVM1_SET_MEMBER);
            break;
        }
        case O_AS_MEMBER_SET:
            load(&actions, m, in->a);
            text(&actions, in->text);
            load(&actions, m, in->b);
            byte(&actions, FIR_AVM1_SET_MEMBER);
            break;
        case O_AS_MEMBER_CALL:
        case O_AS_MEMBER_ARGS:
        {
            int count = 0;
            int argument = in->b;
            while (argument >= 0 && in->op == O_AS_MEMBER_ARGS)
            {
                const FirInstruction *node = &m->instructions[m->values[argument].definition];
                load(&actions, m, node->a);
                argument = node->b;
                ++count;
            }
            if (argument >= 0)
            {
                load(&actions, m, argument);
                ++count;
            }
            integer(&actions, count);
            load(&actions, m, in->a);
            text(&actions, in->text);
            byte(&actions, 0x52);
            break;
        }
        case O_AS_TO_BOOLEAN:
            load(&actions, m, in->a);
            byte(&actions, 0x12); /* Not converts through AVM1 truthiness. */
            byte(&actions, 0x12);
            break;
        case O_I32:
            integer(&actions, in->integer);
            break;
        case O_F64:
            real(&actions, in->real);
            break;
        case O_I32_LOCAL:
            integer(&actions, 0);
            break;
        case O_F64_LOCAL:
            real(&actions, 0.0);
            break;
        case O_I32_STORE:
        case O_F64_STORE:
            variable(&actions, m, in->a);
            load(&actions, m, in->b);
            byte(&actions, 0x1d);
            break;
        case O_I32_LOAD:
        case O_F64_LOAD:
            load(&actions, m, in->a);
            break;
        case O_BOOL_LOCAL:
            /* Private frame cell: its object-local variable holds the bool directly. */
            byte(&actions, 0x96);
            word(&actions, 2);
            byte(&actions, 5);
            byte(&actions, 0);
            break;
        case O_BOOL_STORE:
            variable(&actions, m, in->a);
            load(&actions, m, in->b);
            byte(&actions, 0x1d);
            break;
        case O_BOOL_LOAD:
            load(&actions, m, in->a);
            break;
        case O_BOOL:
            byte(&actions, 0x96);
            word(&actions, 2);
            byte(&actions, 5);
            byte(&actions, (unsigned)in->integer);
            break;
        case O_STRING:
            text(&actions, in->text);
            break;
        case O_COPY:
            load(&actions, m, in->a);
            break;
        case O_F64_LT:
            /* Less2's unordered undefined must become a normalized false bool. */
            load(&actions, m, in->a);
            load(&actions, m, in->b);
            byte(&actions, 0x48);
            byte(&actions, 0x12);
            byte(&actions, 0x12);
            break;
        case O_F64_LE:
            ordered_not_less(&actions, m, in->b, in->a);
            break;
        case O_F64_EQ:
            /* Ordered >= in both directions is exact equality, including +/-0.
             * Numeric Equals2 alone may admit a NaN pair in a loaded player. */
            ordered_not_less(&actions, m, in->a, in->b);
            ordered_not_less(&actions, m, in->b, in->a);
            byte(&actions, 0x10);
            break;
        case O_BOOL_NOT:
            load(&actions, m, in->a);
            byte(&actions, 0x12);
            break;
        case O_BOOL_EQ:
            load(&actions, m, in->a);
            load(&actions, m, in->b);
            byte(&actions, 0x49);
            break;
        case O_I32_AND:
        case O_I32_OR:
        case O_I32_XOR:
            load(&actions, m, in->a);
            load(&actions, m, in->b);
            byte(&actions, in->op == O_I32_AND ? 0x60 : in->op == O_I32_OR ? 0x61 : 0x62);
            break;
        case O_I32_SHL:
        case O_I32_SHR:
            /* Preflight proves the AVM1 masked count equals the checked FIR count. */
            load(&actions, m, in->a);
            load(&actions, m, in->b);
            byte(&actions, in->op == O_I32_SHL ? 0x63 : 0x64);
            break;
        case O_I32_DIV:
        case O_I32_MOD:
            load(&actions, m, in->a);
            load(&actions, m, in->b);
            byte(&actions, in->op == O_I32_DIV ? 0x0d : 0x3f);
            /* ToInt32 truncates the quotient toward zero and wraps MIN/-1.
             * It also canonicalizes a negative-zero remainder to integer zero. */
            integer(&actions, 0);
            byte(&actions, 0x61);
            break;
        case O_MUL:
            multiply(&actions, m, in->a, in->b);
            break;
        case O_ADD:
        case O_SUB:
        case O_LT:
        case O_EQ:
        case O_FADD:
        case O_FSUB:
        case O_FMUL:
        case O_FDIV:
        case O_ASADD:
            load(&actions, m, in->a);
            load(&actions, m, in->b);
            byte(&actions,
                 in->op == O_ADD || in->op == O_FADD   ? 0x0a
                 : in->op == O_SUB || in->op == O_FSUB ? 0x0b
                 : in->op == O_FMUL                    ? 0x0c
                 : in->op == O_FDIV                    ? 0x0d
                 : in->op == O_LT                      ? 0x48
                 : in->op == O_EQ                      ? 0x49
                                                       : 0x47);
            if (in->op == O_ADD || in->op == O_SUB)
            {
                integer(&actions, 0);
                byte(&actions, 0x61);
            }
            break;
        case O_NUMBER:
            load(&actions, m, in->a);
            byte(&actions, 0x4a);
            break;
        case O_CHANNEL:
            integer(&actions, 0);
            text(&actions, "channel");
            byte(&actions, 0x1c);
            text(&actions, "create");
            byte(&actions, 0x52);
            break;
        case O_RECEIVE:
            integer(&actions, 0);
            load(&actions, m, in->a);
            text(&actions, "receive");
            byte(&actions, 0x52);
            break;
        case O_SEND:
            load(&actions, m, in->b);
            integer(&actions, 1);
            load(&actions, m, in->a);
            text(&actions, "send");
            byte(&actions, 0x52);
            byte(&actions, 0x17);
            break;
        case O_CLOSE:
            integer(&actions, 0);
            load(&actions, m, in->a);
            text(&actions, "close");
            byte(&actions, 0x52);
            byte(&actions, 0x17);
            break;
        case O_TRACE:
            load(&actions, m, in->a);
            byte(&actions, 0x26);
            break;
        case O_IF:
            load(&actions, m, in->a);
            fixes[nf].at = branch(&actions, 1);
            fixes[nf++].target = in->yes;
            fixes[nf].at = branch(&actions, 0);
            fixes[nf++].target = in->no;
            break;
        case O_JUMP:
        case O_RETURN:
            if (in->op == O_RETURN && in->a >= 0)
            {
                load(&actions, m, in->a);
                byte(&actions, FIR_AVM1_RETURN);
                break;
            }
            fixes[nf].at = branch(&actions, 0);
            fixes[nf++].target = in->op == O_RETURN ? -1 : in->yes;
            break;
        default:
            snprintf(error, cap, "operation is outside the AVM1 backend");
            goto done;
        }
        if (in->result >= 0)
        {
            byte(&actions, m->avm1_frame ? 0x3c : 0x1d); /* DefineLocal or SetVariable. */
        }
    }
    if (actions.failed)
    {
        snprintf(error, cap, "action size limit exceeded");
        goto done;
    }
    end = actions.n;
    byte(&actions, 0); /* ActionEnd */
    if (actions.failed || (m->avm1_frame && !frame_end(&actions, frame, frame_body)))
    {
        snprintf(error, cap, "AVM1 action or frame body size limit exceeded");
        goto done;
    }
    if (fir_object_relocate(actions.data, fixes, nf, labels, end, error, cap))
    {
        ok = fir_object_write(output, m, actions.data, actions.n, error, cap);
    }
done:
    free(actions.data);
    return ok;
}
