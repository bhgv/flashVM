/* Copyright (c) 2026 bhgv
 * EN: Copy immutable metadata on the active owner; validate bytecode coordinates.
 * RU: Копировать неизменяемые metadata на active owner; проверять bytecode coordinates.
 */
#include "lib9.h"
#include "interp.h"
#include "location.h"
#include <string.h>


/* EN: Copy a valid UTF-8 prefix into private output of capacity bytes, including
 * NUL. source is a trusted live NUL-terminated metadata string or NULL. Set length
 * and truncation; return zero on invalid captured UTF-8. Never split a codepoint.
 * RU: Копировать UTF-8 prefix в private output ёмкостью capacity с NUL. source —
 * живой metadata string с NUL либо NULL. Записать length/truncation; ноль при
 * неверном UTF-8 в снимаемом prefix. Не разрывать codepoint. */
static int copy_label(char *output, unsigned int capacity, const char *source, int *length, int *truncated)
{
    unsigned int used = 0;
    *length = 0;
    *truncated = 0;
    if (!source)
    {
        return 1;
    }
    while (source[used])
    {
        unsigned int width;
        unsigned int index;
        unsigned char first = (unsigned char)source[used];
        if (first < 0x80)
        {
            width = 1;
        }
        else if (first >= 0xc2 && first <= 0xdf)
        {
            width = 2;
        }
        else if (first >= 0xe0 && first <= 0xef)
        {
            width = 3;
        }
        else if (first >= 0xf0 && first <= 0xf4)
        {
            width = 4;
        }
        else
        {
            return 0;
        }
        if (used + width >= capacity)
        {
            *truncated = 1;
            break;
        }
        for (index = 1; index < width; index++)
        {
            unsigned char next = (unsigned char)source[used + index];
            if (next < 0x80 || next > 0xbf ||
                (index == 1 && ((first == 0xe0 && next < 0xa0) || (first == 0xed && next > 0x9f) ||
                                (first == 0xf0 && next < 0x90) || (first == 0xf4 && next > 0x8f))))
            {
                return 0;
            }
        }
        memcpy(output + used, source + used, width);
        used += width;
    }
    output[used] = 0;
    *length = (int)used;
    return 1;
}


/* [GC:] EN: Borrow only this owner's rooted Modlink/Module and next-PC value. Integer
 * spans reject unrelated/native PCs before any subtraction; labels are private
 * copies and cannot keep a Module alive. output is caller-owned native storage.
 * RU: Заимствовать только rooted Modlink/Module своего owner и next-PC value.
 * Integer spans отказывают посторонним/native PC до вычитания. Подписи копируются
 * и не удерживают Module. output — native память caller. */
void dvm_diagnostic_location_copy(DVM_DiagnosticLocation *output, Modlink *link, const void *next_pc)
{
    Module *module;
    uintptr base;
    uintptr address = (uintptr)next_pc;
    int name_truncated;
    int path_truncated;
    memset(output, 0, sizeof(*output));
    if (!link || link == H || !link->m || link->m == H)
    {
        return;
    }
    module = link->m;
    if (link->compiled || module->compiled || module->nprog <= 0 || !module->prog || module->prog == H ||
        link->prog != module->prog)
    {
        return;
    }
    base = (uintptr)module->prog;
    if ((uintptr)module->nprog > ((uintptr)-1 - base) / sizeof(Inst) || address < base ||
        address - base > (uintptr)module->nprog * sizeof(Inst) || (address - base) % sizeof(Inst))
    {
        return;
    }
    if (!copy_label(output->name, sizeof(output->name), module->name, &output->name_bytes, &name_truncated) ||
        !copy_label(output->path, sizeof(output->path), module->path, &output->path_bytes, &path_truncated))
    {
        memset(output, 0, sizeof(*output));
        return;
    }
    output->next_pc = (int)((address - base) / sizeof(Inst));
    output->nprog = module->nprog;
    output->flags = DVM_LOCATION_VALID | (name_truncated ? DVM_LOCATION_NAME_TRUNCATED : 0) |
                    (path_truncated ? DVM_LOCATION_PATH_TRUNCATED : 0);
}


/* [GC:] EN: Capture the active owner's current rooted module/next PC into private
 * output. Same validation as copied caller positions; no retained references.
 * RU: Снять текущие rooted module/next PC active owner в private output.
 * Проверки как у caller positions; без удерживаемых references. */
void dvm_diagnostic_location_capture(DVM_DiagnosticLocation *output, Modlink *link, const void *next_pc)
{
    dvm_diagnostic_location_copy(output, link, next_pc);
}
