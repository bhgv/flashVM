/* Copyright (c) 2026 bhgv
 * EN: Independently construct bounded ARM64 records and validate base FP formatting failures.
 * RU: Независимо создать bounded ARM64 records и проверить отказы base FP formatting.
 */
#include "jit_fault/signals.h"
#include "jit_fault/fp_view.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int dvm_jit_signal_fpsimd(const unsigned char *, size_t, DVM_JitFault *);


/* EN/RU: Fail with an explicit semantic diagnostic; never use assertions.
 * Завершить с явной semantic diagnostic; без assertions. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL FP records: %s\n", message);
        exit(12);
    }
}


/* EN/RU: Encode count little-endian bytes into disjoint bounded test storage.
 * Записать count little-endian bytes в отдельное bounded test storage. */
static void put(unsigned char *output, uint64_t value, unsigned int count)
{
    unsigned int index;
    for (index = 0; index < count; index++)
    {
        output[index] = (unsigned char)(value >> (index * 8));
    }
}


/* EN: Check truncation, invalid record sizes, unknown record skip, extra-pointer
 * non-following, last-vector bounds and refusal output preservation. This is a
 * wire-record unit, not proof of actual ARM64 signal delivery on another CPU.
 * RU: Проверить truncation, размеры records, unknown skip, отказ переходить по
 * extra-pointer, last-vector bounds и output preservation. Это record unit,
 * не доказательство ARM64 signal delivery на другом CPU. */
int main(void)
{
    unsigned char bytes[560] = {0};
    DVM_JitFault output;
    DVM_JitFault unchanged;
    char json[3072];
    unsigned int index;
    put(bytes, UINT32_C(0x45585401), 4);
    put(bytes + 4, 32, 4);
    put(bytes + 8, UINT64_C(0xffffffffffffffff), 8); /* Invalid external pointer must be ignored. */
    put(bytes + 32, UINT32_C(0x46508001), 4);
    put(bytes + 36, 528, 4);
    put(bytes + 40, 0x1234, 4);
    put(bytes + 44, 0x5678, 4);
    for (index = 0; index < 32; index++)
    {
        put(bytes + 48 + index * 16, UINT64_C(0x1122334455667700) + index, 8);
        put(bytes + 56 + index * 16, UINT64_C(0x8877665544332200) + index, 8);
    }
    memset(&unchanged, 0xa5, sizeof(unchanged));
    for (index = 0; index < sizeof(bytes); index++)
    {
        output = unchanged;
        require(!dvm_jit_signal_fpsimd(bytes, index, &output), "truncated payload accepted");
        require(!memcmp(&output, &unchanged, sizeof(output)), "truncated refusal changed output");
    }
    memset(&output, 0, sizeof(output));
    require(dvm_jit_signal_fpsimd(bytes, sizeof(bytes), &output), "complete base record refused");
    require(output.fp_format == 2 && output.fp_count == 32 && output.fp_bits == 128 && output.fp_status == 0x1234 &&
                output.fp_control == 0x5678 && output.fp_registers[31][0] == UINT64_C(0x112233445566771f) &&
                output.fp_registers[31][1] == UINT64_C(0x887766554433221f),
            "complete last vector/control corrupted");
    output.signal_number = 4;
    require(dvm_jit_fault_fp_json(&output, json, sizeof(json)) && strstr(json, "887766554433221f112233445566771f"),
            "base bits were FP-converted/truncated");
    require(!dvm_jit_fault_fp_json(&output, json, 32), "undersized response accepted");
    output.fp_format = 999;
    require(!dvm_jit_fault_fp_json(&output, json, sizeof(json)), "unknown format accepted");
    for (index = 0; index < 4; index++)
    {
        static const unsigned int sizes[] = {0, 8, 17, 4096}; /* Invalid unknown record geometries. */
        put(bytes + 4, sizes[index], 4);
        output = unchanged;
        require(!dvm_jit_signal_fpsimd(bytes, sizeof(bytes), &output) && !memcmp(&output, &unchanged, sizeof(output)),
                "bad geometry changed output");
    }
    require(!dvm_jit_signal_fpsimd(bytes, 4097, &output), "unbounded extension region accepted");
    puts("PASS bounded ARM64 FP records,560 truncations, external-pointer refusal and exact base bits");
    return 0;
}
