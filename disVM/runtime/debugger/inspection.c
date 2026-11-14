/* Copyright (c) 2026 bhgv
 * EN: Bounded rooted stack/MP/object reads, under a frozen source and shared guards.
 * RU: Bounded rooted stack/MP/object чтение, при frozen source и shared guards.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "pool.h"
#include "heap_pool.h"
#include "managed_owner.h"
#include "internal.h"
#include "frames.h"
#include "census.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

extern Pool *heapmem; /* Source root lifetime is pinned by acknowledged debug gate. */


/* [GC:] EN: Find a rooted stack/MP region and immutable map. The returned base
 * never leaves this ACTIVE inspector call; no pointer supplied by client is used.
 * RU: Найти rooted stack/MP region и immutable map. Base не выходит из ACTIVE
 * inspector call; client не передаёт pointer для разыменования. */
static int region(const DebugFrame *frame, const char *name, uchar **base, size_t *size, Type **type)
{
    if (!strcmp(name, "stack"))
    {
        *base = frame->data;
        *type = frame->type;
        *size = (size_t)frame->type->size;
        return 1;
    }
    if (!strcmp(name, "mp") && frame->module->MP && frame->module->MP != H)
    {
        ulong capacity;
        *base = frame->module->MP;
        *type = D2H(*base)->t;
        capacity = poolmsize(heapmem, D2H(*base));
        if (capacity < sizeof(Heap) || !*type || *type == H)
        {
            return 0;
        }
        *size = capacity - sizeof(Heap);
        return 1;
    }
    return 0;
}


/* [GC:] EN: Copy up to64 bytes from a named rooted region or one mapped strong
 * reference within it. Guards pin the incoming reference and copied byte span;
 * no retain/mark/destructor, arbitrary native address or getter is involved.
 * RU: Копировать до64 bytes named rooted region либо одного mapped strong
 * reference внутри него. Guards удерживают incoming reference и byte span;
 * без retain/mark/destructor, arbitrary native address либо getter. */
static int read_bytes(const DebugFrame *frame,
                      const char *name,
                      int object,
                      unsigned long long slot,
                      unsigned long long offset,
                      unsigned int count,
                      unsigned char *output,
                      size_t *extent,
                      int *kind)
{
    uchar *base;
    Type *type;
    size_t size;
    DVM_RangeAccess access = {0};
    DVM_CellResult claim;
    if (!count || count > 64 || !region(frame, name, &base, &size, &type))
    {
        return 0;
    }
    if (object)
    {
        void *value;
        ulong capacity;
        size_t field;
        if (slot > size || sizeof(value) > size - (size_t)slot || slot % sizeof(value))
        {
            return 0;
        }
        field = (size_t)slot / sizeof(value);
        if ((field / 8) >= (size_t)type->np || !(type->map[field / 8] & (0x80U >> (field % 8))))
        {
            return 0;
        }
        claim = dvm_managed_ranges_begin(base + (size_t)slot, sizeof(value), NULL, 0, &access);
        if (claim != DVM_CELL_ACQUIRED)
        {
            return claim == DVM_CELL_BUSY ? -1 : 0;
        }
        memcpy(&value, base + (size_t)slot, sizeof(value));
        if (!value || value == H)
        {
            dvm_range_access_end(&access);
            return 0;
        }
        capacity = poolmsize(heapmem, D2H(value));
        if (capacity < sizeof(Heap))
        {
            dvm_range_access_end(&access);
            return 0;
        }
        *kind = D2H(value)->t == &Tarray ? 1 : D2H(value)->t == &Tstring ? 2 : 3;
        base = value;
        size = capacity - sizeof(Heap);
        if (offset > size || count > size - (size_t)offset)
        {
            dvm_range_access_end(&access);
            return 0;
        }
        claim = dvm_range_access_extend(&access, base + (size_t)offset, count);
    }
    else
    {
        if (offset > size || count > size - (size_t)offset)
        {
            return 0;
        }
        claim = dvm_managed_ranges_begin(base + (size_t)offset, count, NULL, 0, &access);
    }
    if (claim == DVM_CELL_ACQUIRED)
    {
        memcpy(output, base + (size_t)offset, count);
        *extent = size;
    }
    if (access.domain)
    {
        dvm_range_access_end(&access);
    }
    return claim == DVM_CELL_ACQUIRED ? 1 : claim == DVM_CELL_BUSY ? -1 : 0;
}


/* [GC:] EN/RU: Inspect only acknowledged frozen publications; helper never runs source Prog.
 * Исследовать только acknowledged frozen publication; helper не исполняет source Prog. */
void dvm_debug_inspect(DebugTarget *target, const char *command, char *output, size_t capacity)
{
    char operation[24];
    char name[8];
    char extra;
    int pid;
    unsigned int index;
    unsigned int count;
    unsigned long long slot = 0;
    unsigned long long offset;
    DebugFrame frame;
    if (!target->native_ready || !target->requested)
    {
        snprintf(output, capacity, "{\"ok\":false,\"error\":\"not retained\"}\n");
        return;
    }
    if (!dvm_debug_inspector_enter())
    {
        snprintf(output, capacity, "{\"ok\":false,\"error\":\"inspector unavailable\"}\n");
        return;
    }
    __atomic_fetch_add(&dvm_debug_service.inspections, 1, __ATOMIC_RELAXED);
    snprintf(output, capacity, "{\"ok\":false,\"error\":\"invalid region or command\"}\n");
    if (!strcmp(command, "census") || !strncmp(command, "census ", 7))
    {
        dvm_debug_census(target, output, capacity);
        dvm_debug_inspector_leave();
        return;
    }
    if (sscanf(command, "backtrace %d %u %c", &pid, &index, &extra) == 2 && !strncmp(command, "backtrace ", 10) &&
        dvm_debug_frame(target, index, &frame))
    {
        DVM_DiagnosticLocation location;
        char namehex[DVM_DIAG_NAME_BYTES * 2 + 1];
        unsigned int byte;
        dvm_diagnostic_location_copy(&location, frame.module, frame.pc);
        for (byte = 0; byte < (unsigned int)location.name_bytes; byte++)
        {
            snprintf(namehex + 2 * byte, 3, "%02x", (unsigned char)location.name[byte]);
        }
        namehex[2 * byte] = 0;
        snprintf(
            output,
            capacity,
            "{\"ok\":true,\"frame\":%u,\"pc\":%d,\"instructions\":%d,\"bytes\":%d,\"flags\":%d,\"namehex\":\"%s\"}\n",
            index,
            location.next_pc,
            location.nprog,
            frame.type->size,
            location.flags,
            namehex);
    }
    else
    {
        int object = !strncmp(command, "object ", 7);
        int valid;
        if (object)
        {
            valid = sscanf(command,
                           "%23s %d %7s %u %llu %llu %u %c",
                           operation,
                           &pid,
                           name,
                           &index,
                           &slot,
                           &offset,
                           &count,
                           &extra) == 7;
        }
        else
        {
            valid =
                sscanf(command, "%23s %d %7s %u %llu %u %c", operation, &pid, name, &index, &offset, &count, &extra) ==
                    6 &&
                !strcmp(operation, "read");
        }
        if (valid && dvm_debug_frame(target, index, &frame))
        {
            unsigned char bytes[64];
            char hex[129];
            size_t extent;
            int kind = 0;
            int result = read_bytes(&frame, name, object, slot, offset, count, bytes, &extent, &kind);
            if (result == 1)
            {
                unsigned int byte;
                for (byte = 0; byte < count; byte++)
                {
                    snprintf(hex + 2 * byte, 3, "%02x", bytes[byte]);
                }
                hex[2 * byte] = 0;
                snprintf(output,
                         capacity,
                         "{\"ok\":true,\"extent\":%lu,\"kind\":%d,\"hex\":\"%s\"}\n",
                         (unsigned long)extent,
                         kind,
                         hex);
            }
            else if (result < 0)
            {
                snprintf(output, capacity, "{\"ok\":false,\"error\":\"busy\"}\n");
            }
        }
    }
    dvm_debug_inspector_leave();
}
