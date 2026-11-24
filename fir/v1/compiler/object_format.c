/* Copyright (c) 2026 bhgv
 * EN: Admit verified FIR for AVM1, apply branch fixups and publish one bounded SWO1 record.
 * RU: Проверить применимость FIR к AVM1, замкнуть переходы и выпустить запись SWO1.
 */
#include "object_format.h"
#include <float.h>
#include <stdlib.h>
#include <string.h>


/* EN: Check target-specific arithmetic, types and supported operations before staging actions.
 * RU: Проверить арифметику, типы и поддерживаемые операции до генерации команд. */
int fir_object_admit(FirModule *module, char *error, size_t capacity)
{
    FirIntegerFacts facts;
    int i;
    if (!fir_verify(module, error, capacity))
    {
        return 0;
    }
    if (sizeof(double) != 8 || DBL_MANT_DIG != 53)
    {
        snprintf(error, capacity, "backend requires IEEE binary64 double");
        return 0;
    }
    fir_integer_facts(module, &facts);
    for (i = 0; i < module->count; ++i)
    {
        const FirInstruction *in = &module->instructions[i];
        if (in->removed)
        {
            continue;
        }
        if ((in->op == O_I32_SHL || in->op == O_I32_SHR) && !fir_integer_shift_count(&facts, in->b, 32))
        {
            snprintf(error, capacity, "AVM1 checked int shift requires a proven literal count or bit proof in [0,31]");
            return 0;
        }
        if ((in->op == O_I32_DIV || in->op == O_I32_MOD) && !fir_integer_nonzero(&facts, in->b))
        {
            snprintf(error, capacity,
                "AVM1 signed int division/remainder requires a proven nonzero literal divisor or known set bit");
            return 0;
        }
        if (((in->op > O_RETURN && in->op < O_AS_GLOBAL_GET) && in->op != O_BOOL_LOCAL && in->op != O_BOOL_STORE &&
             in->op != O_BOOL_LOAD && in->op != O_I32_SHL && in->op != O_I32_SHR && in->op != O_I32_DIV &&
             in->op != O_I32_MOD) ||
            (in->type > T_CHANNEL && in->type != T_BOOL_SLOT && in->type != T_I32_SLOT &&
             in->type != T_F64_SLOT && in->type != T_AS_ARGS))
        {
            snprintf(error, capacity, "table/big operations require a table-capable backend");
            return 0;
        }
    }
    return 1;
}


/* EN: Patch already validated branches in private buffered actions. Failure discards the buffer.
 * RU: Записать смещения проверенных переходов в закрытый буфер. При ошибке буфер отбрасывается. */
int fir_object_relocate(unsigned char *actions, const FirAvm1Fixup *fixups, int count, const size_t *labels,
    size_t end, char *error, size_t capacity)
{
    int i;
    for (i = 0; i < count; ++i)
    {
        const FirAvm1Fixup *fixup = &fixups[i];
        size_t target = fixup->target < 0 ? end : labels[fixup->target];
        long offset = (long)target - (long)(fixup->at + 2);
        uint16_t bits;
        if (offset < -32768 || offset > 32767)
        {
            snprintf(error, capacity, "AVM1 branch exceeds signed 16-bit range");
            return 0;
        }
        bits = (uint16_t)offset;
        actions[fixup->at] = (unsigned char)bits;
        actions[fixup->at + 1] = (unsigned char)(bits >> 8);
    }
    return 1;
}


/* EN: Store a 32-bit header field in wire order / RU: Записать поле с порядком байтов SWO. */
static void store_u32(unsigned char *out, uint32_t value)
{
    out[0] = (unsigned char)value;
    out[1] = (unsigned char)(value >> 8);
    out[2] = (unsigned char)(value >> 16);
    out[3] = (unsigned char)(value >> 24);
}


/* EN: Serialize a complete object in memory before writing to the borrowed stream.
 * RU: Сериализовать полный объект в памяти перед записью в заимствованный поток. */
int fir_object_write(FILE *output, const FirModule *module, const unsigned char *actions, size_t size,
    char *error, size_t capacity)
{
    const char *name = module->module + 1;
    size_t name_size = strlen(name), header_size = 24, total = header_size + name_size + size;
    uint32_t hash = UINT32_C(2166136261); /* SWO1 FNV-1a integrity check. */
    unsigned char *object;
    size_t i;
    int ok;
    if (total > FIR_OBJECT_LIMIT)
    {
        snprintf(error, capacity, "object size limit exceeded");
        return 0;
    }
    object = malloc(total);
    if (!object)
    {
        snprintf(error, capacity, "out of memory");
        return 0;
    }
    memcpy(object, "SWO1", 4);
    store_u32(object + 4, 1);
    store_u32(object + 8, module->channels ? 1 : 0);
    store_u32(object + 12, (uint32_t)name_size);
    store_u32(object + 16, (uint32_t)size);
    memcpy(object + header_size, name, name_size);
    memcpy(object + header_size + name_size, actions, size);
    for (i = header_size; i < total; ++i)
    {
        hash = (hash ^ object[i]) * UINT32_C(16777619);
    }
    store_u32(object + 20, hash);
    ok = fwrite(object, 1, total, output) == total;
    free(object);
    if (!ok)
    {
        snprintf(error, capacity, "output write failed");
    }
    return ok;
}
