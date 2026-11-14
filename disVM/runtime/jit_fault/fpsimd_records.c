/* Copyright (c) 2026 bhgv
 * EN: Bounded Linux ARM64 FPSIMD record decoding; no external context pointers.
 * RU: Ограниченное декодирование Linux ARM64 FPSIMD records; без external pointers.
 */
#include "signals.h"

#define FPSIMD_MAGIC 0x46508001U /* Linux UAPI identifies mandatory base FP/SIMD record. */
#define FPSIMD_BYTES 528U        /* Header8, status/control8,32 vectors of16 bytes. */


/* EN/RU: Read up to8 checked little-endian bytes using integer operations only.
 * Читать до8 проверенных little-endian bytes только integer operations. */
static uint64_t little(const unsigned char *bytes, unsigned int count)
{
    uint64_t value = 0;
    unsigned int index;
    for (index = 0; index < count; index++)
    {
        value |= (uint64_t)bytes[index] << (index * 8);
    }
    return value;
}


/* EN: Walk at most the supplied reserved region. Refuse malformed/truncated record
 * geometry before reading payload. Unknown records are skipped; external extra
 * context/SVE pointers are never followed. Return1 only for a complete base record.
 * RU: Обойти только supplied reserved region. Отказать malformed/truncated record
 * geometry до payload. Unknown records пропускаются; extra context/SVE pointers
 * не разыменовываются. Вернуть1 только при complete base record. */
int dvm_jit_signal_fpsimd(const unsigned char *bytes, size_t size, DVM_JitFault *output)
{
    size_t offset = 0;
    unsigned int index;
    if (!bytes || !output || size > 4096)
    {
        return 0;
    }
    while (offset <= size && size - offset >= 8)
    {
        uint32_t magic = (uint32_t)little(bytes + offset, 4);
        uint32_t length = (uint32_t)little(bytes + offset + 4, 4);
        if (!magic)
        {
            return 0;
        }
        if (length < 16 || length % 16 || length > size - offset)
        {
            return 0;
        }
        if (magic == FPSIMD_MAGIC)
        {
            if (length < FPSIMD_BYTES)
            {
                return 0;
            }
            __atomic_store_n(&output->fp_status, (uint32_t)little(bytes + offset + 8, 4), __ATOMIC_RELAXED);
            __atomic_store_n(&output->fp_control, (uint32_t)little(bytes + offset + 12, 4), __ATOMIC_RELAXED);
            for (index = 0; index < 32; index++)
            {
                __atomic_store_n(
                    &output->fp_registers[index][0], little(bytes + offset + 16 + index * 16, 8), __ATOMIC_RELAXED);
                __atomic_store_n(
                    &output->fp_registers[index][1], little(bytes + offset + 24 + index * 16, 8), __ATOMIC_RELAXED);
            }
            __atomic_store_n(&output->fp_bits, 128, __ATOMIC_RELAXED);
            __atomic_store_n(&output->fp_count, 32, __ATOMIC_RELAXED);
            __atomic_store_n(&output->fp_format, 2, __ATOMIC_RELAXED);
            return 1;
        }
        offset += length;
    }
    return 0;
}
