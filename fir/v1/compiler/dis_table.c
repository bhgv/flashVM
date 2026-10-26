/* Direct NC64 Dis table lowering through signature-checked hosted Bigtable calls. */
#include "fir.h"
#include <stdlib.h>
#include <string.h>

#define CELL 8U /* Explicit NC64 frame/data cell size; never derived from host pointers. */
#define MAP_BYTES ((2U * FIR_LIMIT + 32U + 7U) / 8U) /* Maximum entry-frame map for paired SSA slots. */

enum
{
    LOAD = 8,              /* Load a signature-checked module instance. */
    MCALL = 9,             /* Invoke the prepared module function. */
    MFRAME = 11,           /* Allocate a typed native call frame. */
    RET = 12,              /* Return from the entry harness. */
    LEA = 39,              /* Pass a return destination by address. */
    MOVP = 41,             /* Retain/move one heap reference. */
    MOVW = 45,             /* Move native status/flag words. */
    MOVL = 118,            /* Move an exact signed big payload. */
    MP = 0,                /* Module-relative addressing. */
    FP = 1,                /* Frame-relative addressing. */
    IMM = 2,               /* Immediate scalar operand. */
    NONE = 3,              /* Absent source/destination operand. */
    INDIRECT_FP = 5,       /* Indirect frame addressing for callee argument slots. */
    MIDDLE_IMMEDIATE = 64, /* Module import indices are immediate middle operands. */
    HAS_IMPORTS = 64,      /* Existing HASLDT flag, no container extension. */
    MODULE_SLOT = 64,      /* [GC:] Rooted loaded Bigtable instance in the entry frame. */
    ARGUMENTS = 64,        /* First argument in the checked NC64 builtin ABI. */
    RETURN_ADDRESS = 32    /* Raw return-address slot in that builtin frame. */
};

typedef struct
{
    const char *name;   /* Exact exported function name. */
    uint32_t signature; /* Checked structural signature from the matching bigtable.m contract. */
} Import;

enum
{
    CREATE,      /* Create a new table owner. */
    GET_INDEX,   /* Read a numeric location. */
    GET_TEXT,    /* Read a string location. */
    INDEX_CHECK, /* Validate eager numeric-index construction. */
    KEY_LEN,     /* Count an immutable snapshot. */
    KEYS_OF,     /* Copy all keys. */
    LENGTH,      /* Read contiguous sequence length. */
    RANGE_CHECK, /* Validate eager half-open bounds. */
    REQUIRE_OK,  /* Raise on a failed get/set status. */
    SET_INDEX,   /* Write a numeric location. */
    SET_TEXT,    /* Write a string location. */
    SLICE        /* Copy a numeric sequence slice. */
};

static const Import imports[] = {
    {"create", UINT32_C(0xf9bff31f)},
    {"getindex", UINT32_C(0xd03b2e8c)},
    {"gettext", UINT32_C(0x7a1ec3e7)},
    {"indexcheck", UINT32_C(0x93504811)},
    {"keylen", UINT32_C(0x6c3299de)},
    {"keysof", UINT32_C(0xa4157ed7)},
    {"length", UINT32_C(0x6c3299de)},
    {"rangecheck", UINT32_C(0xd039b06a)},
    {"requireok", UINT32_C(0xb1f25db6)},
    {"setindex", UINT32_C(0xe420e8cf)},
    {"settext", UINT32_C(0xb244346a)},
    {"slice", UINT32_C(0x71c4132a)}}; /* Loader checks every signature before any generated native call can run. */

typedef struct
{
    FILE *body;                  /* Owned temporary bytecode, published only after successful preflight/emission. */
    unsigned count;              /* Emitted Dis instruction count. */
    unsigned offsets[FIR_LIMIT]; /* Frame offsets of typed SSA values. */
    unsigned strings[FIR_LIMIT]; /* MP offsets of live copied string constants. */
    unsigned call_frame;         /* Raw prepared stack-frame pointer, not a heap/GC root. */
    unsigned status;             /* Scalar first result cell for native status/value tuples. */
    unsigned frame_size;         /* Complete native entry-frame byte size. */
    unsigned data_size;          /* Module data size, including numeric captures and string roots. */
    unsigned plain_cells;        /* Unmapped numeric module data cells before strings. */
    unsigned traces;             /* Scalar capture count; executed flags follow the values. */
    unsigned char frame_map[MAP_BYTES];     /* [GC:] Only genuine entry-frame heap references are marked. */
    unsigned char data_map[2U * MAP_BYTES]; /* [GC:] Module path and live constant String roots. */
} Emitter;

/* Write a bounded format byte to a borrowed stream; callers check ferror before publication. */
static void octet(FILE *output, unsigned value)
{
    fputc((int)(value & 255U), output);
}

/* Encode a nonnegative existing Dis operand. All generated offsets fit signed 30-bit bounds. */
static void operand(FILE *output, unsigned value)
{
    if (value <= 63)
    {
        octet(output, value);
    }
    else if (value <= 8191)
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

/* Write exact big-endian payload bits for numeric initializers or import signatures. */
static void bits(FILE *output, uint64_t value, unsigned bytes)
{
    while (bytes)
    {
        bytes--;
        octet(output, (unsigned)(value >> (bytes * 8U)));
    }
}

/* Emit a normal source/destination instruction with optional immediate middle and indirect target.
 * The indirect target is a prepared callee frame at destination, plus the explicit byte offset. */
static void instruction(Emitter *e,
                        unsigned op,
                        unsigned source_mode,
                        unsigned source,
                        unsigned destination_mode,
                        unsigned destination,
                        unsigned indirect_offset,
                        int middle)
{
    unsigned mode = (source_mode << 3) | destination_mode;
    if (middle >= 0)
    {
        mode |= MIDDLE_IMMEDIATE;
    }
    octet(e->body, op);
    octet(e->body, mode);
    if (middle >= 0)
    {
        operand(e->body, (unsigned)middle);
    }
    if (source_mode != NONE)
    {
        operand(e->body, source);
    }
    if (destination_mode != NONE)
    {
        operand(e->body, destination);
        if (destination_mode == INDIRECT_FP)
        {
            operand(e->body, indirect_offset);
        }
    }
    e->count++;
}

/* Mark one aligned genuine heap-reference cell in an initialized type bitmap. */
static void root(unsigned char *map, unsigned offset)
{
    unsigned cell = offset / CELL;
    map[cell / 8U] |= (unsigned char)(128U >> (cell % 8U));
}

/* Resolve a slot's defining index through typed copies; verifier guarantees dominance/no cycles.
 * Returns the borrowed index instruction or null if the public IR violates supported provenance. */
static const FirInstruction *location(FirModule *module, int value)
{
    int step;
    for (step = 0; step < FIR_LIMIT; step++)
    {
        int defining;
        const FirInstruction *in;
        if (value < 0 || value >= module->value_count)
        {
            return NULL;
        }
        defining = module->values[value].definition;
        if (defining < 0)
        {
            return NULL;
        }
        in = &module->instructions[defining];
        if (in->op == O_TABLE_INDEX)
        {
            return in;
        }
        if (in->op != O_COPY)
        {
            return NULL;
        }
        value = in->a;
    }
    return NULL;
}

/* Check the whole supported subset and construct exact layouts before publishing any bytes.
 * module is already verified. Rejected CFG/types do not fall back to a fabricated scalar lowering. */
static int layout(Emitter *e, FirModule *module, char *error, size_t capacity)
{
    unsigned offset = MODULE_SLOT + CELL;
    unsigned strings = 1; /* The imported module path is always the first String root. */
    int i;
    if (module->block_count != 1 || module->channels)
    {
        snprintf(error, capacity, "Dis table backend requires one block without channels");
        return 0;
    }
    root(e->frame_map, MODULE_SLOT);
    for (i = 0; i < module->value_count; i++)
    {
        FirType type = module->values[i].type;
        e->offsets[i] = offset;
        if (type == T_TABLE_BIG || type == T_KEYS || type == T_STRING)
        {
            root(e->frame_map, offset);
        }
        else if (type == T_TABLE_SLOT)
        {
            const FirInstruction *in = location(module, i);
            if (!in)
            {
                snprintf(error, capacity, "unsupported Dis table location provenance");
                return 0;
            }
            root(e->frame_map, offset);
            if (module->values[in->b].type == T_STRING)
            {
                root(e->frame_map, offset + CELL);
            }
        }
        else if (type != T_I64 && type != T_BIG_RANGE)
        {
            snprintf(error, capacity, "type outside Dis table backend coverage");
            return 0;
        }
        offset += type == T_TABLE_SLOT || type == T_BIG_RANGE ? 2U * CELL : CELL;
    }
    e->call_frame = offset;
    e->status = offset + CELL;
    e->frame_size = offset + 3U * CELL; /* Raw frame pointer plus scalar (status,big) pair. */
    for (i = 0; i < module->count; i++)
    {
        FirInstruction *in = &module->instructions[i];
        if (in->removed)
        {
            continue;
        }
        if (in->op == O_STRING)
        {
            strings++;
        }
        else if (in->op == O_TRACE)
        {
            if (module->values[in->a].type != T_I64)
            {
                snprintf(error, capacity, "Dis table trace requires big");
                return 0;
            }
            e->traces++;
        }
        else if (in->op != O_COPY && in->op != O_RETURN && !(in->op >= O_I64 && in->op <= O_KEYS_LEN))
        {
            snprintf(error, capacity, "line %d: operation outside Dis table backend coverage", in->line);
            return 0;
        }
    }
    e->plain_cells = 2U * e->traces + (unsigned)module->value_count;
    offset = e->plain_cells * CELL;
    root(e->data_map, offset);
    offset += CELL;
    for (i = 0; i < module->count; i++)
    {
        FirInstruction *in = &module->instructions[i];
        if (!in->removed && in->op == O_STRING)
        {
            e->strings[in->result] = offset;
            root(e->data_map, offset);
            offset += CELL;
        }
    }
    e->data_size = (e->plain_cells + strings) * CELL;
    return 1;
}

/* Prepare a signature-checked native frame. Arguments/results are filled before call_end. */
static void call_begin(Emitter *e, unsigned import)
{
    instruction(e, MFRAME, FP, MODULE_SLOT, FP, e->call_frame, 0, (int)import);
}

/* Copy a borrowed caller value into one generated native argument cell, retaining pointer owners. */
static void argument(Emitter *e, unsigned op, unsigned source, unsigned index)
{
    instruction(e, op, FP, source, INDIRECT_FP, e->call_frame, ARGUMENTS + index * CELL, -1);
}

/* Bind an optional caller return destination by address, then invoke the prepared native frame. */
static void call_end(Emitter *e, unsigned import, int destination)
{
    if (destination >= 0)
    {
        instruction(e, LEA, FP, (unsigned)destination, INDIRECT_FP, e->call_frame, RETURN_ADDRESS, -1);
    }
    instruction(e, MCALL, FP, e->call_frame, FP, MODULE_SLOT, 0, (int)import);
}

/* Raise on a failed native read/write status. Success stays silent and does not allocate a VM value. */
static void require_ok(Emitter *e)
{
    call_begin(e, REQUIRE_OK);
    argument(e, MOVW, e->status, 0);
    call_end(e, REQUIRE_OK, -1);
}

/* Emit paired/scalar typed copies; only genuine heap cells use retaining MOVP. */
static void copy_value(Emitter *e, FirModule *module, int destination, int source)
{
    FirType type = module->values[source].type;
    unsigned a = e->offsets[source];
    unsigned d = e->offsets[destination];
    instruction(e, type == T_I64 || type == T_BIG_RANGE ? MOVL : MOVP, FP, a, FP, d, 0, -1);
    if (type == T_TABLE_SLOT || type == T_BIG_RANGE)
    {
        const FirInstruction *in = type == T_TABLE_SLOT ? location(module, source) : NULL;
        unsigned op = in && module->values[in->b].type == T_STRING ? MOVP : MOVL;
        instruction(e, op, FP, a + CELL, FP, d + CELL, 0, -1);
    }
}

/* Lower one verified straight-line body using only existing Dis operations and builtin ABI calls.
 * Entry/callee root maps hold references during allocation; paired slots never encode native addresses. */
static void body(Emitter *e, FirModule *module)
{
    unsigned trace = 0;
    int i;
    instruction(e, LOAD, MP, e->plain_cells * CELL, FP, MODULE_SLOT, 0, 0);
    for (i = 0; i < module->count; i++)
    {
        FirInstruction *in = &module->instructions[i];
        unsigned d = in->result < 0 ? 0 : e->offsets[in->result];
        unsigned a = in->a < 0 ? 0 : e->offsets[in->a];
        unsigned b = in->b < 0 ? 0 : e->offsets[in->b];
        unsigned import;
        const FirInstruction *slot;
        if (in->removed)
        {
            continue;
        }
        switch (in->op)
        {
        case O_I64:
            instruction(e, MOVL, MP, (2U * e->traces + (unsigned)in->result) * CELL, FP, d, 0, -1);
            break;
        case O_STRING:
            instruction(e, MOVP, MP, e->strings[in->result], FP, d, 0, -1);
            break;
        case O_COPY:
            copy_value(e, module, in->result, in->a);
            break;
        case O_TABLE_NEW:
            call_begin(e, CREATE);
            call_end(e, CREATE, (int)d);
            break;
        case O_TABLE_INDEX:
            if (module->values[in->b].type == T_I64)
            {
                call_begin(e, INDEX_CHECK);
                argument(e, MOVL, b, 0);
                call_end(e, INDEX_CHECK, -1);
            }
            instruction(e, MOVP, FP, a, FP, d, 0, -1);
            instruction(e, module->values[in->b].type == T_STRING ? MOVP : MOVL, FP, b, FP, d + CELL, 0, -1);
            break;
        case O_TABLE_LOAD:
        case O_TABLE_STORE:
            slot = location(module, in->a);
            import = in->op == O_TABLE_LOAD ? (module->values[slot->b].type == T_STRING ? GET_TEXT : GET_INDEX)
                                            : (module->values[slot->b].type == T_STRING ? SET_TEXT : SET_INDEX);
            call_begin(e, import);
            argument(e, MOVP, a, 0);
            argument(e, module->values[slot->b].type == T_STRING ? MOVP : MOVL, a + CELL, 1);
            if (in->op == O_TABLE_STORE)
            {
                argument(e, MOVL, b, 2);
            }
            call_end(e, import, (int)e->status);
            require_ok(e);
            if (in->op == O_TABLE_LOAD)
            {
                instruction(e, MOVL, FP, e->status + CELL, FP, d, 0, -1);
            }
            break;
        case O_TABLE_LEN:
        case O_TABLE_KEYS:
        case O_KEYS_LEN:
            import = in->op == O_TABLE_LEN ? LENGTH : in->op == O_TABLE_KEYS ? KEYS_OF : KEY_LEN;
            call_begin(e, import);
            argument(e, MOVP, a, 0);
            call_end(e, import, (int)d);
            break;
        case O_BIG_RANGE:
            call_begin(e, RANGE_CHECK);
            argument(e, MOVL, a, 0);
            argument(e, MOVL, b, 1);
            call_end(e, RANGE_CHECK, -1);
            instruction(e, MOVL, FP, a, FP, d, 0, -1);
            instruction(e, MOVL, FP, b, FP, d + CELL, 0, -1);
            break;
        case O_TABLE_SLICE:
            call_begin(e, SLICE);
            argument(e, MOVP, a, 0);
            argument(e, MOVL, b, 1);
            argument(e, MOVL, b + CELL, 2);
            call_end(e, SLICE, (int)d);
            break;
        case O_TRACE:
            instruction(e, MOVL, FP, a, MP, trace * CELL, 0, -1);
            instruction(e, MOVW, IMM, 1, MP, (e->traces + trace) * CELL, 0, -1);
            trace++;
            break;
        default: /* Preflight admits only O_RETURN here. */
            instruction(e, RET, NONE, 0, NONE, 0, 0, -1);
            break;
        }
    }
}

/* Emit one initialized UTF-8 String at a mapped module cell; text is borrowed and NUL-free by FIR. */
static void string_data(FILE *output, unsigned offset, const char *text)
{
    size_t size = strlen(text);
    octet(output, 3U << 4); /* Existing extended-count DEFS initializer. */
    operand(output, (unsigned)size);
    operand(output, offset);
    fwrite(text, 1, size, output);
}

/* Emit descriptor index/size and exact root bitmap; trailing zero bytes need not be stored. */
static void descriptor(FILE *output, unsigned index, unsigned size, const unsigned char *map)
{
    unsigned bytes = (size / CELL + 7U) / 8U;
    while (bytes && !map[bytes - 1])
    {
        bytes--;
    }
    operand(output, index);
    operand(output, size);
    operand(output, bytes);
    fwrite(map, 1, bytes, output);
}

/* Emit table-capable NC64 Dis from an already verified caller-owned module.
 * Preflight/body failures leave output untouched. I/O failure can publish a prefix; CLI staging
 * protects existing files. No bootstrap/compiler, SDK headers or native pointers are required. */
int fir_dis_table(FILE *output, FirModule *module, char *error, size_t capacity)
{
    Emitter *e = calloc(1, sizeof(*e));
    unsigned i;
    size_t size;
    char buffer[4096];
    int ok = 0;
    extern FILE *fir_temporary(void);
    if (!e)
    {
        snprintf(error, capacity, "Dis table layout allocation failed");
        return 0;
    }
    if (!layout(e, module, error, capacity))
    {
        goto done;
    }
    e->body = fir_temporary();
    if (!e->body)
    {
        snprintf(error, capacity, "Dis table instruction staging failed");
        goto done;
    }
    body(e, module);
    if (ferror(e->body) || fflush(e->body) || fseek(e->body, 0, SEEK_SET))
    {
        snprintf(error, capacity, "Dis table instruction staging I/O failure");
        goto done;
    }
    operand(output, 819264); /* Existing NC64 magic. */
    operand(output, HAS_IMPORTS);
    operand(output, e->frame_size + 4096U);
    operand(output, e->count);
    operand(output, e->data_size);
    operand(output, 2); /* Module data and entry frame descriptors. */
    operand(output, 0); /* Executable harness with no public exports. */
    operand(output, 0);
    operand(output, 1);
    while ((size = fread(buffer, 1, sizeof(buffer), e->body)) != 0)
    {
        fwrite(buffer, 1, size, output);
    }
    descriptor(output, 0, e->data_size, e->data_map);
    descriptor(output, 1, e->frame_size, e->frame_map);
    if (e->plain_cells)
    {
        octet(output, 8U << 4); /* Existing extended-count DEFL initializer. */
        operand(output, e->plain_cells);
        operand(output, 0);
        for (i = 0; i < 2U * e->traces; i++)
        {
            bits(output, 0, CELL);
        }
        for (i = 0; i < (unsigned)module->value_count; i++)
        {
            int defining = module->values[i].definition;
            FirInstruction *in = defining < 0 ? NULL : &module->instructions[defining];
            bits(output, in && in->op == O_I64 && !in->removed ? (uint64_t)in->big : 0, CELL);
        }
    }
    string_data(output, e->plain_cells * CELL, "$Bigtable");
    for (i = 0; i < (unsigned)module->count; i++)
    {
        FirInstruction *in = &module->instructions[i];
        if (!in->removed && in->op == O_STRING)
        {
            string_data(output, e->strings[in->result], in->text);
        }
    }
    octet(output, 0);   /* End data initializers. */
    fwrite(module->module, 1, strlen(module->module) + 1, output);
    operand(output, 1); /* One import interface. */
    operand(output, (unsigned)(sizeof(imports) / sizeof(imports[0])));
    for (i = 0; i < sizeof(imports) / sizeof(imports[0]); i++)
    {
        bits(output, imports[i].signature, 4);
        fwrite(imports[i].name, 1, strlen(imports[i].name) + 1, output);
    }
    octet(output, 0); /* Existing import-list terminator. */
    octet(output, 0); /* Empty source path; source FIRDBG1 remains a separate artifact. */
    ok = !ferror(output) && !ferror(e->body);
    if (!ok)
    {
        snprintf(error, capacity, "Dis table image I/O failure");
    }
done:
    if (e->body)
    {
        fclose(e->body);
    }
    free(e);
    return ok;
}
