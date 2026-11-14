/* Copyright (c) 2026 bhgv
 * EN: Human-named base FP/vector bit patterns from retained native diagnostics.
 * RU: Человекочитаемые имена base FP/vector bit patterns из retained native диагностики.
 */
#include "fp_view.h"
#include <stdio.h>
#include <string.h>


/* EN/RU: Format fixed bounded copied register fields; no pointer dereferences from values.
 * Форматировать bounded copied register fields; значения не разыменовываются. */
int dvm_jit_fault_fp_json(const DVM_JitFault *fault, char *output, size_t capacity)
{
    char buffer[3072];
    size_t used;
    unsigned int index;
    if (!fault || !fault->signal_number || !fault->fp_format || fault->fp_format > 3 || fault->fp_count > 32 ||
        fault->x87_count > 8 || (fault->fp_bits != 64 && fault->fp_bits != 128) ||
        (fault->fp_format == 1 && (fault->fp_count != 16 || fault->fp_bits != 128 || fault->x87_count != 8)) ||
        (fault->fp_format == 2 && (fault->fp_count != 32 || fault->fp_bits != 128 || fault->x87_count)) ||
        (fault->fp_format == 3 && (fault->fp_count != 32 || fault->fp_bits != 64 || fault->x87_count)))
    {
        snprintf(output, capacity, "{\"ok\":false,\"error\":\"no valid base FP context\"}\n");
        return 0;
    }
    used = (size_t)snprintf(
        buffer,
        sizeof(buffer),
        "{\"ok\":true,\"architecture\":\"%s\",\"format\":%u,\"coverage\":\"base\",\"bits\":%u,\"control\":\"%x\",\"status\":\"%x\",\"extra\":\"%x\",\"tag\":\"%x\",\"registers\":{",
        dvm_jit_fault_architecture(),
        fault->fp_format,
        fault->fp_bits,
        fault->fp_control,
        fault->fp_status,
        fault->fp_extra,
        fault->fp_tag);
    for (index = 0; index < fault->fp_count; index++)
    {
        used += (size_t)snprintf(buffer + used,
                                 sizeof(buffer) - used,
                                 "%s\"%s%u\":\"",
                                 index ? "," : "",
                                 dvm_jit_fault_fp_prefix(fault->fp_format),
                                 index);
        if (fault->fp_bits == 128)
        {
            used += (size_t)snprintf(
                buffer + used, sizeof(buffer) - used, "%016llx", (unsigned long long)fault->fp_registers[index][1]);
        }
        used += (size_t)snprintf(
            buffer + used, sizeof(buffer) - used, "%016llx\"", (unsigned long long)fault->fp_registers[index][0]);
    }
    used += (size_t)snprintf(buffer + used, sizeof(buffer) - used, "},\"x87\":{");
    for (index = 0; index < fault->x87_count; index++)
    {
        used += (size_t)snprintf(buffer + used,
                                 sizeof(buffer) - used,
                                 "%s\"st%u\":\"%04x%016llx\"",
                                 index ? "," : "",
                                 index,
                                 (unsigned int)(fault->x87_registers[index][1] & 65535U),
                                 (unsigned long long)fault->x87_registers[index][0]);
    }
    used += (size_t)snprintf(buffer + used, sizeof(buffer) - used, "}}\n");
    if (used >= capacity)
    {
        snprintf(output, capacity, "{\"ok\":false,\"error\":\"FP response capacity\"}\n");
        return 0;
    }
    memcpy(output, buffer, used + 1);
    return 1;
}


/* EN/RU: Log only copied integer representations outside signal handler; no FP formatting.
 * Вывести copied integer representations вне signal handler; без FP formatting. */
void dvm_jit_fault_fp_report(const DVM_JitFault *fault, int pid)
{
    char output[3072];
    if (dvm_jit_fault_fp_json(fault, output, sizeof(output)))
    {
        fprintf(stderr, "JITFP pid=%d %s", pid, output);
    }
}
